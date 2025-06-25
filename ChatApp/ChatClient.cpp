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

    m_sServerIP = sServerIP;
    m_uiPort = uiPort;

    return true;
}

bool ChatClient::Connect(CString sServerIP, UINT uiPort)
{
    if (!MakeNonBlockingSocket(sServerIP, uiPort))
    {
        OutputDebugString(_T("Ŭ���̾�Ʈ Connect ���� ���� ���� ����\n"));
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
    if (!m_sock) return false;
    bool bResult = false;
    CString sValue;

    while (TRUE)
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 1; // �ִ� 1�� ���
        timeout.tv_usec = 0;

        int iResult = select(0, NULL, &writeSet, NULL, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //�����Ǹ� Ŭ���̾�Ʈ ���� ����
        {
            OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����\n"));
            bResult = true;
            break;
        }
        else 
        {
            // ���� ���� �Ǵ� Ÿ�Ӿƿ�
            int iOptVal;
            int iOptLen = sizeof(iOptVal);
            getsockopt(m_sock, SOL_SOCKET, SO_ERROR, (char*)&iOptVal, &iOptLen);
            // optval�� ���� ���� �ڵ� Ȯ�� ����

            if (iOptVal == 0)
            {  
                bResult = true;

                sValue.Format(_T("%d Ŭ���̾�Ʈ ���� ����\n"), m_sock);
                OutputDebugString(sValue);
                if(m_fCallback)
                    m_fCallback(sValue);
                break;
            }
            else
            {
                bResult = false;

                sValue.Format(_T("%d Ŭ���̾�Ʈ ���� ����... ��õ�\\n"), m_sock);
                OutputDebugString(sValue);
                if (m_fCallback)
                    m_fCallback(sValue);
            }
        }
    }

    if (bResult)
    {
        std::thread SendThread(&ChatClient::SendThread, this);
        SendThread.detach();
    }
}

bool ChatClient::SendThread(ChatClient* pClient)
{
    if (pClient)
    {
        return pClient->SendProc();
    }
    return false;
}

bool ChatClient::SendProc()
{
    while (!m_bExit)
    {
        DWORD dwResult = WaitForSingleObject(NULL, 1000);
        const int iSendCount = m_aSend.GetCount();
        if (iSendCount > 0)
        {
            for (int i = 0; i < m_aSend.GetCount(); i++)
            {
                CString sValue = m_aSend.GetAt(i);
                char cSend[500];
                memcpy_s(&cSend, 500, sValue, sValue.GetLength());
                send(m_sock, cSend, sValue.GetLength(), 0);
                
                sValue.Format(_T("%s ��Ŷ ���� �Ϸ�\n"), cSend);
                if (RemoveSend(i)) i -= 1;

                OutputDebugString(sValue);
            }
        }
    }
    return false;
}
