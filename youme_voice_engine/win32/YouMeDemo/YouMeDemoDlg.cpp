
// YouMeDemoDlg.cpp : ʵ���ļ�
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

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_YOUME_ABOUT };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CYouMeDemoDlg �Ի���



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


// CYouMeDemoDlg ��Ϣ�������

BOOL CYouMeDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	m_currMessage.Empty();
	m_sliderVol.SetRange(0, 100);
	m_sliderVol.SetPos(100);

	m_ServerList.AddString(_T("ѡ��һ������"));
	m_UserList.AddString(_T("ѡ��һ���û�"));
	m_ServerList.SetCurSel(0);
	m_UserList.SetCurSel(0);

	int ver = -1;
	CString strVersion;
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		ver = m_cConfig.m_pEngine->getSDKVersion();
		
		CString strMsg;
		strMsg.Format(_T("@@@��ȡ��ǰSDK�汾 [%d] "), ver);
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
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CYouMeDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CYouMeDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CYouMeDemoDlg::OnBnClickedBtnLeaveroom()
{
	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInRoom) {
			ShowStatus(_T("��û�м�����飬�����˳���"));
			return;
		}

		YouMeErrorCode ret = m_cConfig.m_pEngine->leaveChannelMultiMode(m_strRoomID.c_str());

		CString strMsg;
		strMsg.Format(_T("@@@�뿪���飬����ID [ %s ] ����ֵ [%d] "), m_strRoomID.c_str(), ret);
		ShowStatus(strMsg);
	}	
}


void CYouMeDemoDlg::OnBnClickedBtnJoinroom()
{
	CString strUserID, strRoomID;
	GetDlgItemText(IDC_EDIT_USERID, strUserID);
	GetDlgItemText(IDC_EDIT_ROOMID, strRoomID);

	if (!isInit) {
		ShowStatus(_T("��û��ʼ�������ʼ����ɺ�������."));
		return;
	}

	if (isInRoom) {
		ShowStatus(_T("�Ѿ��ڻ������ˣ������˳���ǰ���顣"));
		return;
	}
	
	if (strUserID.IsEmpty() || strRoomID.IsEmpty())
	{
		ShowStatus(_T("�û������߷���������Ϊ��"));
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
			strMsg.Format(_T("@@@�������, �û�ID [%s], ����ID [%s], �û��б� [%d], ��˷� [%d], ������ [%d], �Զ�״̬ [%d]"), m_strUserID.c_str(), m_strRoomID.c_str(), (int)needUserList, (int)needMic, (int)needSpeaker, (int)needAutoStatus);
			ShowStatus(strMsg);

			strMsg.Empty();
			strMsg.Format(_T("������� ����ֵ [%d] "), ret);
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
			ShowStatus(_T("�Ѿ���ʼ���ˣ������ٴγ�ʼ����"));
			return;
		}

		/** ʹ�ò��Է����� */
		YouMeErrorCode ret = initEngine();
	
	
		CString strMsg;
		strMsg.Format(_T("@@@��ʼ�� ����ֵ [ %d ]"), ret);
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnUninit()
{
	if (m_cConfig.m_pEngine == NULL) {
		ShowStatus(_T("�Ѿ��˳��ˣ���ô�ᷴ��ʼ���ء�"));
		return;
	}

	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInit) {
			ShowStatus(_T("��ǰû�г�ʼ��������Ҫ���з���ʼ��."));
			return;
		}

		if (isInRoom) {
			ShowStatus(_T("���ڻ����У������˳����顣"));
			return;
		}

		YouMeErrorCode ret = m_cConfig.m_pEngine->unInit();

		isInit = false;

		CString strMsg;
		strMsg.Format(_T("@@@����ʼ�� ����ֵ [%d]"), ret);
		ShowStatus(strMsg);

	}
}

