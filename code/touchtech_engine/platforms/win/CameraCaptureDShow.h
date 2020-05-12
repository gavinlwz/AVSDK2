#ifndef CCAMERA_CAPTURE_H
#define CCAMERA_CAPTURE_H


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <atlbase.h>
#include <string>
#include <list>

// #include "./DShow/include/dshow.h"
#include "./DShow/include/qedit.h"
#include "./DShow/include/control.h"
#include "./DShow/include/dvdmedia.h"
#include "video_capture_dshow.h"

#include <YouMeCommon/yuvlib/libyuv.h>

#ifdef _JACKY_USING_OPENCV_
#include <cxcore.h>
#endif




#define MYFREEMEDIATYPE(mt)	{if ((mt).cbFormat != 0)		\
					{CoTaskMemFree((PVOID)(mt).pbFormat);	\
					(mt).cbFormat = 0;						\
					(mt).pbFormat = NULL;					\
				}											\
				if ((mt).pUnk != NULL)						\
				{											\
					(mt).pUnk->Release();					\
					(mt).pUnk = NULL;						\
				}}									


typedef void(*SampleGrabberCallBack)(BYTE * pBuffer, long lBufferSize, int width, int height,int fps, double dblSampleTime, void* context);


class CSampleGrabberCB :public ISampleGrabberCB 
{
public:
	CRITICAL_SECTION  m_cs;

	CSampleGrabberCB(){
		m_width = 0;
		m_height = 0;
		m_fps = 0;
		m_callback = NULL;
		m_context = NULL;
	}
	~CSampleGrabberCB(){
	}
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
	{
		if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown )
		{ 
			*ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
			return NOERROR;
		} 
		return E_NOINTERFACE;
	}
	STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )
	{
		return 0;
	}
	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize )
	{	
		if (!pBuffer)
			return  E_POINTER;
		
		if (m_callback){
			(*m_callback)(pBuffer, lBufferSize, m_width, m_height, m_fps, dblSampleTime, m_context);
		}
		return 0;
	}


public:
	int  m_width;
	int  m_height;
	int  m_fps;
	SampleGrabberCallBack m_callback;
	void * m_context;
};

class CaptureSinkFilter;

class CCameraCaptureDShow: public VideoCaptureExternal
{
	
private:
#ifdef _JACKY_USING_OPENCV_
	IplImage * m_pFrame;
#else
	BYTE*		m_pFrame;
#endif
	int m_samplingMode;			// sampling mode: 0 for GrabberFilter; 1 for SinkFilter
	bool m_bConnected;
	int m_nWidth;
	int m_nHeight;
	int m_nFps;
	bool m_bLock;
	bool m_bChanged;
	long m_nBufferSize;
	bool graphStatus;

	CComPtr<IGraphBuilder> m_pGraph;
	CComPtr<IMediaControl> m_pMediaControl;
	CComPtr<IBaseFilter> m_pDeviceFilter;
	
	CComPtr<IPin> m_pCameraOutput;

	// for GrabberFilter
	CComPtr<IBaseFilter> m_pSampleGrabberFilter;
	CComPtr<ISampleGrabber> m_pSampleGrabber;
	CComPtr<IPin> m_pGrabberInput;
	CComPtr<IPin> m_pGrabberOutput;
	CComPtr<IMediaEvent> m_pMediaEvent;
	CComPtr<IBaseFilter> m_pNullFilter;
	CComPtr<IPin> m_pNullInputPin;

	CSampleGrabberCB  m_SampleCB;

	// for SinkFilter
	CComPtr<CaptureSinkFilter> m_pSinkFilter;
	CComPtr<IPin> m_pInputSendPin;
private:
	bool BindFilter(int nCamIDX, IBaseFilter **pFilter);
	void SetCrossBar();

	YOUME_libyuv::RotationMode ConvertRotationMode(VideoRotation rotation);
	VideoType RawVideoTypeToCommonVideoVideoType(RawVideoType type);
	int ConvertVideoType(VideoType video_type);
	size_t CalcBufferSize(VideoType type, int width, int height);

public:
	CCameraCaptureDShow();
	virtual ~CCameraCaptureDShow();

	// Implement VideoCaptureExternal
    // |capture_time| must be specified in NTP time format in milliseconds.
    virtual int IncomingFrame(unsigned char* videoFrame,
                                  size_t videoFrameLength,
                                  const VideoCaptureCapability& frameInfo,
                                  int64_t captureTime = 0);

	// 打开摄像头，nCamID指定打开哪个摄像头，取值可以为0,1,2,...
	// mode表示camera采集模式 0: GrabberFilter; 1: SinkFilter, default is 0
	// bDisplayProperties指示是否自动弹出摄像头属性页
	// nWidth和nHeight设置的摄像头的宽和高，如果摄像头不支持所设定的宽度和高度，则返回false
	bool OpenCamera(int nCamID, int mode, bool bDisplayProperties=true, int nWidth=320, int nHeight=240, int fps = 30);

	//关闭摄像头，析构函数会自动调用这个函数
	void CloseCamera();

	void SetCallBack(SampleGrabberCallBack callback, void *context);

	//返回摄像头的数目
	//可以不用创建CCameraDS实例，采用int c=CCameraDS::CameraCount();得到结果。
	static int CameraCount(); 

	//根据摄像头的编号返回摄像头的名字
	//param:
	//	nCamID: 摄像头编号
	//return 摄像头名字，出错时返回空字符串
	//可以不用创建CCameraDS实例，采用CCameraDS::CameraName();得到结果。
	static std::string CameraName(int nCamID);

	//显示设置属性
	static int ShowCaptureProperty(int nCamID);

	//返回图像宽度
	int GetWidth(){return m_nWidth;} 

	//返回图像高度
	int GetHeight(){return m_nHeight;}

	bool GetCaptureSize(int &desWidth, int &desHeight, int& desFps, int srcWidth, int srcHeight, int srcFps, int &capabilityIndex, BOOL onlyMJPG = false);

	bool GetCaptureSize(int &desWidth, int &desHeight, int& desFps, RawVideoType &desttype, int srcWidth, int srcHeight, int srcFps, int &capabilityIndex, BOOL onlyMJPG = false);

	//抓取一帧，返回的IplImage不可手动释放！
	//返回图像数据的为RGB模式的Top-down(第一个字节为左上角像素)，即IplImage::origin=0(IPL_ORIGIN_TL)
#ifdef _JACKY_USING_OPENCV_
	IplImage * QueryFrame();
#else
	BYTE* QueryFrame(long *nsize);
#endif


};





#endif 
