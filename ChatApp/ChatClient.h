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

	static bool SendThread(ChatClient* pClient);
	bool SendProc();

public:
	CString m_sServerIP;
	UINT m_uiPort;
	CArray<CString> m_aSend;
};