void CYouMeDemoDlg::OnNMCustomdrawSliderVol(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	int volume = m_sliderVol.GetPos();
	CString strVol;
	strVol.Format(_T("%d"), volume);
	m_setVol.SetWindowTextA(strVol);
	*pResult = 0;

	if (m_cConfig.EngineInstance(this->GetSafeHwnd()))
	{
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷�����������"));
			return;
		}
		m_cConfig.m_pEngine->setVolume(volume);
	}

	CString strMsg;
	strMsg.Format(_T("��������: %d"), volume);
	ShowStatus(strMsg);
}


void CYouMeDemoDlg::OnBnClickedBtnCloseMic()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷�����˷羲����"));
			return;
		}

		ShowStatus(_T("[�Լ�] ��˷羲��"));
		m_cConfig.m_pEngine->setMicrophoneMute(true);
	}

}


void CYouMeDemoDlg::OnBnClickedBtnGetMic()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷���ȡ��˷羲��״̬��"));
			return;
		}

		bool status = m_cConfig.m_pEngine->getMicrophoneMute();
		CString strMsg;
		strMsg.Format(_T("��ȡ [�Լ�] ��˷羲��״̬:%d"), status);
		ShowStatus(strMsg);

	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenMic()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷�ȡ����˷羲����"));
			return;
		}

		ShowStatus(_T("[�Լ�] ��˷�ȡ������"));
		m_cConfig.m_pEngine->setMicrophoneMute(false);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnGetSpeaker()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷���ȡ��ǰ������״̬��"));
			return;
		}

		bool status = m_cConfig.m_pEngine->getSpeakerMute();
		CString strMsg;
		strMsg.Format(_T("��ȡ [�Լ�] ������״̬:%d"), status);
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenSpeaker()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷�����������"));
			return;
		}

		m_cConfig.m_pEngine->setSpeakerMute(false);
		CString strMsg;
		strMsg.Format(_T("�� [�Լ�] ������"));
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseSpeaker()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷��ر���������"));
			return;
		}

		ShowStatus(_T("�ر� [�Լ�] ������"));
		m_cConfig.m_pEngine->setSpeakerMute(false);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenOtherSpeaker()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();
		
		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherSpeakerMute(user_id.c_str(), true);

		CString strMsg;
		strMsg.Format(_T("��[ %s ]������"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseOtherSpeaker()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherSpeakerMute(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("�ر�[ %s ]������"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnCloseOtherMic()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherMicMute(user_id.c_str(), true);
		
		CString strMsg;
		strMsg.Format(_T("�ر�[ %s ]��˷�"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnOpenOtherMic()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setOtherMicMute(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("��[ %s ]����˷�"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnBnClickedBtnQuit()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.m_pEngine == NULL) {
		ShowStatus(_T("�Ѿ��˳��ˣ���Ҫ�ظ����á�"));
		return;
	}

	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		ShowStatus(_T("�˳�����������API"));
		m_cConfig.m_pEngine->destroy();
		m_cConfig.m_pEngine = NULL;

		isInit = false;
		isInRoom = false;
	}
}



void CYouMeDemoDlg::OnBnClickedBtnAvoidOthers()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setListenOtherVoice(user_id.c_str(), false);

		CString strMsg;
		strMsg.Format(_T("��������[ %s ]������"), user_id.c_str());
		ShowStatus(strMsg);
	}
}

void CYouMeDemoDlg::OnBnClickedBtnCancleAvoidOther()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_UserList.GetLBText(m_UserList.GetCurSel(), cs);
		std::string user_id = cs.GetBuffer();

		if (user_id.compare("") == 0 || user_id.compare("ѡ��һ���û�") == 0){
			CString strMsg;
			strMsg.Format(_T("����ѡ��һ���û�"));
			ShowStatus(strMsg);
			return;
		}

		m_cConfig.m_pEngine->setListenOtherVoice(user_id.c_str(), true);

		CString strMsg;
		strMsg.Format(_T("ȡ������[ %s ]������"), user_id.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnCbnSelchangeCmbServer()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		CString cs;
		m_ServerList.GetLBText(m_ServerList.GetCurSel(), cs);
		std::string server = cs.GetBuffer();

		int server_area = RTC_CN_SERVER;

		if (server.compare("�й�")) { server_area = RTC_CN_SERVER; }
		else if (server.compare("���")) { server_area = RTC_HK_SERVER; }
		else if (server.compare("����")) { server_area = RTC_US_SERVER; }
		else if (server.compare("�¼���")){ server_area = RTC_SG_SERVER; }
		else if (server.compare("����")){ server_area = RTC_KR_SERVER; }
		else if (server.compare("����")){ server_area = RTC_AU_SERVER; }
		else if (server.compare("�¹�")){ server_area = RTC_DE_SERVER; }
		else if (server.compare("����")){ server_area = RTC_BR_SERVER; }
		else if (server.compare("ӡ��")){ server_area = RTC_IN_SERVER; }
		else if (server.compare("�ձ�")){ server_area = RTC_JP_SERVER; }
		else if (server.compare("������")){ server_area = RTC_IE_SERVER; } 
		else { server_area = RTC_CN_SERVER; }

		m_cConfig.m_pEngine->setServerRegion((YOUME_RTC_SERVER_REGION_t)server_area, "kuangfeng",false);

		CString strMsg;
		strMsg.Format(_T("�������������� [%s]"), server.c_str());
		ShowStatus(strMsg);
	}
}


void CYouMeDemoDlg::OnCbnDropdownCmbServer()
{
	int curSel = m_ServerList.GetCurSel();
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	m_ServerList.ResetContent();
	m_ServerList.AddString("�й�");
	m_ServerList.AddString("���");
	m_ServerList.AddString("����");
	m_ServerList.AddString("�¼���");
	m_ServerList.AddString("����");
	m_ServerList.AddString("����");
	m_ServerList.AddString("�¹�");
	m_ServerList.AddString("����");
	m_ServerList.AddString("ӡ��");
	m_ServerList.AddString("�ձ�");
	m_ServerList.AddString("������");
	m_ServerList.SetCurSel(curSel);
}


void CYouMeDemoDlg::OnBnClickedBtnGetVol()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_cConfig.EngineInstance(this->GetSafeHwnd())) {
		if (!isInit) {
			ShowStatus(_T("û�г�ʼ�����޷���ȡ��ǰ������"));
			return;
		}

		int vol = m_cConfig.m_pEngine->getVolume();
		CString strVol;
		strVol.Format(_T("%d"), vol);
		m_getVol.SetWindowTextA(strVol);

		CString strVolDis;
		strVolDis.Format(_T("��ȡ��ǰϵͳ������%d"), vol);
		ShowStatus(strVolDis);
	}
}

