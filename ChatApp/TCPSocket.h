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
	CONNECT = 0,
	DISCONNECT,
	LISTEN,
	ACCEPT,
	SEND,
	RECV,
	NE_ERROR,
};

typedef struct PACKET
{
	char* pszData;
	int iOPCode;
	UINT uiSize;
}PACKET, * PACKET_PTR;

constexpr int MAX_BUF = 1024;
using CommonCallback = void(*)(NETWORK_EVENT, PACKET, int, CString);
using SendCallback = void(*)(NETWORK_EVENT, int, CString, PACKET);

class TCPSocket
{
public:
	TCPSocket();
	virtual ~TCPSocket();

public:
	void SetIPPort(const CString& sIP, const UINT uiPort) { m_sIP = sIP; m_uiPort = uiPort; }
	void SetPort(const UINT uiPort) { m_uiPort = uiPort; }
	void SetCallback(CommonCallback pCb) { m_fcbCommon = pCb; }
	void SetSendCallback(SendCallback pCb) { m_fcbSend = pCb; }
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
	void RemoveAllSend();
	virtual void SendProc(SOCKET sock) = NULL;
	virtual void RecvProc(SOCKET sock) = NULL;
	virtual int Send(SOCKET sock) = NULL;
	virtual int Read(SOCKET sock) = NULL;
	void SetInit(const bool bInit) { m_bInit = bInit; }
	bool GetInit() const { return m_bInit; }
	void UseCallback(NETWORK_EVENT eEvent, PACKET packet, int iSocket, const CString& sValue) const;

protected:
	CString m_sIP;
	UINT m_uiPort = -1;
	CArray<PACKET> m_aSend;
	CommonCallback m_fcbCommon;
	SendCallback m_fcbSend;

	//Exit
	bool m_bExit = false;
	bool GetExit() const { return m_bExit; };
	void SetExit(const bool bExit) { m_bExit = bExit; }

protected:
	SOCKET m_sock;
	int m_iKey;
	bool m_bInit = false;
	CRITICAL_SECTION m_cs;
};

