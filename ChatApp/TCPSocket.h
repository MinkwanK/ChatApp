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
	void SetCallback(void (*pCallback)(CString)) { m_fCallback = pCallback; }

public:
	SOCKET m_sock;
	bool m_bExit = false;
	CArray<CString> m_aSend;
	void (*m_fCallback)(CString);	//콜백 함수 포인터
};

