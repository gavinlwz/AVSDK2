// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBSCREEN_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBSCREEN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef __LIB_SCREEN_h__
#define __LIB_SCREEN_h__

#ifdef LIBSCREEN_EXPORTS
#define LIBSCREEN_API __declspec(dllexport)
#else
#define LIBSCREEN_API __declspec(dllimport)
#endif


#define LIBSCREEN_API_VERSION   TEXT("1.0.0")

#define CAPTURE_MODE_DEVICE    0x01
#define CAPTURE_MODE_WINDOW    0x02
#define CAPTURE_MODE_SCREEN    0x03

#define MAX_LOADSTRING 256
typedef struct YmWindowInfo
{
	int				index;
	HWND			hWnd;
	WCHAR			szWindowClass[MAX_LOADSTRING];
	WCHAR			szWindowName[MAX_LOADSTRING];
}YMWINDOWINFO;

typedef void(*MEDIA_PICTURE_CALLBACK)(LPVOID /*param*/, LPBYTE /*data*/, UINT /*length*/, ULONGLONG /*timestamp*/);


LIBSCREEN_API HRESULT InitMedia(HINSTANCE hInstance);

LIBSCREEN_API HRESULT SetNotifyWnd(HWND hWnd);

LIBSCREEN_API HRESULT SetRenderWnd(HWND hWnd);

LIBSCREEN_API HRESULT SetCaptureMode(UINT nCaptureMode);

LIBSCREEN_API HRESULT SetCaptureFPS(INT minFps, INT maxFps); // 1 ~ 30, default 25

LIBSCREEN_API HRESULT SetCaptureWindow(HWND hWnd, UINT x, UINT y, UINT cx, UINT cy); // fullscreen/fullwindow, set cx,cy to 0 

LIBSCREEN_API HRESULT SetOutputSize(UINT cx, UINT cy);			// default same as capture source size

LIBSCREEN_API HRESULT SetPictureCallback(MEDIA_PICTURE_CALLBACK cb, LPVOID param);

LIBSCREEN_API HRESULT StartStream(BOOL bOnlyPreview);

LIBSCREEN_API HRESULT StopStream();

LIBSCREEN_API HRESULT GetWindowBitmap(HWND hWnd, HBITMAP* hBitmap);

LIBSCREEN_API HRESULT DestroyMedia();

LIBSCREEN_API void GetWindowList(YMWINDOWINFO *winList, int * winCount);


/*
usage for libscreen

step1, When Application Start:
    InitMedia()

step2, Set Input Source:
   SetCaptureMode()
    1) if capture window
        SetCaptureWindow(hWnd, ...)
    2) if capture screen
        SetCaptureWindow(NULL, ...)

step3, Set Output
    SetRenderWnd()
    SetPictureCallback()

step4, Start/Stop Stream
    StartStream()
    StopStream()

step5, When Application Exit:
    DestroyMedia()

*/

#endif