#pragma once
#include "TCPSocket.h"
#include "ChatServer.h"

class ChatServer : public TCPSocket
{
public:
	ChatServer();
	~ChatServer();

public:
	bool MakeNonBlockingSocket();
	bool StartServer();
	bool Listen();
	static bool Accept(ChatServer* pServer);
	bool Acceptproc();
	bool GetAccept() { return m_accepting; };
	bool GetExit() { return m_exit; };
	void SetExit(bool bExit) { m_exit = bExit; }
	
public:
	bool m_accepting = FALSE;
};

