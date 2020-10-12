
// YouMeDemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "YouMeDemo.h"
#include "YouMeDemoDlg.h"
#include "afxdialogex.h"
#include <sstream>
#include <vector>
#include <map>

extern "C" {
	_declspec(dllexport) void youme_setTestConfig(int bTest);
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_YOUME_ABOUT };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CYouMeDemoDlg 对话框



CYouMeDemoDlg::CYouMeDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CYouMeDemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CYouMeDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_VOL, m_sliderVol);
	DDX_Control(pDX, IDC_CMB_SERVER, m_ServerList);
	DDX_Control(pDX, IDC_CMB_USER_LIST, m_UserList);
	DDX_Control(pDX, IDC_CHK_USERLIST, m_needUserList);
	DDX_Control(pDX, IDC_CHK_MIC, m_needMic);
	DDX_Control(pDX, IDC_CHK_SPEAKER, m_needSpeaker);
	DDX_Control(pDX, IDC_CHK_AUTO_STATUS, m_needAutoStatus);
	DDX_Control(pDX, IDC_STATIC_VOL, m_setVol);
	DDX_Control(pDX, IDC_STR_CURR_VOL, m_getVol);
	DDX_Control(pDX, IDC_STR_SDK_VERSION, m_sdkVersion);
	DDX_Control(pDX, IDC_EDIT_CURRENTSTATUS, m_currStatus);
}

BEGIN_MESSAGE_MAP(CYouMeDemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_LEAVEROOM, &CYouMeDemoDlg::OnBnClickedBtnLeaveroom)
	ON_BN_CLICKED(IDC_BTN_JOINROOM, &CYouMeDemoDlg::OnBnClickedBtnJoinroom)
	ON_BN_CLICKED(IDC_BTN_INIT, &CYouMeDemoDlg::OnBnClickedBtnInit)
	ON_MESSAGE(UM_CONTROL, &CYouMeDemoDlg::OnInterfaceEvent)
	//ON_MESSAGE(CALL_CONTROL, &CYouMeDemoDlg::OnControlCallMessage)
	ON_BN_CLICKED(IDC_BTN_UNINIT, &CYouMeDemoDlg::OnBnClickedBtnUninit)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_VOL, &CYouMeDemoDlg::OnNMCustomdrawSliderVol)
	ON_BN_CLICKED(IDC_BTN_CLOSE_MIC, &CYouMeDemoDlg::OnBnClickedBtnCloseMic)
	ON_BN_CLICKED(IDC_BTN_GET_MIC, &CYouMeDemoDlg::OnBnClickedBtnGetMic)
	ON_BN_CLICKED(IDC_BTN_OPEN_MIC, &CYouMeDemoDlg::OnBnClickedBtnOpenMic)
	ON_BN_CLICKED(IDC_BTN_GET_SPEAKER, &CYouMeDemoDlg::OnBnClickedBtnGetSpeaker)
	ON_BN_CLICKED(IDC_BTN_OPEN_SPEAKER, &CYouMeDemoDlg::OnBnClickedBtnOpenSpeaker)
	ON_BN_CLICKED(IDC_BTN_CLOSE_SPEAKER, &CYouMeDemoDlg::OnBnClickedBtnCloseSpeaker)
	ON_BN_CLICKED(IDC_BTN_OPEN_OTHER_SPEAKER, &CYouMeDemoDlg::OnBnClickedBtnOpenOtherSpeaker)
	ON_BN_CLICKED(IDC_BTN_CLOSE_OTHER_SPEAKER, &CYouMeDemoDlg::OnBnClickedBtnCloseOtherSpeaker)
	ON_BN_CLICKED(IDC_BTN_CLOSE_OTHER_MIC, &CYouMeDemoDlg::OnBnClickedBtnCloseOtherMic)
	ON_BN_CLICKED(IDC_BTN_OPEN_OTHER_MIC, &CYouMeDemoDlg::OnBnClickedBtnOpenOtherMic)
	ON_BN_CLICKED(IDC_BTN_QUIT, &CYouMeDemoDlg::OnBnClickedBtnQuit)
	ON_BN_CLICKED(IDC_BTN_AVOID_OTHERS, &CYouMeDemoDlg::OnBnClickedBtnAvoidOthers)
	ON_CBN_SELCHANGE(IDC_CMB_SERVER, &CYouMeDemoDlg::OnCbnSelchangeCmbServer)
	ON_BN_CLICKED(IDC_BTN_CANCLE_AVOID_OTHER, &CYouMeDemoDlg::OnBnClickedBtnCancleAvoidOther)
	ON_CBN_DROPDOWN(IDC_CMB_SERVER, &CYouMeDemoDlg::OnCbnDropdownCmbServer)
	ON_BN_CLICKED(IDC_BTN_GET_VOL, &CYouMeDemoDlg::OnBnClickedBtnGetVol)
	//ON_MESSAGE(MEMBER_CHANGE, &CYouMeDemoDlg::OnMemberChange)
	//ON_MESSAGE(COMMON_STATUS, &CYouMeDemoDlg::OnCommonStatus)
	ON_BN_CLICKED(IDC_BTN_CLEAR, &CYouMeDemoDlg::OnBnClickedBtnClear)
