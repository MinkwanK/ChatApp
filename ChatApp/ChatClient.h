#pragma once
#include "TCPSocket.h"
class ChatClient : public TCPSocket
{
public:
	ChatClient();
	~ChatClient();

public:
	bool MakeNonBlockingSocket(CString sIP, int iPort);

public:

};

