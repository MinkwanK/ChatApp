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

    std::thread ConnectThread(Client::ConnectThread, this);
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
    //sValue.Format(_T("Trying to connect to %s:%d...\n"), sServerIP, uiPort);
    //OutputDebugString(sValue);

    while (!m_bExit)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 1; // 최대 1초 대기
        timeout.tv_usec = 0;

        int iResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //감지되면 클라이언트 연결 성공
        {
            sValue.Format(_T("[클라이언트] 성공: %d 접속\n"), m_sock);
            OutputDebugString(sValue);
            if (m_fCallback)
                m_fCallback(NETWORK_EVENT::Connect, m_sock, sValue);
            break;
        }
        else 
        {
            if (iResult == 0)
            {
                OutputDebugString(_T("[클라이언트] 에러: StartClient Connect 타임아웃\n"));

                if (!Connect())
                    return false;
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();

                CString sMsg;
                sMsg.Format(_T("[클라이언트] 에러: StartClient - select() 실패 - WSAGetLastError(): %d\n"), iError);
                OutputDebugString(sMsg);

                if (m_fCallback)
                    m_fCallback(NETWORK_EVENT::Error, m_sock, sValue);

                CloseSocket();
                return false;
            }
        }
    }

    std::thread SendThread(&Client::SendThread, this);
    SendThread.detach();
    return true;
}

bool Client::Connect()
{
	//비동기 연결
    const int result = connect(m_sock, reinterpret_cast<SOCKADDR*>(&m_serverAddr), sizeof(m_serverAddr));
    if (result == SOCKET_ERROR)
    {
        int iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS && iError != WSAEINVAL)
        {
	        OutputDebugString(_T("Socket creation failed.\n"));
	        CloseSocket();
	        return false;
        }
    }
    return true;
}

bool Client::SendThread(Client* pClient)
{
    if (pClient)
    {
        return pClient->SendProc();
    }
    return false;
}

bool Client::SendProc()
{
    CString sValue;
    while (!m_bExit)
    {
        if (m_sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            m_fCallback(NETWORK_EVENT::Error, m_sock, sValue);
            break;
        }
        // select 준비
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1초 대기

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] 오류: select (%d)"), WSAGetLastError());
            m_fCallback(NETWORK_EVENT::Error, m_sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(m_sock, &writeSet))
        {
            Send();
        }
    }
    return false;
}
