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
}

TCPSocket::~TCPSocket()
{
	RemoveAllSend();
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
}

void TCPSocket::Send()
{
	CString sValue;
	const int iSend = m_aSend.GetCount();
	if (iSend > 0)
	{
		PACKET packet = m_aSend.GetAt(0);
		int iSendSize = send(m_sock, packet.pszData, packet.uiSize, 0);

		if (iSendSize > 0)
		{
			sValue.Format(_T("[Client][Send] %d 소켓 %d 바이트\n"), m_sock, iSendSize);
		}
		else
		{
			sValue.Format(_T("[Client][Send] %d 소켓 실패코드 %d\n"), m_sock, WSAGetLastError());
		}

		if (m_fcbCommon) m_fcbCommon(NETWORK_EVENT::Send, packet, m_sock, sValue);
		RemoveSend(0);
	}
}

void TCPSocket::Read()
{
	CString sValue;
	char buf[MAX_BUF];
	int iRecv = recv(m_sock, buf, MAX_BUF, 0);

	if (iRecv > 0)
	{
		sValue.Format(_T("[Client][Recv] %d 소켓 %d 바이트\n"), m_sock, iRecv);
	}
	else
	{
		sValue.Format(_T("[Client][Recv] %d 소켓 실패코드 %d\n"), m_sock, WSAGetLastError());
	}

	PACKET packet;
	memset(&packet, 0x00, sizeof(PACKET));

	if (iRecv > 0)
	{
		packet.pszData = buf;
		packet.uiSize = iRecv;
		UseCallback(NETWORK_EVENT::Recv, packet, m_sock, sValue);
	}
	else
	{
		UseCallback(NETWORK_EVENT::Error, {}, m_sock, sValue);
	}
}


void TCPSocket::UseCallback(const NETWORK_EVENT eEvent, const PACKET packet, const int iSocket, const CString& sValue) const
{
	if (m_fcbCommon)
	{
		OutputDebugString(sValue);
		m_fcbCommon(eEvent, packet, iSocket, sValue);
	}
}
