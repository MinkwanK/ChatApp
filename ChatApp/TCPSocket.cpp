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
