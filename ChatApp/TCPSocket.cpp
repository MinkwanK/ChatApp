#include "pch.h"
#include "TCPSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;

#endif

TCPSocket::TCPSocket()
{
	InitWinSocket();
	InitializeCriticalSection(&m_cs);
	m_hExit = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

TCPSocket::~TCPSocket()
{
	RemoveAllSend();
	Clear();
	DeleteCriticalSection(&m_cs); // «ÿ¡¶
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
	m_bExitProccess = TRUE;
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
	bool bResult = false;
	if (iIndex < m_aSend.GetSize())
	{
		PACKET packet = m_aSend.GetAt(iIndex);

		m_aSend.RemoveAt(iIndex);
		if (packet.pszData)
		{
			delete[] packet.pszData;
			packet.pszData = nullptr;
		}
		bResult = true;
	}
	LeaveCriticalSection(&m_cs);
	return bResult;

}

void TCPSocket::RemoveAllSend()
{
	EnterCriticalSection(&m_cs);
	for (int i = 0; i < m_aSend.GetCount(); i++)
	{
		PACKET packet = m_aSend.GetAt(i);
		m_aSend.RemoveAt(i);

		if (packet.pszData)
		{
			delete[] packet.pszData;
			packet.pszData = nullptr;
		}
	}
	LeaveCriticalSection(&m_cs);
}

void TCPSocket::UseCallback(const NETWORK_EVENT eEvent, const PACKET packet, const int iSocket, const CString& sValue) const
{
	if (m_fcbCommon)
	{
		OutputDebugString(sValue);
		m_fcbCommon(eEvent, packet, iSocket, sValue);
	}
}
