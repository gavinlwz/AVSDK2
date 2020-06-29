#include "VideoRenderWnd.h"
#include <windows.h>
#include "VideoRenderMgr.h"


#define WM_RENDER_UPDATE                      (WM_USER + 0x2101)
#define WM_RENDER_CREATE_LOCAL_PREVIEW        (WM_USER + 0x2102)
#define WM_RENDER_DELETE_LOCAL_PREVIEW        (WM_USER + 0x2103)
#define WM_RENDER_CREATE_RENDER               (WM_USER + 0x2104)
#define WM_RENDER_DELETE_RENDER               (WM_USER + 0x2105)
#define WM_RENDER_DELETE_ALL_RENDER           (WM_USER + 0x2106)
#define WM_RENDER_SET_ROTATION           	  (WM_USER + 0x2107)

VideoRenderWnd::VideoRenderWnd()
{
	m_updateIng = false;
	m_hWnd = NULL;
}


VideoRenderWnd::~VideoRenderWnd()
{
	destroy();
}


HWND VideoRenderWnd::create(){

	HINSTANCE hInstance;
	hInstance = GetModuleHandle(NULL);	//��ȡһ��Ӧ�ó����̬���ӿ��ģ����
	WNDCLASS Render_WND;
	Render_WND.cbClsExtra = 0;
	Render_WND.cbWndExtra = 0;
	Render_WND.hCursor = LoadCursor(hInstance, IDC_ARROW);	//�����
	Render_WND.hIcon = NULL;	//ͼ����
	Render_WND.lpszMenuName = NULL;	//�˵���
	Render_WND.style = CS_HREDRAW | CS_VREDRAW;	//���ڵķ��
	Render_WND.hbrBackground = (HBRUSH)COLOR_WINDOW;	//����ɫ
	Render_WND.lpfnWndProc = WindowProc;	//���ؼ��������Զ�����Ϣ��������Ҳ������Ĭ�ϵ�DefWindowProc
	Render_WND.lpszClassName = "YouMeRenderWindow";	//���ؼ����ô����������
	Render_WND.hInstance = hInstance;	//���ؼ�����ʾ�����ô��ڵĳ��������ʵ�����

	RegisterClass(&Render_WND);

	m_hWnd = CreateWindow(
		"YouMeRenderWindow",           //���ؼ�������ע�������lpszClassName��Ҫ��ȫһ��  
		"",   //���ڱ�������  
		WS_OVERLAPPEDWINDOW, //���������ʽ  
		-2,             //��������ڸ�����X����  
		-2,             //��������ڸ�����Y����  
		1,                //���ڵĿ��  
		1,                //���ڵĸ߶�  
		NULL,               //û�и����ڣ�ΪNULL  
		NULL,               //û�в˵���ΪNULL  
		hInstance,          //��ǰӦ�ó����ʵ�����  
		NULL);              //û�и������ݣ�ΪNULL  

	if (!m_hWnd)
		return NULL;

	// ��ʾ����  
	ShowWindow(m_hWnd, SW_HIDE);

	// ���´���  
	UpdateWindow(m_hWnd);

	return m_hWnd;
}

void VideoRenderWnd::destroy(){
	if (m_hWnd){
		::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
		m_hWnd = NULL;
	}
	
}


// �Զ�����Ϣ��������ʵ��
LRESULT CALLBACK VideoRenderWnd::WindowProc(
	_In_  HWND hwnd,
	_In_  UINT uMsg,
	_In_  WPARAM wParam,
	_In_  LPARAM lParam
	)
{
	switch (uMsg)
	{
		case WM_RENDER_UPDATE:
		{
			VideoRenderWnd * self = (VideoRenderWnd*)lParam;
			if (self) self->m_updateIng = false;
			VideoRenderMgr::getInstance()->onVideoUpdate();
		}
			break;
		case WM_RENDER_CREATE_LOCAL_PREVIEW:
		{
			VideoRenderMgr::getInstance()->onCreateLocalRender((HWND)lParam);
		}
			break;
		case WM_RENDER_DELETE_LOCAL_PREVIEW:
		{
			VideoRenderMgr::getInstance()->onDeleteLocalRender();
		}
			break;
		case WM_RENDER_CREATE_RENDER:
		{
			char* userId = (char*)wParam;
			VideoRenderMgr::getInstance()->onCreateRender(userId, (HWND)lParam);
			delete[] userId;
		}
			break;
		case WM_RENDER_DELETE_RENDER:
		{
			char* userId = (char*)wParam;
			VideoRenderMgr::getInstance()->onDeleteRender(userId);
			delete[] userId;
		}
			break;
		case WM_RENDER_DELETE_ALL_RENDER:
		{
			VideoRenderMgr::getInstance()->onDeleteAllRender();
		}
			break;
		case WM_RENDER_SET_ROTATION:
		{
			char* userId = (char*)wParam;
			VideoRenderMgr::getInstance()->onSetRenderRotation(userId, (int)lParam);
			delete[] userId;
		}
			break;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int  VideoRenderWnd::createLocalRender(HWND  hWnd){
	if (!m_hWnd || !hWnd)
		return -1;
	 ::PostMessage(m_hWnd, WM_RENDER_CREATE_LOCAL_PREVIEW, 0, (LPARAM)hWnd);
	 return 0;
}

void  VideoRenderWnd::deleteLocalRender(){
	if (!m_hWnd)
		return;
	::PostMessage(m_hWnd, WM_RENDER_DELETE_LOCAL_PREVIEW, 0, 0);
}

int  VideoRenderWnd::createRender(const char* userId, HWND  hWnd){
	if (!m_hWnd || !hWnd)
		return -1;
	int len = strlen(userId);
	char * strTmp = new char[len + 1];
	strTmp[len] = '\0';
	strcpy(strTmp, userId);
    ::PostMessage(m_hWnd, WM_RENDER_CREATE_RENDER, (WPARAM)strTmp, (LPARAM)hWnd);
	return 0;
}

void  VideoRenderWnd::deleteRender(const char* userId){
	if (!m_hWnd)
		return;
	int len = strlen(userId);
	char * strTmp = new char[len + 1];
	strTmp[len] = '\0';
	strcpy(strTmp, userId);
	::PostMessage(m_hWnd, WM_RENDER_DELETE_RENDER, (WPARAM)strTmp, 0);
}

void  VideoRenderWnd::deleteAllRender(){
	if (!m_hWnd)
		return;
	::PostMessage(m_hWnd, WM_RENDER_DELETE_ALL_RENDER, 0, 0);
}

void VideoRenderWnd::update(){
	if (m_updateIng || !m_hWnd)
		return;
	m_updateIng = true;
	::PostMessage(m_hWnd, WM_RENDER_UPDATE, 0, (LPARAM)this);
}

int  VideoRenderWnd::setRenderRotation(const char* userId, int rotation){
	if (!m_hWnd)
		return -1;

	int len = strlen(userId);
	char * strTmp = new char[len + 1];
	strTmp[len] = '\0';
	strcpy(strTmp, userId);
    ::PostMessage(m_hWnd, WM_RENDER_SET_ROTATION, (WPARAM)strTmp, (LPARAM)rotation);
	return 0;
}
