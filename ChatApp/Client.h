#pragma once
#include "TCPSocket.h"
class Client : public TCPSocket
{
public:
	Client();
	~Client();

public:
	bool StartClient();

protected:
	//Socket
	bool MakeNonBlockingSocket() override;

	//Connect
	static bool ConnectThread(Client* pClient);
	bool ConnectProc();
	bool Connect();

	//Send
	static bool SendThread(Client* pClient);
	bool SendProc();

private:
	sockaddr_in m_serverAddr{}; //���� ��� ������ sockaddr_in
};

