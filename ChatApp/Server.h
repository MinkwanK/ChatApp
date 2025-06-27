#pragma once
#include "TCPSocket.h"
#include "Server.h"

class Server : public TCPSocket
{
public:
	Server();
	~Server();
public:
	bool StartServer(UINT uiPort = -1);
protected:
	//Socket
	bool MakeNonBlockingSocket() override;

	//Listen & Accept
	bool Listen();
	static bool Accept(Server* pServer);
	bool Acceptproc();
	bool GetAccept() { return m_accepting; };

private:
	bool m_accepting = FALSE;
};

