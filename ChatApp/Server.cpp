#include "pch.h"
#include "Server.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

Server::Server()
{

}
Server::~Server()
{
    Clear();
}

bool Server::MakeNonBlockingSocket()
{
    CString sValue;

    // 2. TCP 소켓 생성
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        sValue = _T("[Server] Socket creation failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, -1, sValue);
        return false;
    }

    // 3. 소켓을 논블로킹 모드로 설정
    u_long mode = 1;  // 1이면 non-blocking
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) {
        sValue = _T("[Server] ioctlsocket failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, sValue);
        Clear();
        return false;
    }

    // 4. 주소 구조체 설정: INADDR_ANY + 포트 0 (자동 포트 선택)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = m_uiPort == -1 ? 0 : htons(m_uiPort);

    // 5. 바인딩
    if (bind(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        sValue = _T("[Server] Bind failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, sValue);
        Clear();
        return false;
    }

    // 6. 실제 할당된 포트 번호 얻기
    int addrLen = sizeof(serverAddr);
    if (getsockname(m_sock, (SOCKADDR*)&serverAddr, &addrLen) == 0)
    {
        sValue.Format(_T("[Server] port: %d\n"), ntohs(serverAddr.sin_port));
        OutputDebugString(sValue);
    }

    return true;
}

bool Server::StartServer()
{
    if (!MakeNonBlockingSocket())   //논블로킹 소켓 생성
    {
        return false;
    }

    if (!Listen())    //리스닝
    {
        return false;
    }

    std::thread AcceptTrhead(&Server::Accept, this);   //Accept Thread
    AcceptTrhead.detach();
    return true;
}

bool Server::Listen()
{
    CString sValue;

    if (listen(GetSocket(), SOMAXCONN) == SOCKET_ERROR)
    {
        sValue.Format(_T("[Server] Listen Failed\n"));
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, sValue);
        Clear();
        return false;
    }
    sValue.Format(_T("[Server] Port: %d Listen\n"), m_uiPort);
    UseCallback(NETWORK_EVENT::LISTEN, PACKET{}, m_sock, sValue);
    return true;
}

bool Server::Accept(Server* pServer)
{
    if (pServer)
    {
        return pServer->Acceptproc();
    }
    return false;
}

bool Server::Acceptproc()
{
    m_accepting = true;
    SetRunning(true);
    // 논블로킹 방식으로 클라이언트 수신 대기 (select 사용)
    while (!m_bExitProccess)    //클라이언트 연결을 기다림 
    {
        //종료 이벤트 체크
        if (WaitForSingleObject(m_hExit, 0) == WAIT_OBJECT_0)
        {
            break;
        }

        fd_set readfds;                  // 감시할 소켓 목록 선언
        FD_ZERO(&readfds);              // 목록 초기화
        FD_SET(m_sock, &readfds);       // 감시할 소켓 등록

        timeval timeout{};
        timeout.tv_sec = 1;

        /*
        select(): 지정한 소켓이 읽기/쓰기/예외 처리가 가능한지 감지, 블로킹 함수
        FD_ISSET은 해당 소켓이 실제로 이벤트가 감지됐는지 확인
        select를 통해 소켓이 읽을 준비가 됐을 때만 처리, cpu를 거의 쓰지 않음
        */
        int result = select(0, &readfds, nullptr, nullptr, &timeout);
        if (result > 0 && FD_ISSET(m_sock, &readfds))   //감지되면 클라이언트 연결 요청이 있다는 뜻
        {
            SOCKET clientSock = accept(m_sock, nullptr, nullptr);
            if (clientSock != INVALID_SOCKET)
            {
                CString sMsg;
                sMsg.Format(_T("[Server] 클라이언트 접속 완료 (소켓: %d)\n"), clientSock);
                UseCallback(NETWORK_EVENT::ACCEPT, PACKET{}, clientSock, sMsg);

                std::thread sendThread(&Server::SendThread, this, clientSock);
                sendThread.detach();

                std::thread recvThread(&Server::RecvThread, this, clientSock);
                recvThread.detach();
            }
            else
            {
                UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, _T("[Server] Accept() 실패\n"));
            }
        }
        else if (result == SOCKET_ERROR)
        {
            UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, _T("[Server] Accept select() 실패\n"));
            break; // 에러 발생 시 루프 탈출
        }
        else
        {
            //timeout   
        }
    }
    SetRunning(false);
    m_accepting = false;
    return FALSE;
}

void Server::SendThread(Server* pServer, SOCKET sock)
{
    if (pServer)
    {
        pServer->SendProc(sock);
    }
}

void Server::SendProc(SOCKET sock)
{
    OutputDebugString(_T("[Server] Send Thread Start\n"));
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
            sValue.Format(_T("[Server] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
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
            sValue.Format(_T("[Server] SEND 오류: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(sock, &writeSet))
        {
            const int iResult = Send();
            if (iResult == 0)
            {
                OutputDebugString(_T("[Server] Send 가 0이므로 Exit 이벤트 발동\n"));
                SetEvent(m_hExit);
                RemoveAllSend();
                break;
            }
        }
    }
    SetRunning(false);
    OutputDebugString(_T("[Server] Send Thread End\n"));
}

int Server::Send()
{
    EnterCriticalSection(&m_cs);
    CString sValue;
    const int iSendCount = m_aSend.GetCount();
    int iSend = -1;
    if (iSendCount > 0)
    {
        PACKET packet = m_aSend.GetAt(0);
        SOCKET targetSock = packet.sock;  // 패킷 내에 있는 정확한 소켓 사용

        if (targetSock != INVALID_SOCKET)
        {
            iSend = send(targetSock, packet.pszData, packet.uiSize, 0);

            if (iSend > 0)
                sValue.Format(_T("[Server][SEND] %d 소켓 %d 바이트\n"), targetSock, iSend);
            else if (iSend == 0)
                sValue.Format(_T("[Server][SEND] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), targetSock);
            else
            {
                sValue.Format(_T("[Server][SEND] %d 소켓 실패코드 %d\n"), targetSock, WSAGetLastError());
                return false;
            }

            UseCallback(NETWORK_EVENT::SEND, packet, targetSock, sValue);
        }

        RemoveSend(0);
    }

    LeaveCriticalSection(&m_cs);
    return iSend;
}

void Server::RecvThread(Server* pServer, SOCKET sock)
{
    if (pServer)
    {
        pServer->RecvProc(sock);
    }
}

void Server::RecvProc(SOCKET sock)
{
    OutputDebugString(_T("[Server] Recv Thread Start\n"));
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
            sValue.Format(_T("[Server] 오류: 유효하지 않은 소켓 (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        // select 준비
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 1;

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Server] Recv 오류: select (%d)"), WSAGetLastError());
            UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // 타임아웃, 다음 루프

        if (FD_ISSET(sock, &readSet))
        {
            const int iResult = Read(sock);
            if (iResult == 0)
            {
                SetEvent(m_hExit);
                RemoveAllSend();
                break;
            }
        }
    }
    SetRunning(false);
    OutputDebugString(_T("[Server] Recv Thread End\n"));
}

int Server::Read(SOCKET sock)
{
    CString sValue;
    char buf[MAX_BUF];
    int iRecv = recv(sock, buf, MAX_BUF, 0);

    if (iRecv > 0)
    {
        sValue.Format(_T("[Server][RECV] %d 소켓 %d 바이트\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Server][RECV] %d 소켓 정상 종료 (상대방이 연결 끊음)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Server][RECV] %d 소켓 실패코드 %d\n"), sock, WSAGetLastError());
        return false;
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
