
// ChatAppDlg.h: 헤더 파일
//
#include "Server.h"
#include "Client.h"
#pragma once


// CChatAppDlg 대화 상자
class CChatAppDlg : public CDialogEx
{
// 생성입니다.
public:
	CChatAppDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	static CChatAppDlg* m_pInstance;
	void InitInstance() { m_pInstance = this; }
public:
	Server m_server;
	Client m_client;
	CString m_sChat;

public:
	void Init();
	void AddChat(CString sChat);
	static void CallBackHandler(NETWORK_EVENT, PACKET, int, CString);

public:
	afx_msg void OnBnClickedButtonServer();
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnBnClickedButtonSend();

private:
	//윈도우 컨트롤
	CIPAddressCtrl m_ipServer;
	CEdit m_edPort;
	CListBox m_lstChat;
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
};
