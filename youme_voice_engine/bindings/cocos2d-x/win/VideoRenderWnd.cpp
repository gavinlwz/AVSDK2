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
	hInstance = GetModuleHandle(NULL);	//获取一个应用程序或动态链接库的模块句柄
	WNDCLASS Render_WND;
	Render_WND.cbClsExtra = 0;
	Render_WND.cbWndExtra = 0;
	Render_WND.hCursor = LoadCursor(hInstance, IDC_ARROW);	//鼠标风格
	Render_WND.hIcon = NULL;	//图标风格
	Render_WND.lpszMenuName = NULL;	//菜单名
	Render_WND.style = CS_HREDRAW | CS_VREDRAW;	//窗口的风格
	Render_WND.hbrBackground = (HBRUSH)COLOR_WINDOW;	//背景色
	Render_WND.lpfnWndProc = WindowProc;	//【关键】采用自定义消息处理函数，也可以用默认的DefWindowProc
	Render_WND.lpszClassName = "YouMeRenderWindow";	//【关键】该窗口类的名称
	Render_WND.hInstance = hInstance;	//【关键】表示创建该窗口的程序的运行实体代号

	RegisterClass(&Render_WND);

	m_hWnd = CreateWindow(
		"YouMeRenderWindow",           //【关键】上面注册的类名lpszClassName，要完全一致  
		"",   //窗口标题文字  
		WS_OVERLAPPEDWINDOW, //窗口外观样式  
		-2,             //窗口相对于父级的X坐标  
		-2,             //窗口相对于父级的Y坐标  
		1,                //窗口的宽度  
		1,                //窗口的高度  
		NULL,               //没有父窗口，为NULL  
		NULL,               //没有菜单，为NULL  
		hInstance,          //当前应用程序的实例句柄  
		NULL);              //没有附加数据，为NULL  

	if (!m_hWnd)
		return NULL;

	// 显示窗口  
	ShowWindow(m_hWnd, SW_HIDE);

	// 更新窗口  
	UpdateWindow(m_hWnd);

	return m_hWnd;
}

void VideoRenderWnd::destroy(){
	if (m_hWnd){
		::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
		m_hWnd = NULL;
	}
	
}


// 自定义消息处理函数的实现
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
