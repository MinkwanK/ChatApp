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

}

bool Client::MakeNonBlockingSocket()
{
    CString sValue;

    // 1. ���� �ʱ�ȭ
    CloseSocket();

    // 2. ���� ����
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) {
        OutputDebugString(_T("Socket creation failed.\n"));
        return false;
    }

    // 3. ����ŷ ��� ����
    u_long mode = 1;
    if (ioctlsocket(m_sock, FIONBIO, &mode) != 0) 
    {
        OutputDebugString(_T("ioctlsocket failed\n"));
        Clear();
        return false;
    }

    // 4. ���� �ּ� ����;
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
    if (m_bStop)
        return false;

    RemoveAllSend();
    if (!MakeNonBlockingSocket())
    {
        OutputDebugString(_T("Ŭ���̾�Ʈ StartClient ���� ���� ���� ����\n"));
    }

    std::thread ConnectThread(&Client::ConnectThread, this);
    ConnectThread.detach();
    return TRUE;
}

void Client::RestartClient()    //���� �ʿ�
{
    if (m_bStop)
        return;

    CloseSocket();
    StartClient();
}

void Client::Disconnect()
{
    //if (m_sock != INVALID_SOCKET)
    //{
    //    //�۽� ���� �˸� (FIN ��Ŷ ����)
    //    shutdown(m_sock, SD_SEND);
    //    OutputDebugString(_T("[Client] shutdown(SD_SEND) ȣ��� - FIN �۽�\n"));
    //}

    StopThread();
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
    OutputDebugString(_T("Connect ����\n"));
    if (m_sock == INVALID_SOCKET)
        return false;

    if (!Connect())
    {
        return false;
    }
    CString sValue;

    while (!m_bStop)
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
                m_fcbCommon(NETWORK_EVENT::CONNECT, {}, m_sock, sValue);
            break;
        }
        else 
        {
            if (iResult == 0)
            {
                sValue.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient CONNECT Ÿ�Ӿƿ�\n"));
                OutputDebugString(sValue);
                //if (m_fcbCommon)
                //    m_fcbCommon(NETWORK_EVENT::CONNECT_FAILED, {}, m_sock, sValue);
                //    return false;
                
            }
            else if (iResult == SOCKET_ERROR)
            {
                int iError = WSAGetLastError();
                CString sMsg;
                sMsg.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient - select() ���� - WSAGetLastError(): %d\n"), iError);
                OutputDebugString(sMsg);

                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::CONNECT_FAILED, {}, m_sock, sValue);
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
	//�񵿱� ����
    const int result = connect(m_sock, reinterpret_cast<SOCKADDR*>(&m_serverAddr), sizeof(m_serverAddr));
    if (result == SOCKET_ERROR)
    {
        int iError = WSAGetLastError();
        if (iError != WSAEWOULDBLOCK && iError != WSAEINPROGRESS && iError != WSAEINVAL && iError != WSAEALREADY)  
            //WSAEALREADY �񵿱� ���� �̹� �õ� ��...
        {
            CString sValue;
            sValue.Format(_T("[Ŭ���̾�Ʈ] ����: StartClient CONNECT �õ� �� �����߻� : %d\n"),iError);
            UseCallback(NETWORK_EVENT::NE_ERROR, {}, m_sock, sValue);
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
    while (!m_bStop)
    {
        DWORD dwResult = WaitForSingleObject(m_hStop, 100);
        if (dwResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            if(m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }
        // select �غ�
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5�� ���

        int iSelectResult = select(0, nullptr, &writeSet, nullptr, &timeout);  
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] SEND ����: select (%d)"), WSAGetLastError());
            if(m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(sock, &writeSet))
        {
            int iResult = Send();
            if (iResult == 0)
            {
                SetEvent(m_hStop);
                break;
            }
        }
    }
    SetEvent(m_hSendThread);
}

int Client::Send()
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
                 sValue.Format(_T("[Client][SEND] %d ���� %d ����Ʈ\n"), targetSock, iSend);
            else if (iSend == 0)
                sValue.Format(_T("[Client][SEND] %d ���� ���� ���� (������ ���� ����)\n"), targetSock);
            else
            {
                sValue.Format(_T("[Client][SEND] %d ���� �����ڵ� %d\n"), targetSock, WSAGetLastError());
                return false;
            }
            
            UseCallback(NETWORK_EVENT::SEND, packet, targetSock, sValue);
        }
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
    while (!m_bStop)
    {
        DWORD dwResult = WaitForSingleObject(m_hStop, 100);
        if (dwResult == WAIT_OBJECT_0)
        {
            break;
        }

        if (sock == INVALID_SOCKET)
        {
            sValue.Format(_T("[Client] ����: ��ȿ���� ���� ���� (%d)"), WSAGetLastError());
            if(m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        // select �غ�
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sock, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 5000; // 0.5�� ���

        int iSelectResult = select(0, &readSet, nullptr, nullptr, &timeout);
        if (iSelectResult == SOCKET_ERROR)
        {
            sValue.Format(_T("[Client] Recv ����: select (%d)"), WSAGetLastError());
            if(m_fcbCommon)
                m_fcbCommon(NETWORK_EVENT::NE_ERROR, {}, sock, sValue);
            break;
        }

        if (iSelectResult == 0) continue; // Ÿ�Ӿƿ�, ���� ����

        if (FD_ISSET(sock, &readSet))
        {
           int iResult = Read(sock);
            if (iResult < 0)
            {
                SetEvent(m_hStop);
                break;
            }
            else if (iResult == 0) //���� ����
            {
                sValue.Format(_T("[Client][RECV] %d ���� ���� ���� (������ ���� ����)\n"), sock);
                if (m_fcbCommon)
                    m_fcbCommon(NETWORK_EVENT::DISCONNECT, {}, sock, sValue);
                m_bStop = TRUE;
                SetEvent(m_hStop);
                break;
            }
        }
    }
    SetEvent(m_hRecvThread);
}

int Client::Read(SOCKET sock)
{
    CString sValue;
    char buf[MAX_BUF];
    int iRecv = recv(sock, buf, MAX_BUF, 0);

    if (iRecv > 0)
    {
        sValue.Format(_T("[Client][RECV] %d ���� %d ����Ʈ\n"), sock, iRecv);
    }
    else if (iRecv == 0)
    {
        sValue.Format(_T("[Client][RECV] %d ���� ���� ���� (������ ���� ����)\n"), sock);
    }
    else
    {
        sValue.Format(_T("[Client][RECV] %d ���� �����ڵ� %d\n"), sock, WSAGetLastError());
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
