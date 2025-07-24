
// ChatAppDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ChatApp.h"
#include "ChatAppDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//실제 static 변수의 메모리 할당
CChatAppDlg* CChatAppDlg::m_pInstance = nullptr;
// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatAppDlg 대화 상자



CChatAppDlg::CChatAppDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATAPP_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	InitInstance();
}

void CChatAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ipServer);
	DDX_Control(pDX, IDC_EDIT_PORT, m_edPort);
	DDX_Control(pDX, IDC_LIST_CHAT, m_lstChat);
}

BEGIN_MESSAGE_MAP(CChatAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SERVER, &CChatAppDlg::OnBnClickedButtonServer)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CChatAppDlg::OnBnClickedButtonConnect)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CChatAppDlg::OnBnClickedButtonSend)
END_MESSAGE_MAP()


// CChatAppDlg 메시지 처리기

BOOL CChatAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	Init();
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CChatAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CChatAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CChatAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CChatAppDlg::Init()
{
	m_ipServer.SetWindowText(_T("127.0.0.1"));
	m_edPort.SetWindowText(_T("9000"));

	m_server.SetCallback(CallBackHandler);
	m_client.SetCallback(CallBackHandler);
}

void CChatAppDlg::AddChat(CString sChat)
{
	if (m_lstChat && ::IsWindow(m_lstChat.GetSafeHwnd()))
	{
		// 100개 이상이면 가장 오래된 항목 삭제
		if (m_lstChat.GetCount() >= 100)
		{
			m_lstChat.DeleteString(m_lstChat.GetCount() - 1); // 가장 아래 항목 삭제
		}

		// 새 채팅을 맨 위에 추가
		m_lstChat.InsertString(0, sChat);
	}
}

//static 함수는 클래스 안에 있어도 일반 함수 취급
//메모리 상에 독립적으로 존재
//일반 함수 포인터에 그대로 대입 가능
void CChatAppDlg::CallBackHandler(NETWORK_EVENT eEvent, PACKET packet, int iSocket, CString sValue)
{
	if (m_pInstance)
	{
		switch (eEvent)
		{
		case NETWORK_EVENT::SEND:
		{

		}break;
		case NETWORK_EVENT::RECV:
		{
			if (packet.pszData && packet.uiSize > 0)
			{
				sValue.Format(_T("%s"), packet.pszData);
			}
		}break;
		}
		m_pInstance->AddChat(sValue);
	}
}

void CChatAppDlg::OnBnClickedButtonServer()
{
	if (m_server.GetRunning())
	{
		m_server.SetExit();
		SetDlgItemText(IDC_BUTTON_SERVER, _T("방 생성"));
		GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(TRUE);
	}
	else
	{
		CString sServerIP, sServerPort;
		m_ipServer.GetWindowText(sServerIP);
		m_edPort.GetWindowText(sServerPort);
		m_server.SetIPPort(sServerIP, _ttoi(sServerPort));
		if (m_server.StartServer())
		{
			SetDlgItemText(IDC_BUTTON_SERVER, _T("사용자 입장 대기 중... (대기취소)"));
			AddChat(CString(_T("사용자 입장 대기 중... (대기취소)")));
			GetDlgItem(IDC_BUTTON_CONNECT)->EnableWindow(FALSE);
		}
	}
}


void CChatAppDlg::OnBnClickedButtonConnect()
{
	if (m_client.GetRunning())
	{
		m_client.SetExit();
		SetDlgItemText(IDC_BUTTON_CONNECT, _T("방 입장"));
		GetDlgItem(IDC_BUTTON_SERVER)->EnableWindow(TRUE);
	}
	else
	{
		CString sServerIP, sServerPort;
		m_ipServer.GetWindowText(sServerIP);
		m_edPort.GetWindowText(sServerPort);
		m_client.SetIPPort(sServerIP, _ttoi(sServerPort));
		if (m_client.StartClient())
		{
			AddChat(CString(_T("클라이언트 접속 중...")));
			SetDlgItemText(IDC_BUTTON_CONNECT, _T("접속 중....(취소)"));
			GetDlgItem(IDC_BUTTON_SERVER)->EnableWindow(FALSE);
		}
	}
}


void CChatAppDlg::OnBnClickedButtonSend()
{
	CString sSend;
	GetDlgItemText(IDC_EDIT_INPUT, sSend);

	PACKET packet;
	packet.uiSize = sSend.GetLength() + 1;
	packet.pszData = new char[sSend.GetLength() + 1];
	packet.sock = m_client.GetSocket();
	ZeroMemory(packet.pszData, packet.uiSize);
	memcpy_s(packet.pszData, packet.uiSize, sSend, sSend.GetLength());

	m_client.AddSend(packet);
	sSend.Format(_T("나: %s"), packet.pszData);
	AddChat(sSend);
	SetDlgItemText(IDC_EDIT_INPUT, _T(""));
}


BOOL CChatAppDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
	{
		OnBnClickedButtonSend();
		return TRUE; // 여기서 TRUE를 반환하면 기본 처리를 막음
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}