END_MESSAGE_MAP()


// CYouMeDemoDlg 消息处理程序

BOOL CYouMeDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码
	m_currMessage.Empty();
	m_sliderVol.SetRange(0, 100);
	m_sliderVol.SetPos(100);

	m_ServerList.AddString(_T("选择一个地区"));
	m_UserList.AddString(_T("选择一个用户"));
	m_ServerList.SetCurSel(0);
	m_UserList.SetCurSel(0);

	int ver = -1;
	CString strVersion;
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		ver = m_cConfig.m_pEngine->getSDKVersion();
		
		CString strMsg;
		strMsg.Format(_T("@@@获取当前SDK版本 [%d] "), ver);
		ShowStatus(strMsg);
	}

	strVersion.Format(_T("%d"), ver);
	m_sdkVersion.SetWindowTextA(strVersion);

	isInit = false;
	isInRoom = false;

	m_needMic.SetCheck(true);
	m_needSpeaker.SetCheck(true);

	srand((unsigned)time(NULL));
	std::string userid;
	userid.append("user_").append(std::to_string(1000 + (rand() % 100)));

	SetDlgItemTextA(IDC_EDIT_USERID, userid.c_str());
	SetDlgItemTextA(IDC_EDIT_ROOMID, "1234");
	
	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		initEngine();
	}

	m_YouMeVideoDlg = new YouMeVideoDlg();
	m_YouMeVideoDlg->Create(IDD_YOUME_VIDEO);
	m_YouMeVideoDlg->ShowWindow(SW_SHOWNORMAL);

	int cx = GetSystemMetrics(SM_CXSCREEN);
	int cy = GetSystemMetrics(SM_CYSCREEN);
	::SetWindowPos(m_hWnd, HWND_TOP, 100, 30, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	::SetWindowPos(m_YouMeVideoDlg->m_hWnd, HWND_TOP, 600, 80, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CYouMeDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CYouMeDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CYouMeDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CYouMeDemoDlg::OnBnClickedBtnLeaveroom()
{
	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInRoom) {
			ShowStatus(_T("还没有加入会议，不用退出。"));
			return;
		}

		YouMeErrorCode ret = m_cConfig.m_pEngine->leaveChannelMultiMode(m_strRoomID.c_str());

		CString strMsg;
		strMsg.Format(_T("@@@离开会议，房间ID [ %s ] 返回值 [%d] "), m_strRoomID.c_str(), ret);
		ShowStatus(strMsg);
	}	
}


