#pragma once
#include "TCPSocket.h"
class ChatClient : public TCPSocket
{
public:
	ChatClient();
	~ChatClient();

public:
	bool MakeNonBlockingSocket(CString sServerIP, UINT uiPort);
	bool Connect(CString sServerIP, UINT uiPort);

public:

};

