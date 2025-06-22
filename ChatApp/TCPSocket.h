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
	void AddSend(CString sSend);
	bool RemoveSend(int iIndex);

public:
	SOCKET m_sock;
	bool m_bExit = false;
	CArray<CString> m_aSend;
};

