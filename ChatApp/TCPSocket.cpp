#include "pch.h"
#include "TCPSocket.h"

TCPSocket::TCPSocket()
{
	InitWinSocket();
}

TCPSocket::~TCPSocket()
{
	Close();
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

void TCPSocket::Close()
{
	CloseSocket();
	CloseWinSocket();
}

void TCPSocket::AddSend(CString sSend)
{
	m_aSend.Add(sSend);
}

bool TCPSocket::RemoveSend(int iIndex)
{
	if (iIndex < m_aSend.GetSize())
	{
		m_aSend.RemoveAt(iIndex);
		return true;
	}
	return false;
}
