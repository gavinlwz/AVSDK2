#pragma once
#include <wtypes.h>


class VideoRenderWnd
{
public:
	VideoRenderWnd();
	~VideoRenderWnd();

	HWND create();
	void destroy();

	static LRESULT CALLBACK WindowProc(
		_In_  HWND hwnd,
		_In_  UINT uMsg,
		_In_  WPARAM wParam,
		_In_  LPARAM lParam
		);

	int  createLocalRender(HWND  hWnd);
	void deleteLocalRender();
	int  createRender(const char* userId, HWND  hWnd);
	void deleteRender(const char* userId);
	void deleteAllRender();
	void update();
	int setRenderRotation(const char* userId, int rotation);

private:
	HWND m_hWnd;
	bool m_updateIng;
};

