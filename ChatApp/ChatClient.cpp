#include "pch.h"
#include "ChatClient.h"
#include <WS2tcpip.h>

ChatClient::ChatClient()
{
}

ChatClient::~ChatClient()
{
}

bool ChatClient::MakeNonBlockingSocket(CString sServerIP, UINT uiPort)
{
    CString sValue;

    // 1. ���� ����
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        OutputDebugString(_T("Socket creation failed.\n"));
        return false;
    }

    // 2. ����ŷ ��� ����
    u_long mode = 1;
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) 
    {
        OutputDebugString(_T("ioctlsocket failed\n"));
        Close();
        return false;
    }

    // 3. ���� �ּ� ����
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(uiPort);
    if (InetPton(AF_INET, sServerIP, &serverAddr.sin_addr) != 1)    //InetPton�� ���ڿ� IP�� ���� IP�� �����ϰ� ��ȯ
    {
        OutputDebugString(_T("Invalid IP address format\n"));
        Close();
        return false;
    }

    // 4. �񵿱� ���� �õ�
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
        // �� ������ ����ŷ���� "���� ��" ������ �� �����Ƿ� ���� �ƴ�
    }

    sValue.Format(_T("Trying to connect to %s:%d...\n"), sServerIP, uiPort);
    OutputDebugString(sValue);

    return true;
}

bool ChatClient::Connect(CString sServerIP, UINT uiPort)
{
    MakeNonBlockingSocket(sServerIP, uiPort);
    return false;
}
