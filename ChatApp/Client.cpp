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
        Clear();
        return false;
    }

    // 3. ���� �ּ� ����;
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_uiPort);
    if (InetPton(AF_INET, m_sIP, &m_serverAddr.sin_addr) != 1)    //InetPton�� ���ڿ� IP�� ���� IP�� �����ϰ� ��ȯ
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
        OutputDebugString(_T("Ŭ���̾�Ʈ StartClient ���� ���� ���� ����\n"));
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
        timeout.tv_sec = 1; // �ִ� 1�� ���
        timeout.tv_usec = 0;

        const int iResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iResult > 0 && FD_ISSET(m_sock, &writeSet)) //�����Ǹ� Ŭ���̾�Ʈ ���� ����
        {
            sValue.Format(_T("[Ŭ���̾�Ʈ] ����: %d ����\n"), m_sock);
            OutputDebugString(sValue);
            if (m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::Connect, {}, m_sock, sValue);
            break;
        }
        else
        {
            if (iResult == 0)
            {
                sValue.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient Connect Ÿ�Ӿƿ�\n"));
                OutputDebugString(sValue);
                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);
                if (!Connect())
                {
                    return false;
                }
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();

                CString sMsg;
                sMsg.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient - select() ���� - WSAGetLastError(): %d\n"), iError);
                OutputDebugString(sMsg);

                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);

                CloseSocket();
                return false;
            }
        }
    }

    std::thread SendThread(&Client::SendThread, this);
    SendThread.detach();

    std::thread RecvThread(&Client::RecvThread, this);
    RecvThread.detach();

    return true;
}

bool Client::Connect()
{
    //�񵿱� ����
    const int result = connect(m_sock, reinterpret_cast<SOCKADDR*>(&m_serverAddr), sizeof(m_serverAddr));
    if (result == SOCKET_ERROR)
    {
        int iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS && iError != WSAEINVAL && iError != WSAEALREADY)   //WSAEALREADY �񵿱� ���� �̹� �õ� ��...
        {
            CString sValue;
            sValue.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient Connect �õ� �� �����߻� : %d\n"), iError);
            UseCallback(NETWORK_EVENT::Error, {}, m_sock, sValue);
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
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);
            break;
        }
        // select �غ�
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1�� ���

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Send ����: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(m_sock, &writeSet))
        {
            Send();
        }
    }
    return false;
}

bool Client::RecvThread(Client* pClient)
{
    if (pClient)
    {
        return pClient->RecvProc();
    }
    return false;
}

bool Client::RecvProc()
{
    CString sValue;
    while (!m_bExit)
    {
        if (m_sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);
            break;
        }
        // select �غ�
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 1000 * 1000; // 1�� ���

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Recv ����: select (%d)"), WSAGetLastError());
            m_fcbCommon(NETWORK_EVENT::Error, {}, m_sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(m_sock, &readSet))
        {
            Read();
        }
    }
    return false;
}