void CYouMeDemoDlg::OnBnClickedBtnJoinroom()
{
	CString strUserID, strRoomID;
	GetDlgItemText(IDC_EDIT_USERID, strUserID);
	GetDlgItemText(IDC_EDIT_ROOMID, strRoomID);

	if (!isInit) {
		ShowStatus(_T("还没初始化，请初始化完成后加入会议."));
		return;
	}

	if (isInRoom) {
		ShowStatus(_T("已经在会议中了，请先退出当前会议。"));
		return;
	}
	
	if (strUserID.IsEmpty() || strRoomID.IsEmpty())
	{
		ShowStatus(_T("用户名或者房间名不能为空"));
		return;
	}else{
		m_strUserID = strUserID.GetBuffer();
		m_strRoomID = strRoomID.GetBuffer();
		if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
		{
			bool needUserList = false;
			bool needMic = false; 
			bool needSpeaker = false; 
			bool needAutoStatus = false;

			if (m_needUserList.GetCheck() == BST_CHECKED) {
				needUserList = true;
			}

			if (m_needMic.GetCheck() == BST_CHECKED) {
				needMic = true;
			}

			if (m_needSpeaker.GetCheck() == BST_CHECKED) {
				needSpeaker = true;
			}

			if (m_needAutoStatus.GetCheck() == BST_CHECKED) {
				needAutoStatus = true;
			}

			m_cConfig.m_pEngine->setToken("");
			//m_cConfig.m_pEngine->setVideoNetResolution(320, 240);
			m_cConfig.m_pEngine->setAVStatisticInterval(5000);
			m_cConfig.m_pEngine->setAudioQuality(LOW_QUALITY);

			YouMeErrorCode ret = m_cConfig.m_pEngine->joinChannelSingleMode(m_strUserID.c_str(), m_strRoomID.c_str(), YOUME_USER_HOST, CYouMeConfig::strJoinAppKey.c_str());

			CString strMsg;
			strMsg.Format(_T("@@@加入会议, 用户ID [%s], 房间ID [%s], 用户列表 [%d], 麦克风 [%d], 扬声器 [%d], 自动状态 [%d]"), m_strUserID.c_str(), m_strRoomID.c_str(), (int)needUserList, (int)needMic, (int)needSpeaker, (int)needAutoStatus);
			ShowStatus(strMsg);

			strMsg.Empty();
			strMsg.Format(_T("加入会议 返回值 [%d] "), ret);
			ShowStatus(strMsg);
		}		
	}
}

YouMeErrorCode CYouMeDemoDlg::initEngine()
{
	youme_setTestConfig((int)TEST_SERVER);
	m_cConfig.m_pEngine->setExternalInputMode(false);
	m_cConfig.m_pEngine->setAVStatisticInterval(5000);
	m_cConfig.m_pEngine->setVideoNetResolution(240, 320);
	YouMeErrorCode ret = m_cConfig.m_pEngine->init(m_cConfig.m_pEventCallback, m_cConfig.g_strKey.c_str(), m_cConfig.g_strSecret.c_str(), RTC_CN_SERVER, "cn");
	return ret;
}

void CYouMeDemoDlg::OnBnClickedBtnInit()
{
	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (isInit) {
			ShowStatus(_T("已经初始化了，不用再次初始化。"));
			return;
		}

		/** 使用测试服务器 */
		YouMeErrorCode ret = initEngine();
	
	
		CString strMsg;
		strMsg.Format(_T("@@@初始化 返回值 [ %d ]"), ret);
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnUninit()
{
	if (m_cConfig.m_pEngine == NULL) {
		ShowStatus(_T("已经退出了，怎么会反初始化呢。"));
		return;
	}

	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInit) {
			ShowStatus(_T("当前没有初始化，不需要进行反初始化."));
			return;
		}

		if (isInRoom) {
			ShowStatus(_T("正在会议中，请先退出会议。"));
			return;
		}

		YouMeErrorCode ret = m_cConfig.m_pEngine->unInit();

		isInit = false;

		CString strMsg;
		strMsg.Format(_T("@@@反初始化 返回值 [%d]"), ret);
		ShowStatus(strMsg);

	}
}

void CYouMeDemoDlg::OnNMCustomdrawSliderVol(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO:  在此添加控件通知处理程序代码
	int volume = m_sliderVol.GetPos();
	CString strVol;
	strVol.Format(_T("%d"), volume);
	m_setVol.SetWindowTextA(strVol);
	*pResult = 0;

	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法设置音量。"));
			return;
		}
		m_cConfig.m_pEngine->setVolume(volume);
	}

	CString strMsg;
	strMsg.Format(_T("设置音量: %d"), volume);
	ShowStatus(strMsg);
}


void CYouMeDemoDlg::OnBnClickedBtnCloseMic()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法给麦克风静音。"));
			return;
		}

		ShowStatus(_T("[自己] 麦克风静音"));
		m_cConfig.m_pEngine->setMicrophoneMute(true);
	}

}


