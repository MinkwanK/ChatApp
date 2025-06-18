#include "pch.h"
#include "ChatClient.h"
#include <WS2tcpip.h>
#include <thread>

ChatClient::ChatClient()
{
}

ChatClient::~ChatClient()
{
}

bool ChatClient::MakeNonBlockingSocket(CString sServerIP, UINT uiPort)
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
        Close();
        return false;
    }

    // 3. 서버 주소 설정
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(uiPort);
    if (InetPton(AF_INET, sServerIP, &serverAddr.sin_addr) != 1)    //InetPton은 문자열 IP를 이진 IP로 안전하게 변환
    {
        OutputDebugString(_T("Invalid IP address format\n"));
        Close();
        return false;
    }

    // 4. 비동기 연결 시도
    int result = connect(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) 
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK && err != WSAEINPROGRESS && err != WSAEINVAL) 
        {
            OutputDebugString(_T("Socket creation failed.\n"));
            Close();
            return false;
        }
        // 위 에러는 논블로킹에서 "연결 중" 상태일 수 있으므로 오류 아님
    }

    sValue.Format(_T("Trying to connect to %s:%d...\n"), sServerIP, uiPort);
    OutputDebugString(sValue);

    m_sServerIP = sServerIP;
    m_uiPort = uiPort;

    return true;
}

bool ChatClient::Connect(CString sServerIP, UINT uiPort)
{
    if (!MakeNonBlockingSocket(sServerIP, uiPort))
    {
        OutputDebugString(_T("클라이언트 Connect 위한 소켓 생성 실패\n"));
    }

    std::thread ConnectThread(ChatClient::ConnectThread, this);
    ConnectThread.detach();
    return TRUE;
}

bool ChatClient::ConnectThread(ChatClient* pClient)
{
    if (pClient)
    {
        return pClient->ConnectProc();
    }

    return FALSE;
}

bool ChatClient::ConnectProc()
{
    if (!m_sock) return FALSE;

    while (TRUE)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 1; // 최대 1초 대기
        timeout.tv_usec = 0;

        int iResult = select(0, NULL, &writeSet, NULL, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //감지되면 클라이언트 연결 성공
        {
            OutputDebugString(_T("클라이언트 접속 성공\n"));
            return true;
        }
        else 
        {
            // 연결 실패 또는 타임아웃
            int iOptVal;
            int iOptLen = sizeof(iOptVal);
            getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&iOptVal, &iOptLen);
            // optval을 통해 에러 코드 확인 가능

            if (iOptVal == 0)
            {
                OutputDebugString(_T("클라이언트 접속 성공\n"));
                return TRUE;
            }
            else
            {
                OutputDebugString(_T("클라이언트 접속 실패... 재시도\n"));
            }
        }
    }
}
