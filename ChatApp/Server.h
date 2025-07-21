#pragma once
#include "TCPSocket.h"
#include "Server.h"

class Server : public TCPSocket
{
public:
	Server();
	~Server();
public:
	bool StartServer();
protected:
	//Socket
	bool MakeNonBlockingSocket() override;

	//Listen & Accept
	bool Listen();
	static bool Accept(Server* pServer);
	bool Acceptproc();
	bool GetAccept() { return m_accepting; }

	//SEND
	static void SendThread(Server* pServer, SOCKET sock);
	void SendProc(SOCKET sock) override;
	int Send() override;

	//RECV
	static void RecvThread(Server* pServer, SOCKET sock);
	void RecvProc(SOCKET sock) override;
	int Read(SOCKET sock) override;

private:
	bool m_accepting = FALSE;
};

