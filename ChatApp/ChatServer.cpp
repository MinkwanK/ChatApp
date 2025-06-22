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

    // 2. TCP ���� ����
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        return false;
    }

    // 3. ������ ����ŷ ���� ����
    u_long mode = 1;  // 1�̸� non-blocking
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) {
        std::cerr << "ioctlsocket failed.\n";
        Close();
        return false;
    }

    // 4. �ּ� ����ü ����: INADDR_ANY + ��Ʈ 0 (�ڵ� ��Ʈ ����
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;                // IPv4 ���
    serverAddr.sin_addr.s_addr = INADDR_ANY;        // ��� IP���� ����
    serverAddr.sin_port = 0;                        // ��Ʈ 0 -> OS�� ���� ��Ʈ �ڵ� �Ҵ�

     // 5. ���ε� (������ IP/��Ʈ�� ����) 
    if (bind(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        Close();
        return false;
    }

    // 6. ���� �Ҵ�� ��Ʈ ��ȣ ���
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
    if (!MakeNonBlockingSocket())   //����ŷ ���� ����
    {
        AfxMessageBox(_T("���� ���� ���� ����"));
        return false;
    }
        
    if (!listen)    //������
    {
        AfxMessageBox(_T("���� ������ ����"));
        return false;
    }

    std::thread AcceptTrhead(&ChatServer::Accept, this);   //Accept Thread
    AcceptTrhead.detach();
    return true;
}

bool ChatServer::Listen()
{
        // 7. ������ ����
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
    // ����ŷ ������� Ŭ���̾�Ʈ ���� ��� (select ���)
    while (!m_bExit)    //Ŭ���̾�Ʈ ������ ��ٸ� 
    {
        fd_set readfds;                  // ������ ���� ��� ����
        FD_ZERO(&readfds);              // ��� �ʱ�ȭ
        FD_SET(m_sock, &readfds);  // ������ ���� ���

        timeval timeout{};
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        /*
        select(): ������ ������ �б�/����/���� ó���� �������� ����, ���ŷ �Լ�
        FD_ISSET�� �ش� ������ ������ �̺�Ʈ�� �����ƴ��� Ȯ��
        select�� ���� ������ ���� �غ� ���� ���� ó��, cpu�� ���� ���� ����
        */
        int result = select(0, &readfds, NULL, NULL, &timeout);
        if (result > 0 && FD_ISSET(m_sock, &readfds))   //�����Ǹ� Ŭ���̾�Ʈ ���� ��û�� �ִٴ� ��
        {
            SOCKET clientSock = accept(m_sock, NULL, NULL);
            if (clientSock != INVALID_SOCKET)
            {
                sValue.Format(_T("%d Ŭ���̾�Ʈ ���� ���� �Ϸ�\n"), clientSock);
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
