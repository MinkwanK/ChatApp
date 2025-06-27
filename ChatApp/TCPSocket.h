#pragma once

/*
 *
WSANOTINITIALISED (10093)	WSAStartup() ȣ�� �� ��
WSAENETDOWN (10050)			��Ʈ��ũ ����ý��� �ٿ�
WSAEINVAL (10022)			�߸��� �Ķ���� (fd_set�� �߸��� ���� ���� ��)
WSAEFAULT (10014)			�߸��� �޸� ����
WSAEINTR (10004)			���� ȣ���� ��ҵ�
WSAENOTSOCK (10038)			SOCKET �ڵ��� �ƴ�

 */
enum class NETWORK_EVENT
{
	Connect = 0,
	Disconnect,
	Send,
	Recv,
	Error,
};

typedef struct PACKET
{
	char* pszData;
	UINT uiSize;
}PACKET, *PACKET_PTR;

class TCPSocket
{
public:
	TCPSocket();
	virtual ~TCPSocket();

public:
	void SetIPPort(const CString& sIP, const UINT uiPort) { m_sIP = sIP; m_uiPort = uiPort; }
	void SetCallback(void (*pCallback)(NETWORK_EVENT, int, CString)) { m_fCallback = pCallback; }
	int GetSocket() const { return m_sock; }
	void SetKey(const int iKey) { m_iKey = iKey; }
	int GetKey() const { return m_iKey; }
	void AddSend(PACKET packet);

protected:
	bool InitWinSocket();
	void CloseWinSocket();
	void CloseSocket();
	void Clear();		//���ҽ� ����
	virtual bool MakeNonBlockingSocket() = NULL;
	bool RemoveSend(int iIndex);
	void Send();
	void SetInit(const bool bInit) { m_bInit = bInit; }
	bool GetInit() const { return m_bInit; }

protected:
	CString m_sIP;	
	UINT m_uiPort = -1;

	bool m_bExit = false;
	CArray<PACKET> m_aSend;
	void (*m_fCallback)(NETWORK_EVENT, int, CString);	//�ݹ� �Լ� ������

	//Exit
	bool GetExit() const { return m_bExit; };
	void SetExit(const bool bExit) { m_bExit = bExit; }

protected:
	SOCKET m_sock;
	int m_iKey;
	bool m_bInit = false;
	CRITICAL_SECTION m_cs;


};

