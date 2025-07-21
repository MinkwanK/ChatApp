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

    // 2. TCP ���� ����
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        sValue = _T("[Server] Socket creation failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, -1, sValue);
        return false;
    }

    // 3. ������ ����ŷ ���� ����
    u_long mode = 1;  // 1�̸� non-blocking
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) {
        sValue = _T("[Server] ioctlsocket failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, sValue);
        Clear();
        return false;
    }

    // 4. �ּ� ����ü ����: INADDR_ANY + ��Ʈ 0 (�ڵ� ��Ʈ ����)
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = m_uiPort == -1 ? 0 : htons(m_uiPort);

    // 5. ���ε�
    if (bind(m_sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        sValue = _T("[Server] Bind failed.\n");
        UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, sValue);
        Clear();
        return false;
    }

    // 6. ���� �Ҵ�� ��Ʈ ��ȣ ���
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
    if (!MakeNonBlockingSocket())   //����ŷ ���� ����
    {
        return false;
    }

    if (!Listen())    //������
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
    // ����ŷ ������� Ŭ���̾�Ʈ ���� ��� (select ���)
    while (!m_bExitProccess)    //Ŭ���̾�Ʈ ������ ��ٸ� 
    {
        //���� �̺�Ʈ üũ
        if (WaitForSingleObject(m_hExit, 0) == WAIT_OBJECT_0)
        {
            break;
        }

        fd_set readfds;                  // ������ ���� ��� ����
        FD_ZERO(&readfds);              // ��� �ʱ�ȭ
        FD_SET(m_sock, &readfds);       // ������ ���� ���

        timeval timeout{};
        timeout.tv_sec = 1;

        /*
        select(): ������ ������ �б�/����/���� ó���� �������� ����, ���ŷ �Լ�
        FD_ISSET�� �ش� ������ ������ �̺�Ʈ�� �����ƴ��� Ȯ��
        select�� ���� ������ ���� �غ� ���� ���� ó��, cpu�� ���� ���� ����
        */
        int result = select(0, &readfds, nullptr, nullptr, &timeout);
        if (result > 0 && FD_ISSET(m_sock, &readfds))   //�����Ǹ� Ŭ���̾�Ʈ ���� ��û�� �ִٴ� ��
        {
            SOCKET clientSock = accept(m_sock, nullptr, nullptr);
            if (clientSock != INVALID_SOCKET)
            {
                CString sMsg;
                sMsg.Format(_T("[Server] Ŭ���̾�Ʈ ���� �Ϸ� (����: %d)\n"), clientSock);
                UseCallback(NETWORK_EVENT::ACCEPT, PACKET{}, clientSock, sMsg);

                std::thread sendThread(&Server::SendThread, this, clientSock);
                sendThread.detach();

                std::thread recvThread(&Server::RecvThread, this, clientSock);
                recvThread.detach();
            }
            else
            {
                UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, _T("[Server] Accept() ����\n"));
            }
        }
        else if (result == SOCKET_ERROR)
        {
            UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, m_sock, _T("[Server] Accept select() ����\n"));
            break; // ���� �߻� �� ���� Ż��
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
            sValue.Format(_T("[Server] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }
        // select �غ�
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1�� ���

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Server] SEND ����: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(sock, &writeSet))
        {
            const int iResult = Send();
            if (iResult == 0)
            {
                OutputDebugString(_T("[Server] Send �� 0�̹Ƿ� Exit �̺�Ʈ �ߵ�\n"));
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
        SOCKET targetSock = packet.sock;  // ��Ŷ ���� �ִ� ��Ȯ�� ���� ���

        if (targetSock != INVALID_SOCKET)
        {
            iSend = send(targetSock, packet.pszData, packet.uiSize, 0);

            if (iSend > 0)
                sValue.Format(_T("[Server][SEND] %d ���� %d ����Ʈ\n"), targetSock, iSend);
            else if (iSend == 0)
                sValue.Format(_T("[Server][SEND] %d ���� ���� ���� (������ ���� ����)\n"), targetSock);
            else
            {
                sValue.Format(_T("[Server][SEND] %d ���� �����ڵ� %d\n"), targetSock, WSAGetLastError());
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
            sValue.Format(_T("[Server] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        // select �غ�
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 1;

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Server] Recv ����: select (%d)"), WSAGetLastError());
            UseCallback(NETWORK_EVENT::NE_ERROR, PACKET{}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

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
        sValue.Format(_T("[Server][RECV] %d ���� %d ����Ʈ\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Server][RECV] %d ���� ���� ���� (������ ���� ����)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Server][RECV] %d ���� �����ڵ� %d\n"), sock, WSAGetLastError());
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
