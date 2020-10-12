// YouMeVideoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "YouMeDemo.h"
#include "YouMeVideoDlg.h"
#include "afxdialogex.h"
#include "YouMeEventCallback.h"
#include <stdio.h>

// YouMeVideoDlg 对话框

IMPLEMENT_DYNAMIC(YouMeVideoDlg, CDialogEx)

YouMeVideoDlg::YouMeVideoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(YouMeVideoDlg::IDD, pParent)
{
	m_openCamera = false;
	m_isJoinRoom = false;
	m_vecWinId.push_back(IDC_STATIC_VIDEO1);
	m_vecWinId.push_back(IDC_STATIC_VIDEO2);
	m_vecWinId.push_back(IDC_STATIC_VIDEO3);
	m_vecWinId.push_back(IDC_STATIC_VIDEO4);
}

YouMeVideoDlg::~YouMeVideoDlg()
{
}

void YouMeVideoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_VIDEO1, m_videoView1);
	DDX_Control(pDX, IDC_STATIC_VIDEO2, m_videoView2);
	DDX_Control(pDX, IDC_STATIC_VIDEO3, m_videoView3);
	DDX_Control(pDX, IDC_STATIC_VIDEO4, m_videoView4);
}


BEGIN_MESSAGE_MAP(YouMeVideoDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CAM_OPEN, &YouMeVideoDlg::OnBnClickedButtonCamOpen)
	ON_BN_CLICKED(IDC_BUTTON_CAM_CLOSE, &YouMeVideoDlg::OnBnClickedButtonCamClose)
END_MESSAGE_MAP()


// YouMeVideoDlg 消息处理程序
BOOL YouMeVideoDlg::OnInitDialog()
{
	__super::OnInitDialog();

	// TODO:  在此添加额外的初始化
	//local
	//addRender(0);


	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常:  OCX 属性页应返回 FALSE
}


void YouMeVideoDlg::OnBnClickedButtonCamOpen()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_openCamera)
		return;
	if (!m_isJoinRoom)
	{
		::MessageBox(NULL, "请先进入频道！",  "提示", MB_OK);
		return;
	}


	IYouMeVoiceEngine::getInstance()->setVideoCallback(this);
	//IYouMeVoiceEngine::getInstance()->setCaptureProperty(15, 640, 480);
	IYouMeVoiceEngine::getInstance()->startCapture();
	m_openCamera = true;
}


void YouMeVideoDlg::OnBnClickedButtonCamClose()
{
	// TODO:  在此添加控件通知处理程序代码
	if (m_openCamera){
		IYouMeVoiceEngine::getInstance()->stopCapture();
		m_openCamera = false;
	}
}

void YouMeVideoDlg::frameRender(int renderId, int nWidth, int nHeight, int nRotationDegree, int nBufSize, const void * buf)
{
	MapSdl::iterator iter = m_mapRender.find(renderId);
	if (iter != m_mapRender.end()){
		SDL_Display(iter->second.hsdl, (const unsigned char*)buf, nWidth, nHeight, nWidth);
#if 0
		static FILE * pf = NULL;
		if (!pf)
			fopen_s(&pf, "test.yuv", "wb");
			if(pf)
				fwrite(buf, nWidth*nHeight*3/2,1,pf);
#endif
	}
}

void YouMeVideoDlg::addRender(int renderId)
{
	if (m_mapRender.size() > 4 || m_vecWinId.size() == 0)
		return;

	int id = m_vecWinId[0];
	HWND hw = GetDlgItem(id)->m_hWnd;
	_HSDL hsdl = SDL_Display_Init(hw, 160, 120, 15);
	if (hsdl == NULL)
		return;
	sdlinfo info = { hsdl, id };
	
	m_mapRender.insert(std::make_pair(renderId, info));
	m_vecWinId.erase(m_vecWinId.begin());
}

void YouMeVideoDlg::removeRender(int renderId)
{

	MapSdl::iterator iter = m_mapRender.find(renderId);
	if (iter != m_mapRender.end()){
		m_vecWinId.push_back(iter->second.winid);
		SDL_Display_Uninit(iter->second.hsdl);
		m_mapRender.erase(iter);
	}
	
}

int YouMeVideoDlg::removeRender(std::string userid){

	MapSdl::iterator iter = m_mapRender.begin();
	for (; iter != m_mapRender.end(); ++iter)
	{
		if (iter->second.userid == userid)
		{
			removeRender(iter->first);
			return iter->first;
		}
	}
	return 0;
}

void YouMeVideoDlg::clearRender()
{
	MapSdl::iterator iter = m_mapRender.begin();
	for (; iter != m_mapRender.end();)
	{
		m_vecWinId.push_back(iter->second.winid);
		SDL_Display_Uninit(iter->second.hsdl);
		iter = m_mapRender.erase(iter);
	}
}

void YouMeVideoDlg::joinRoomSuc(bool state)
{
	if (!state)
	{
		OnBnClickedButtonCamClose();
		clearRender();
	}
	m_isJoinRoom = state;
	

}