void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c);

/**
 * �����Ա�仯
 */
void CYouMeDemoDlg::OnMemberChange(std::string roomid, std::string userlists)
{
	CString strStatusDis;
	strStatusDis.Format(_T("�ص���OnMemberChange RoomID:%s"), roomid.c_str());
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
	strStatusDis.Format(_T("�ص���OnMemberChange ��ԱID�б�%s"), userlists.c_str());
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
	strStatusDis.Format(_T("�ص���OnEvent event: %d, error: %d, room: %s, param: %s"), event, error, room.c_str(), param.c_str());
	ShowStatus(strStatusDis);

	switch (event) {
	case YOUME_EVENT_INIT_OK:			 ///< SDK��֤�ɹ�
		isInit = true;
		m_cConfig.m_pEngine->setLogLevel(LOG_INFO);
		m_cConfig.m_pEngine->setServerRegion(RTC_CN_SERVER, "", false);


		ShowStatus(_T("��ʼ���ɹ�"));
		break;
	case YOUME_EVENT_INIT_FAILED:           ///< SDK��֤ʧ��
		isInit = false;
		ShowStatus(_T("��ʼ��ʧ��"));
		break;
//	case YOUME_EVENT_INCOMING:           ///< �յ�����
//		break;
//	case YOUME_EVENT_INPROGRESS:         ///< ���ں���
//		break;
//	case YOUME_EVENT_RINGING:            ///< Զ��������
//		break;
	case YOUME_EVENT_JOIN_OK:          ///< ���гɹ�
		isInRoom = true;
		m_sliderVol.SetPos(100);
		m_YouMeVideoDlg->joinRoomSuc(true);
		m_cConfig.m_pEngine->setSpeakerMute(false);
		IYouMeVoiceEngine::getInstance()->setVideoCallback(m_YouMeVideoDlg);
		m_setVol.SetWindowTextA(_T("100"));
		ShowStatus(_T("���뷿��ɹ�"));

		break;
//	case YOUME_EVENT_TERMWAITING:        ///< �Ҷ��У����������������߶Է��Ҷ�ʱ���ܵ����¼�
		break;
	case YOUME_EVENT_LEAVED_ONE:         ///< �ѹҶϣ����������������߶Է��Ҷ�ʱ���ܵ����¼�
		isInRoom = false;
		ShowStatus(_T("�뿪����ɹ�"));
		m_YouMeVideoDlg->joinRoomSuc(false);
		break;
	case YOUME_EVENT_JOIN_FAILED:             ///< ����ʧ��
		isInRoom = false;
		ShowStatus(_T("���뷿��ʧ��"));
		break;
	case YOUME_EVENT_REC_PERMISSION_STATUS:///< ֪ͨ¼��ʧ�ܣ���ʱ������˷�mute״̬��Σ���û�����������
		break;
	case YOUME_EVENT_BGM_STOPPED:        ///< ֪ͨ�������ֲ��Ž���
		break;
	case YOUME_EVENT_BGM_FAILED:         ///< ֪ͨ�������ֲ���ʧ��
		break;
//	case YOUME_EVENT_PAUSE:              ///< ֪ͨ��ͣͨ���Ƿ�ɹ�
//		break;
//	case YOUME_EVENT_RESUME:             ///< ֪ͨ�ָ�ͨ���Ƿ�ɹ�
//		break;
	case YOUME_EVENT_SPEAK_SUCCESS:      ///< �л�����ͨ���ɹ�
		break;
	case YOUME_EVENT_SPEAK_FAILED:       ///< �л�����ͨ��ʧ��
		break;
	/*case YOUME_EVENT_MEMBER_CHANGE:      ///< �����Ա�仯
		break;
	case YOUME_EVENT_MIC_NORMAL:         ///< ��˷��������򿪣�
		break;
	case YOUME_EVENT_MIC_MUTE:           ///< ��˷羲�����رգ�
		break;
	case YOUME_EVENT_SPEAKER_NORMAL:     ///< �������������򿪣�
		break;
	case YOUME_EVENT_SPEAKER_MUTE:       ///< �������������رգ�
		break;
	case YOUME_EVENT_MIC_CTR_NORMAL:     ///< ��˷�����������򿪣�
		break;
	case YOUME_EVENT_MIC_CTR_MUTE:       ///< ��˷���ƾ������رգ�
		break;
	case YOUME_EVENT_SPEAKER_CTR_NORMAL: ///< �����������������򿪣�
		break;
	case YOUME_EVENT_SPEAKER_CTR_MUTE:   ///< ���������ƾ������رգ�
		break;
	case YOUME_EVENT_AVOID_OTHER_NORMAL: ///< �������������������Σ�
		break;
	case YOUME_EVENT_AVOID_OTHER_MUTE:   ///< �������˾��������Σ�
		break;
	case YOUME_EVENT_FAILED_LEAVE:        ///< �˳�����ʧ��
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
 * �ַ����ָ�
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
 * ��ʾStatus
 */
void CYouMeDemoDlg::ShowStatus(CString msg) {
	m_currMessage.Append("[rtc@youme ~]#  ");
	m_currMessage.Append(msg);
	m_currMessage.Append("\r\n");
	m_currStatus.SetWindowTextA(m_currMessage);

	/** ������������ */
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
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	m_currMessage.Empty();
	m_currStatus.SetWindowTextA(m_currMessage);
}
