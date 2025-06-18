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
	static bool ConnectThread(ChatClient* pClient);
	bool ConnectProc();

public:
	CString m_sServerIP;
	UINT m_uiPort;
};

