#include "pch.h"
#include "TCPSocket.h"
#include "ComonDefine.h"
#include "ComonUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

TCPSocket::TCPSocket()
{
	InitWinSocket();
    InitializeCriticalSection(&m_cs);
}

TCPSocket::~TCPSocket()
{
	Clear();
    DeleteCriticalSection(&m_cs); // 해제
}

bool TCPSocket::InitWinSocket()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	return result == 0;
}

void TCPSocket::CloseWinSocket()
{
	WSACleanup();
}

void TCPSocket::CloseSocket()
{
	if (m_sock)
	{
		closesocket(m_sock);
		m_sock = NULL;
	}
}

void TCPSocket::Clear()
{
    m_bExit = TRUE;
	CloseSocket();
	CloseWinSocket();
}

void TCPSocket::AddSend(PACKET packet)
{
    EnterCriticalSection(&m_cs);
	m_aSend.Add(packet);
    LeaveCriticalSection(&m_cs);
}

bool TCPSocket::RemoveSend(int iIndex)
{
    EnterCriticalSection(&m_cs);
	if (iIndex < m_aSend.GetSize())
	{
        PACKET packet = m_aSend.GetAt(iIndex);

		m_aSend.RemoveAt(iIndex);
        if (packet.pszData)
        {
            delete[] packet.pszData;
            packet.pszData = nullptr;
        }
		return true;
	}
	return false;
    LeaveCriticalSection(&m_cs);
}

void TCPSocket::Send()
{
    CString sValue;
    const int iSendCount = m_aSend.GetCount();
    if (iSendCount > 0)
    {
        for (int i = 0; i < iSendCount; i++)
        {
            PACKET packet = m_aSend.GetAt(i);
            if (packet.pszData && packet.uiSize > 0)
            {
                int iSendSize = send(m_sock, packet.pszData, packet.uiSize, 0);

                if (iSendSize > 0)
                {
                    sValue.Format(_T("[Client] %d 소켓 %d 바이트 Send 성공"), m_sock, iSendSize);
                }
                else
                {
                    sValue.Format(_T("[Client] %d 소켓 Send 실패 (코드 %d)"), m_sock, WSAGetLastError());
                }

                if (m_fCallback) m_fCallback(NETWORK_EVENT::Send, m_sock, sValue);
            }

            if (RemoveSend(i)) i -= 1;
        }
    }
}
