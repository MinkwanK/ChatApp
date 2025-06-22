#include "pch.h"
#include "ChatServer.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

ChatServer::ChatServer()
{

}
ChatServer::~ChatServer()
{
    Close();
}

bool ChatServer::MakeNonBlockingSocket()
{
    CString sValue;

    // 2. TCP 소켓 생성
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        return false;
    }

    // 3. 소켓을 논블로킹 모드로 설정
    u_long mode = 1;  // 1이면 non-blocking
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) {
        std::cerr << "ioctlsocket failed.\n";
        Close();
        return false;
    }

    // 4. 주소 구조체 설정: INADDR_ANY + 포트 0 (자동 포트 선택
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;                // IPv4 사용
    serverAddr.sin_addr.s_addr = INADDR_ANY;        // 모든 IP에서 수신
    serverAddr.sin_port = 0;                        // 포트 0 -> OS가 가용 포트 자동 할당

     // 5. 바인딩 (소켓을 IP/포트에 연결) 
    if (bind(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        Close();
        return false;
    }

    // 6. 실제 할당된 포트 번호 얻기
    int addrLen = sizeof(serverAddr);
    if (getsockname(m_sock, (SOCKADDR*)&serverAddr, &addrLen) == 0) 
    {
        sValue.Format(_T("Server port: %d\n"), ntohs(serverAddr.sin_port));
        OutputDebugString(sValue);
    }

    return true;
}

bool ChatServer::StartServer()
{
    if (!MakeNonBlockingSocket())   //논블로킹 소켓 생성
    {
        AfxMessageBox(_T("서버 소켓 생성 실패"));
        return false;
    }
        
    if (!listen)    //리스닝
    {
        AfxMessageBox(_T("서버 리스닝 실패"));
        return false;
    }

    std::thread AcceptTrhead(&ChatServer::Accept, this);   //Accept Thread
    AcceptTrhead.detach();
    return true;
}

bool ChatServer::Listen()
{
        // 7. 리스닝 시작
     if (listen(m_sock, SOMAXCONN) == SOCKET_ERROR) {
         OutputDebugString(_T("Listen Failed\n"));
         Close();
         return false;
     }
     return true;
}

bool ChatServer::Accept(ChatServer* pServer)
{
    if (pServer)
    {
        return pServer->Acceptproc();
    }
    return false;
}

bool ChatServer::Acceptproc()
{
    CString sValue;
    m_accepting = true;
    // 논블로킹 방식으로 클라이언트 수신 대기 (select 사용)
    while (!m_bExit)    //클라이언트 연결을 기다림 
    {
        fd_set readfds;                  // 감시할 소켓 목록 선언
        FD_ZERO(&readfds);              // 목록 초기화
        FD_SET(m_sock, &readfds);  // 감시할 소켓 등록

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        /*
        select(): 지정한 소켓이 읽기/쓰기/예외 처리가 가능한지 감지, 블로킹 함수
        FD_ISSET은 해당 소켓이 실제로 이벤트가 감지됐는지 확인
        select를 통해 소켓이 읽을 준비가 됐을 때만 처리, cpu를 거의 쓰지 않음
        */
        int result = select(0, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(m_sock, &readfds))   //감지되면 클라이언트 연결 요청이 있다는 뜻
        {
            SOCKET clientSock = accept(m_sock, NULL, NULL);
            if (clientSock != INVALID_SOCKET)
            {
                sValue.Format(_T("%d 클라이언트 소켓 접속 완료\n"), clientSock);
                OutputDebugString(sValue);
                AfxMessageBox(sValue);
                return TRUE;
            }
        }
        else
        {
            OutputDebugString(_T("Select Failed\n"));
        }
    }
    m_accepting = false;
    return FALSE;
}