void CYouMeDemoDlg::OnBnClickedBtnGetMic()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法获取麦克风静音状态。"));
			return;
		}

		bool status = m_cConfig.m_pEngine->getMicrophoneMute();
		CString strMsg;
		strMsg.Format(_T("获取 [自己] 麦克风静音状态:%d"), status);
		ShowStatus(strMsg);

	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenMic()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法取消麦克风静音。"));
			return;
		}

		ShowStatus(_T("[自己] 麦克风取消静音"));
		m_cConfig.m_pEngine->setMicrophoneMute(false);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnGetSpeaker()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法获取当前扬声器状态。"));
			return;
		}

		bool status = m_cConfig.m_pEngine->getSpeakerMute();
		CString strMsg;
		strMsg.Format(_T("获取 [自己] 扬声器状态:%d"), status);
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenSpeaker()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法打开扬声器。"));
			return;
		}

		m_cConfig.m_pEngine->setSpeakerMute(false);
		CString strMsg;
		strMsg.Format(_T("打开 [自己] 扬声器"));
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseSpeaker()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法关闭扬声器。"));
			return;
		}

		ShowStatus(_T("关闭 [自己] 扬声器"));
		m_cConfig.m_pEngine->setSpeakerMute(false);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenOtherSpeaker()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();
		
		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherSpeakerMute(user_id.c_str(), true);

		CString strMsg;
		strMsg.Format(_T("打开[ %s ]扬声器"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseOtherSpeaker()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherSpeakerMute(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("关闭[ %s ]扬声器"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseOtherMic()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherMicMute(user_id.c_str(), true);
		
		CString strMsg;
		strMsg.Format(_T("关闭[ %s ]麦克风"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenOtherMic()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherMicMute(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("打开[ %s ]的麦克风"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnQuit()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.m_pEngine == NULL) {
		ShowStatus(_T("已经退出了，不要重复调用。"));
		return;
	}

	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		ShowStatus(_T("退出，调用销毁API"));
		m_cConfig.m_pEngine->destroy();
		m_cConfig.m_pEngine = NULL;

		isInit = false;
		isInRoom = false;
	}
}



void CYouMeDemoDlg::OnBnClickedBtnAvoidOthers()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setListenOtherVoice(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("开启屏蔽[ %s ]的语音"), user_id.c_str());
		ShowStatus(strMsg);
	}
}

void CYouMeDemoDlg::OnBnClickedBtnCancleAvoidOther()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("选择一个用户") == 0){
			CString strMsg;
			strMsg.Format(_T("请先选择一个用户"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setListenOtherVoice(user_id.c_str(), true);

		CString strMsg;
		strMsg.Format(_T("取消屏蔽[ %s ]的语音"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnCbnSelchangeCmbServer()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_ServerList.GetLBText(m_ServerList.GetCurSel(), cs);
		std::string server = cs.GetBuffer();

		int server_area = RTC_CN_SERVER;

		if (server.compare("中国")) { server_area = RTC_CN_SERVER; }
		else if (server.compare("香港")) { server_area = RTC_HK_SERVER; }
		else if (server.compare("美国")) { server_area = RTC_US_SERVER; }
		else if (server.compare("新加坡")){ server_area = RTC_SG_SERVER; }
		else if (server.compare("韩国")){ server_area = RTC_KR_SERVER; }
		else if (server.compare("澳洲")){ server_area = RTC_AU_SERVER; }
		else if (server.compare("德国")){ server_area = RTC_DE_SERVER; }
		else if (server.compare("巴西")){ server_area = RTC_BR_SERVER; }
		else if (server.compare("印度")){ server_area = RTC_IN_SERVER; }
		else if (server.compare("日本")){ server_area = RTC_JP_SERVER; }
		else if (server.compare("爱尔兰")){ server_area = RTC_IE_SERVER; } 
		else { server_area = RTC_CN_SERVER; }

		m_cConfig.m_pEngine->setServerRegion((YOUME_RTC_SERVER_REGION_t)server_area, "kuangfeng",false);

		CString strMsg;
		strMsg.Format(_T("更换服务器区域 [%s]"), server.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnCbnDropdownCmbServer()
{
	int curSel = m_ServerList.GetCurSel();
	// TODO:  在此添加控件通知处理程序代码
	m_ServerList.ResetContent();
	m_ServerList.AddString("中国");
	m_ServerList.AddString("香港");
	m_ServerList.AddString("美国");
	m_ServerList.AddString("新加坡");
	m_ServerList.AddString("韩国");
	m_ServerList.AddString("澳洲");
	m_ServerList.AddString("德国");
	m_ServerList.AddString("巴西");
	m_ServerList.AddString("印度");
	m_ServerList.AddString("日本");
	m_ServerList.AddString("爱尔兰");
	m_ServerList.SetCurSel(curSel);
}


void CYouMeDemoDlg::OnBnClickedBtnGetVol()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("没有初始化，无法获取当前音量。"));
			return;
		}

		int vol = m_cConfig.m_pEngine->getVolume();
		CString strVol;
		strVol.Format(_T("%d"), vol);
		m_getVol.SetWindowTextA(strVol);

		CString strVolDis;
		strVolDis.Format(_T("获取当前系统音量：%d"), vol);
		ShowStatus(strVolDis);
	}
}

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);

/**
 * 房间成员变化
 */
void CYouMeDemoDlg::OnMemberChange(std::string roomid, std::string userlists)
{
	CString strStatusDis;
	strStatusDis.Format(_T("回调：OnMemberChange RoomID:%s"), roomid.c_str());
	ShowStatus(strStatusDis);


	UpdateData(TRUE);
	std::vector<std::string> vct;
	SplitString(userlists, vct, ",");
	m_UserList.ResetContent();
	
	for (std::vector<std::string>::const_iterator iter = vct.cbegin(); iter != vct.cend(); iter++) {	
		CString strUser;
		strUser.Format(_T("%s"), ((std::string)*iter).c_str());
		m_UserList.AddString(strUser);
	}

	m_UserList.SetCurSel(0);
	strStatusDis.Empty();
	strStatusDis.Format(_T("回调：OnMemberChange 人员ID列表：%s"), userlists.c_str());
	ShowStatus(strStatusDis);
	UpdateData(FALSE);
	UpdateWindow();
}


afx_msg LRESULT CYouMeDemoDlg::OnInterfaceEvent(WPARAM wParam, LPARAM lParam)
{
	std::map<std::string, int> * map1 = (std::map<std::string, int> *)wParam;
	std::map<std::string, std::string> * map2 = (std::map<std::string, std::string> *)lParam;

	auto it1 = map1->find("event");
	auto it2 = map1->find("error");
	auto it3 = map2->find("room");
	auto it4 = map2->find("param");
	YouMeEvent event = (YouMeEvent)it1->second;
	YouMeErrorCode error = (YouMeErrorCode)it2->second;
	std::string room = it3->second;
	std::string param = it4->second;

	CString strStatusDis;
	strStatusDis.Format(_T("回调：OnEvent event: %d, error: %d, room: %s, param: %s"), event, error, room.c_str(), param.c_str());
	ShowStatus(strStatusDis);

	switch (event) {
	case YOUME_EVENT_INIT_OK:			 ///< SDK验证成功
		isInit = true;
		m_cConfig.m_pEngine->setLogLevel(LOG_INFO);
		m_cConfig.m_pEngine->setServerRegion(RTC_CN_SERVER, "", false);


		ShowStatus(_T("初始化成功"));
		break;
	case YOUME_EVENT_INIT_FAILED:           ///< SDK验证失败
		isInit = false;
		ShowStatus(_T("初始化失败"));
		break;
//	case YOUME_EVENT_INCOMING:           ///< 收到来电
//		break;
//	case YOUME_EVENT_INPROGRESS:         ///< 正在呼叫
//		break;
//	case YOUME_EVENT_RINGING:            ///< 远端响铃中
//		break;
	case YOUME_EVENT_JOIN_OK:          ///< 呼叫成功
		isInRoom = true;
		m_sliderVol.SetPos(100);
		m_YouMeVideoDlg->joinRoomSuc(true);
		m_cConfig.m_pEngine->setSpeakerMute(false);
		IYouMeVoiceEngine::getInstance()->setVideoCallback(m_YouMeVideoDlg);
		m_setVol.SetWindowTextA(_T("100"));
		ShowStatus(_T("进入房间成功"));

		break;
//	case YOUME_EVENT_TERMWAITING:        ///< 挂断中，在网络出现问题或者对方挂断时会受到此事件
		break;
	case YOUME_EVENT_LEAVED_ONE:         ///< 已挂断，在网络出现问题或者对方挂断时会受到此事件
		isInRoom = false;
		ShowStatus(_T("离开房间成功"));
		m_YouMeVideoDlg->joinRoomSuc(false);
		break;
	case YOUME_EVENT_JOIN_FAILED:             ///< 呼叫失败
		isInRoom = false;
		ShowStatus(_T("进入房间失败"));
		break;
	case YOUME_EVENT_REC_PERMISSION_STATUS:///< 通知录音失败（此时不管麦克风mute状态如何，都没有声音输出）
		break;
	case YOUME_EVENT_BGM_STOPPED:        ///< 通知背景音乐播放结束
		break;
	case YOUME_EVENT_BGM_FAILED:         ///< 通知背景音乐播放失败
		break;
//	case YOUME_EVENT_PAUSE:              ///< 通知暂停通话是否成功
//		break;
//	case YOUME_EVENT_RESUME:             ///< 通知恢复通话是否成功
//		break;
	case YOUME_EVENT_SPEAK_SUCCESS:      ///< 切换房间通话成功
		break;
	case YOUME_EVENT_SPEAK_FAILED:       ///< 切换房间通话失败
		break;
	/*case YOUME_EVENT_MEMBER_CHANGE:      ///< 房间成员变化
		break;
	case YOUME_EVENT_MIC_NORMAL:         ///< 麦克风正常（打开）
		break;
	case YOUME_EVENT_MIC_MUTE:           ///< 麦克风静音（关闭）
		break;
	case YOUME_EVENT_SPEAKER_NORMAL:     ///< 扬声器正常（打开）
		break;
	case YOUME_EVENT_SPEAKER_MUTE:       ///< 扬声器静音（关闭）
		break;
	case YOUME_EVENT_MIC_CTR_NORMAL:     ///< 麦克风控制正常（打开）
		break;
	case YOUME_EVENT_MIC_CTR_MUTE:       ///< 麦克风控制静音（关闭）
		break;
	case YOUME_EVENT_SPEAKER_CTR_NORMAL: ///< 扬声器控制正常（打开）
		break;
	case YOUME_EVENT_SPEAKER_CTR_MUTE:   ///< 扬声器控制静音（关闭）
		break;
	case YOUME_EVENT_AVOID_OTHER_NORMAL: ///< 屏蔽他人正常（不屏蔽）
		break;
	case YOUME_EVENT_AVOID_OTHER_MUTE:   ///< 屏蔽他人静音（屏蔽）
		break;
	case YOUME_EVENT_FAILED_LEAVE:        ///< 退出房间失败
		break;
		*/
	case YOUME_EVENT_OTHERS_VIDEO_ON:
	{
		int renderId = m_cConfig.m_pEngine->createRender(param.c_str());
		m_YouMeVideoDlg->addRender(renderId);
		break;
	}
	case YOUME_EVENT_OTHERS_VIDEO_INPUT_START:
		break;
	case YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP:
		m_cConfig.m_pEngine->deleteRender(m_YouMeVideoDlg->removeRender(param));
		
		break;
	default:
		break;
	}

	UpdateData(FALSE);
	UpdateWindow();
	return 0;
}



/****
 * 字符串分割
 */
void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
		v.push_back(s.substr(pos1));
}

/**
 * 显示Status
 */
void CYouMeDemoDlg::ShowStatus(CString msg) {
	m_currMessage.Append("[rtc@youme ~]#  ");
	m_currMessage.Append(msg);
	m_currMessage.Append("\r\n");
	m_currStatus.SetWindowTextA(m_currMessage);

	/** 滚动到最新行 */
	int iCount = m_currMessage.GetLength();
	m_currStatus.SetRedraw(FALSE);
	m_currStatus.SetWindowText(m_currMessage);
	int iLine = m_currStatus.GetLineCount();
	m_currStatus.LineScroll(iLine, 0);
	m_currStatus.SetSel(iCount, iCount);
	m_currStatus.SetRedraw(TRUE);
}

void CYouMeDemoDlg::OnBnClickedBtnClear()
{
	// TODO:  在此添加控件通知处理程序代码
	m_currMessage.Empty();
	m_currStatus.SetWindowTextA(m_currMessage);
}
