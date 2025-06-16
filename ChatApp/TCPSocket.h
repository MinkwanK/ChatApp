#pragma once
class TCPSocket
{
public:
	TCPSocket();
	~TCPSocket();

	bool InitWinSocket();
	void CloseWinSocket();
	void CloseSocket();
	void Close();

public:
	SOCKET m_sock;
	bool m_exit = false;
};

