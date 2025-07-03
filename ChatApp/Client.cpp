#include "pch.h"
#include "Client.h"
#include <WS2tcpip.h>
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Client::Client()
{
}

Client::~Client()
{
    Clear();
}

bool Client::MakeNonBlockingSocket()
{
    CString sValue;

    if (m_sock)
    {
        closesocket(m_sock);
        m_sock = NULL;
    }
    // 1. 소켓 생성
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        OutputDebugString(_T("Socket creation failed.\n"));
        return false;
    }

    // 2. 논블로킹 모드 설정
    u_long mode = 1;
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0)
    {
        OutputDebugString(_T("ioctlsocket failed\n"));
        Clear();
        return false;
    }

    // 3. 서버 주소 설정;
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_uiPort);
    if (InetPton(AF_INET, m_sIP, &m_serverAddr.sin_addr) != 1)    //InetPton은 문자열 IP를 이진 IP로 안전하게 변환
    {
        OutputDebugString(_T("Invalid IP address format\n"));
        Clear();
        return false;
    }

    return true;
}

bool Client::StartClient()
{
    if (!MakeNonBlockingSocket())
    {
        OutputDebugString(_T("클라이언트 StartClient 위한 소켓 생성 실패\n"));
    }

    std::thread ConnectThread(&Client::ConnectThread, this);
    ConnectThread.detach();
    return TRUE;
}

bool Client::ConnectThread(Client* pClient)
{
    if (pClient)
    {
        return pClient->ConnectProc();
    }

    return FALSE;
}

bool Client::ConnectProc()
{
    if (m_sock == INVALID_SOCKET)
        return false;

    if (!Connect())
        return false;

    CString sValue;

    while (!m_bExitProccess)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 1; // 최대 1초 대기
        timeout.tv_usec = 0;

        const int iResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //감지되면 클라이언트 연결 성공
        {
            sValue.Format(_T("[클라이언트] 성공: %d 접속\n"), m_sock);
            OutputDebugString(sValue);
            if (m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::CONNECT, {}, m_sock, sValue);
            break;
        }
        else
        {
            if (iResult == 0)
            {
                sValue.Format(_T("[클라이언트] 에러: StartClient CONNECT 타임아웃\n"));
                OutputDebugString(sValue);
                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, m_sock, sValue);
                if (!Connect())
                {
                    return false;
                }
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();

                CString sMsg;
                sMsg.Format(_T("[클라이언트] 에러: StartClient - select() 실패 - WSAGetLastError(): %d\n"), iError);
                OutputDebugString(sMsg);

                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, m_sock, sValue);

                CloseSocket();
                return false;
            }
        }
    }

    std::thread sendThread(&Client::SendThread, this, m_sock);
    sendThread.detach();

    std::thread recvThread(&Client::RecvThread, this, m_sock);
    recvThread.detach();

    return true;
}

bool Client::Connect()
{
    //비동기 연결
    const int result = connect(m_sock, reinterpret_cast<SOCKADDR*>(&m_serverAddr), sizeof(m_serverAddr));
    if (result == SOCKET_ERROR)
    {
        int iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS && iError != WSAEINVAL && iError != WSAEALREADY)   //WSAEALREADY 비동기 연결 이미 시도 중...
        {
            CString sValue;
            sValue.Format(_T("[클라이언트] 에러: StartClient CONNECT 시도 중 에러발생 : %d\n"), iError);
            UseCallback(NETWORK_EVENT::NE_ERROR, {}, m_sock, sValue);
            CloseSocket();
            return false;
        }
    }
    return true;
}

void Client::SendThread(Client* pClient, SOCKET sock)
{
    if (pClient)
    {
        pClient->SendProc(sock);
    }
}

void Client::SendProc(SOCKET sock)
{
    CString sValue;
    while (!m_bExitProccess)
    {
        DWORD dwResult = WaitForSingleObject(m_hExit, 100);
        if (dwResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }
        // select 준비
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1초 대기

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] SEND 오류: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(sock, &writeSet))
        {
            int iResult = Send(sock);
            if (iResult == 0)
            {
                SetEvent(m_hExit);
                RemoveAllSend();
                StartClient();
                break;
            }
        }
    }
}

int Client::Send(SOCKET sock)
{
    EnterCriticalSection(&m_cs);
    CString sValue;
    const int iSendCount = m_aSend.GetCount();
    int iSend = -1;
    if (iSendCount > 0)
    {
        PACKET packet = m_aSend.GetAt(0);
        iSend = send(sock, packet.pszData, packet.uiSize, 0);

        if (iSend > 0)
        {
            sValue.Format(_T("[Client][SEND] %d 소켓 %d 바이트\n"), sock, iSend);
        }
        else if (iSend == 0)
        {
            sValue.Format(_T("[Client][SEND] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), sock);
        }
        else
        {
            sValue.Format(_T("[Client][SEND] %d 소켓 실패코드 %d\n"), sock, WSAGetLastError());
        }

        UseCallback(NETWORK_EVENT::SEND, packet, sock, sValue);
        RemoveSend(0);
    }
    LeaveCriticalSection(&m_cs);
    return iSend;
}

void Client::RecvThread(Client* pClient, SOCKET sock)
{
    if (pClient)
    {
        pClient->RecvProc(sock);
    }
}

void Client::RecvProc(SOCKET sock)
{
    CString sValue;
    while (!m_bExitProccess)
    {
        DWORD dwResult = WaitForSingleObject(m_hExit, 100);
        if (dwResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }
        // select 준비
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1초 대기

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Recv 오류: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(sock, &readSet))
        {
            int iResult = Read(sock);
            if (iResult == 0)
            {
                SetEvent(m_hExit);
                RemoveAllSend();
                StartClient();
                break;
            }
        }
    }
}

int Client::Read(SOCKET sock)
{
    CString sValue;
    char buf[MAX_BUF];
    int iRecv = recv(sock, buf, MAX_BUF, 0);

    if (iRecv > 0)
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 %d 바이트\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Client][RECV] %d 소켓 실패코드 %d\n"), sock, WSAGetLastError());
    }

    PACKET packet = {};

    if (iRecv > 0)
    {
        packet.pszData = buf;
        packet.uiSize = iRecv;
        UseCallback(NETWORK_EVENT::RECV, packet, sock, sValue);
    }
    else
    {
        UseCallback(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
    }
    return iRecv;
}
