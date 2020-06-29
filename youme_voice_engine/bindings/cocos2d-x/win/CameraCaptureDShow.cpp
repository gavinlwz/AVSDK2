

#include "help_functions_dshow.h"
#include "CameraCaptureDShow.h"
//#include <math.h>
#include "sink_filter_dshow.h"
// #include "device_info_dshow.h"
#include "tsk.h"

#pragma comment(lib,"Strmiids.lib") 

CCameraCaptureDShow::CCameraCaptureDShow()
{
	m_bConnected	= false;
	m_nWidth		= 0;
	m_nHeight		= 0;
	m_bLock			= false;
	m_bChanged		= false;
	m_pFrame		= NULL;
	m_nBufferSize	= 0;

	m_pNullFilter	= NULL;
	m_pMediaEvent	= NULL;
	m_pSampleGrabberFilter = NULL;
	m_pGraph		= NULL;
	graphStatus     = false;
	//CoInitialize(NULL);
}

CCameraCaptureDShow::~CCameraCaptureDShow()
{
	CloseCamera();
	//CoUninitialize();
}

void CCameraCaptureDShow::CloseCamera()
{
	if (!m_bConnected)
		return;
	
	TSK_DEBUG_INFO ("@@ CCameraCaptureDShow::CloseCamera [%d]", m_bConnected);
	m_bConnected = false;
	HRESULT hr = S_OK;

	if ( 0 == m_samplingMode ) {
		if (m_pMediaControl){
			TSK_DEBUG_INFO ("@@ CCameraCaptureDShow::CloseCamera m_pMediaControl is not null");
			if (graphStatus){
				TSK_DEBUG_INFO ("@@ CCameraCaptureDShow::CloseCamera graphStatus is true");
				m_pMediaControl->Stop();
				if (m_pGraph)
				{
					if (m_pCameraOutput) m_pGraph->Disconnect(m_pCameraOutput);
					if (m_pNullInputPin) m_pGraph->Disconnect(m_pNullInputPin);
				}
			}
			
			TSK_DEBUG_INFO ("@@ CCameraCaptureDShow::CloseCamera mediacControl stop end");
			m_pMediaControl.Release();
		}
		
		// Enumerate the filters And remove them
		if (m_pGraph) {
			IEnumFilters *pEnum = NULL;
			hr = m_pGraph->EnumFilters(&pEnum);
			if (SUCCEEDED(hr))
			{
			    IBaseFilter *pFilter = NULL;
			    while (S_OK == pEnum->Next(1, &pFilter, NULL))
			    {
			        // Remove the filter.
			        m_pGraph->RemoveFilter(pFilter);
			        // Reset the enumerator.
			        pEnum->Reset();
			        pFilter->Release();
			    }
			    pEnum->Release();
			}
		}
	} else {
	    if (m_pMediaControl)
	    {
			TSK_DEBUG_INFO("@@ m_pMediaControl->Stop");
	        m_pMediaControl->Stop();
			TSK_DEBUG_INFO("@@ m_pMediaControl->Stop end");
	    }
	    if (m_pGraph)
	    {
	        if (m_pSinkFilter)
	            m_pGraph->RemoveFilter((IBaseFilter *)m_pSinkFilter);
	        if (m_pDeviceFilter)
	            m_pGraph->RemoveFilter(m_pDeviceFilter);
	        // if (_dvFilter)
	        //     m_pGraph->RemoveFilter(_dvFilter);
	    }	
	}


	TSK_DEBUG_INFO ("@@ CCameraCaptureDShow::CloseCamera end2");
	m_pGraph = NULL;
	m_pDeviceFilter = NULL;
	m_pMediaControl = NULL;
	m_pSampleGrabberFilter = NULL;
	m_pSampleGrabber = NULL;
	m_pGrabberInput = NULL;
	m_pGrabberOutput = NULL;
	m_pCameraOutput = NULL;
	m_pMediaEvent = NULL;
	m_pNullFilter = NULL;
	m_pNullInputPin = NULL;
	graphStatus     = false;

	m_pSinkFilter = NULL;
	m_pInputSendPin = NULL;
	if (m_pFrame)
	{
#ifdef _JACKY_USING_OPENCV_
		cvReleaseImage(&m_pFrame);
#else
		delete[] m_pFrame;
		m_pFrame = NULL;
#endif
	}

	m_bConnected = false;
	m_nWidth = 0;
	m_nHeight = 0;
	m_bLock = false;
	m_bChanged = false;
	m_nBufferSize = 0;
}

bool CCameraCaptureDShow::OpenCamera(int nCamID, int mode, bool bDisplayProperties, int nWidth, int nHeight, int fps)
{
	TSK_DEBUG_INFO("dshow OpenCamera start, mode:%d, cameraId:%d w:%d h:%d fps:%d\n", mode, nCamID, nWidth, nHeight, fps);
	// 1. get video sampling mode
	m_samplingMode = mode;

	HRESULT hr = S_OK;
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	CloseCamera();

	hr = BindFilter(nCamID, &m_pDeviceFilter);
	if (m_pDeviceFilter == NULL){
		TSK_DEBUG_ERROR("dshow bindfilter failed, cameraId:\n", nCamID);
		return false;
	}

	// Create the Filter Graph Manager.
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
							IID_IGraphBuilder, (void **)&m_pGraph);
	if (FAILED(hr)) {
		TSK_DEBUG_ERROR("dshow create instance failed :%d,\n", hr);
		return false;
	}

	// m_pMediaControl
	hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) &m_pMediaControl);
    if (FAILED(hr)) {
        TSK_DEBUG_ERROR("dshow create instance mediacControl failed :%d,\n", hr);
        return false;
    }

    // m_pDeviceFilter
    hr = m_pGraph->AddFilter(m_pDeviceFilter, L"VideoCaptureFilter");
    if (FAILED(hr)) {
        TSK_DEBUG_ERROR("dshow add the capture device to the graph failed :%d,\n", hr);
        return false;
    }

    m_pCameraOutput = GetOutputPin(m_pDeviceFilter, PIN_CATEGORY_CAPTURE);
	if (!m_pCameraOutput) {
		TSK_DEBUG_ERROR("GetOutputPin camera output pin is null\n", hr);
		return false;
	}

	if ( 0 == m_samplingMode ) {
		hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, 
								IID_IBaseFilter, (LPVOID *)&m_pSampleGrabberFilter);

		// hr = m_pGraph->QueryInterface(IID_IMediaControl, (void **) &m_pMediaControl);
		hr = m_pGraph->QueryInterface(IID_IMediaEvent, (void **) &m_pMediaEvent);
		if (FAILED(hr)) {
			TSK_DEBUG_ERROR("dshow create instance filter failed :%d,\n", hr);
			return false;
		}
		hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
								IID_IBaseFilter, (LPVOID*) &m_pNullFilter);

		if (FAILED(hr)) {
			TSK_DEBUG_ERROR("dshow create instance render failed :%d,\n", hr);
			return false;
		}
		hr = m_pGraph->AddFilter(m_pNullFilter, L"NullRenderer");

		hr = m_pSampleGrabberFilter->QueryInterface(IID_ISampleGrabber, (void**)&m_pSampleGrabber);

		AM_MEDIA_TYPE   mt;
		ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
		mt.majortype = MEDIATYPE_Video;
		mt.subtype = MEDIASUBTYPE_RGB24;
		mt.formattype = FORMAT_VideoInfo; 
		hr = m_pSampleGrabber->SetMediaType(&mt);
		if (FAILED(hr)) {
			TSK_DEBUG_ERROR("dshow SetMediaType failed result:%d,\n", hr);
			return false;
		}
		MYFREEMEDIATYPE(mt);

		m_pGraph->AddFilter(m_pSampleGrabberFilter, L"Grabber");
	 
		CComPtr<IEnumPins> pEnum = NULL;
		m_pSampleGrabberFilter->EnumPins(&pEnum);
		pEnum->Reset();
		hr = pEnum->Next(1, &m_pGrabberInput, NULL); 

		pEnum = NULL;
		m_pSampleGrabberFilter->EnumPins(&pEnum);
		pEnum->Reset();
		pEnum->Skip(1);
		hr = pEnum->Next(1, &m_pGrabberOutput, NULL); 

		pEnum = NULL;
		m_pNullFilter->EnumPins(&pEnum);
		pEnum->Reset();
		hr = pEnum->Next(1, &m_pNullInputPin, NULL);

		if (bDisplayProperties) 
		{
			CComPtr<ISpecifyPropertyPages> pPages;

			HRESULT hr = m_pCameraOutput->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
			if (SUCCEEDED(hr))
			{
				PIN_INFO PinInfo;
				m_pCameraOutput->QueryPinInfo(&PinInfo);

				CAUUID caGUID;
				pPages->GetPages(&caGUID);

				OleCreatePropertyFrame(NULL, 0, 0,
							L"Property Sheet", 1,
							(IUnknown **)&(m_pCameraOutput.p),
							caGUID.cElems,
							caGUID.pElems,
							0, 0, NULL);
				CoTaskMemFree(caGUID.pElems);
				PinInfo.pFilter->Release();
			}
			pPages = NULL;
		}
		else 
		{
		   IAMStreamConfig*   iconfig; 
		   iconfig = NULL;
		   hr = m_pCameraOutput->QueryInterface(IID_IAMStreamConfig,   (void**)&iconfig);   
		   if (FAILED(hr)) {
			   TSK_DEBUG_ERROR("get iconfig failed :%x\n", hr);
			   return false;
		   }
		   AM_MEDIA_TYPE* pmt;    
		   if(iconfig->GetFormat(&pmt) !=S_OK) 
		   {
			  TSK_DEBUG_ERROR("getformat failed!");
			  iconfig->Release();
			  return   false;   
		   }
	      
		   VIDEOINFOHEADER*   phead;
		   if (pmt->formattype == FORMAT_VideoInfo)   
		   {   
			   int _Width = nWidth, _Height = nHeight, _Fps = 10000000 / (float)fps;
			   do{
				   int capabilityIndex = 0;
				   if (GetCaptureSize(_Width, _Height, _Fps, nWidth, nHeight, _Fps, capabilityIndex,true))
				   {
					   TSK_DEBUG_INFO("dshow get mjpg success\n");
				   }
				   else
				   {
					   if (!GetCaptureSize(_Width, _Height, _Fps, nWidth, nHeight, _Fps, capabilityIndex, false))
					   {
						   TSK_DEBUG_ERROR("dshow get size failed w:%d,h:%d, fps:%d\n", nWidth, nHeight, fps);
						   iconfig->Release();
						   return false;
					   }
				   }

				   TSK_DEBUG_INFO("dshow get camera size success w:%d,h:%d, fps:%d\n index:%d", _Width, _Height, 10000000 / _Fps, capabilityIndex);
				   /*phead = (VIDEOINFOHEADER*)pmt->pbFormat;
				   phead->bmiHeader.biWidth = _Width;
				   phead->bmiHeader.biHeight = _Height;
				   phead->AvgTimePerFrame =  _Fps;
				   phead->bmiHeader.biSizeImage = DIBSIZE(phead->bmiHeader);*/

				   VIDEO_STREAM_CONFIG_CAPS caps;
				   hr = iconfig->GetStreamCaps(capabilityIndex, &pmt, reinterpret_cast<BYTE*> (&caps));
				   if (FAILED(hr))
				   {
					   TSK_DEBUG_ERROR("getformat failed!");
					   iconfig->Release();
					   return   false;
				   }
				   hr = iconfig->SetFormat(pmt);
				   
				   if (FAILED(hr))
				   {				   
					   if (_Width == nWidth - 1 && _Height == nHeight -1){
						   TSK_DEBUG_ERROR("dshow set size failed w:%d,h:%d, fps:%d\n err:%d", _Width, _Height, fps, hr);
						   iconfig->Release();
						   return false;
					   }
					   nWidth = _Width +1;
					   nHeight = _Height +1;
					   _Fps = 10000000 / (float)fps;
				   }
				   else{
					   break;
				   }

			   } while (true);
				TSK_DEBUG_INFO("dshow setformat success w:%d,h:%d, fps:%d\n", _Width, _Height, 10000000 / _Fps);
			}   
			iconfig->Release();   
			MYFREEMEDIATYPE(*pmt);
		}

		hr = m_pGraph->Connect(m_pCameraOutput, m_pGrabberInput);
		if (FAILED(hr))
		{
			switch (hr)
			{
			case E_FAIL:
				break;
			case E_INVALIDARG:
				break;
			case E_POINTER:
				break;
			}
			TSK_DEBUG_ERROR("graph Connect failed reuslt:%x\n", hr);
			return false;
		}
		hr = m_pGraph->Connect(m_pGrabberOutput, m_pNullInputPin);

		if (FAILED(hr))
		{
			switch(hr)
			{
				case E_FAIL :
					break;
				case E_INVALIDARG :
					break;
				case E_POINTER :
					break;
			}
			TSK_DEBUG_ERROR("graph Connect failed reuslt:%x\n", hr);
			return false;
		}
		//m_pSampleGrabber->SetBufferSamples(TRUE);
		//m_pSampleGrabber->SetOneShot(TRUE);
		m_pSampleGrabber->SetBufferSamples(FALSE);
		m_pSampleGrabber->SetOneShot(FALSE);
		hr = m_pSampleGrabber->SetCallback(&m_SampleCB, 1);
		
		hr = m_pSampleGrabber->GetConnectedMediaType(&mt);
		if (FAILED(hr)){
			TSK_DEBUG_ERROR("GetConnectedMediaType failed result:", hr);
			return false;
		}
		VIDEOINFOHEADER *videoHeader;
		videoHeader = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
		m_SampleCB.m_width = videoHeader->bmiHeader.biWidth;
		m_SampleCB.m_height = videoHeader->bmiHeader.biHeight;
		m_SampleCB.m_fps =  10000000 / videoHeader->AvgTimePerFrame;
		m_bConnected = true;

		pEnum = NULL;

		if (!graphStatus)
		{
			hr = m_pMediaControl->Run();
			if (FAILED(hr)){
				TSK_DEBUG_ERROR("mediacControl run failed result:", hr);
				graphStatus = false;
				return false;
			}
			graphStatus = true;
		}		
	} else {
	    // Create the sink filte used for receiving Captured frames.
		m_pSinkFilter = new CaptureSinkFilter((TCHAR*)L"SinkFilter", NULL, &hr,
	                                        *this, 1);
	    if (hr != S_OK)
	    {
	        TSK_DEBUG_ERROR("Failed to create send filter");
	        return false;
	    }
		//m_pSinkFilter->AddRef();

		hr = m_pGraph->AddFilter(m_pSinkFilter, L"SinkFilter");
	    if (FAILED(hr))
	    {
	       TSK_DEBUG_ERROR("Failed to add the send filter to the graph.");
	        return false;
	    }

		m_pInputSendPin = GetInputPin(m_pSinkFilter);
		if (!m_pInputSendPin) {
			TSK_DEBUG_ERROR("GetInputPin input send pin is null\n", hr);
			return false;
		}

	    //_requestedCapability.codecType = kVideoCodecUnknown;
	    // Temporary connect here.
	    // This is done so that no one else can use the capture device.
	 //    if (SetCameraOutput(_requestedCapability) != 0)
	 //    {
	 //        return -1;
	 //    }

		// hr = m_pMediaControl->Pause();
	 //    if (FAILED(hr))
	 //    {
	 //       TSK_DEBUG_ERROR("Failed to Pause the Capture device. Is it already occupied? %d.", hr);
	 //        return false;
	 //    }

		if (bDisplayProperties) 
		{
			CComPtr<ISpecifyPropertyPages> pPages;

			HRESULT hr = m_pCameraOutput->QueryInterface(IID_ISpecifyPropertyPages, (void**)&pPages);
			if (SUCCEEDED(hr))
			{
				PIN_INFO PinInfo;
				m_pCameraOutput->QueryPinInfo(&PinInfo);

				CAUUID caGUID;
				pPages->GetPages(&caGUID);

				OleCreatePropertyFrame(NULL, 0, 0,
							L"Property Sheet", 1,
							(IUnknown **)&(m_pCameraOutput.p),
							caGUID.cElems,
							caGUID.pElems,
							0, 0, NULL);
				CoTaskMemFree(caGUID.pElems);
				PinInfo.pFilter->Release();
			}
			pPages = NULL;
		}
		else 
		{
		   IAMStreamConfig*   iconfig; 
		   iconfig = NULL;
		   hr = m_pCameraOutput->QueryInterface(IID_IAMStreamConfig,   (void**)&iconfig);   
		   if (FAILED(hr)) {
			   TSK_DEBUG_ERROR("get iconfig failed :%x\n", hr);
			   return false;
		   }

	        AM_MEDIA_TYPE* temppmt = NULL;
	        VIDEO_STREAM_CONFIG_CAPS caps;
	        int count, size;

	        hr = iconfig->GetNumberOfCapabilities(&count, &size);
	        if (FAILED(hr)) {
	            TSK_DEBUG_ERROR("Failed to GetNumberOfCapabilities");
	            iconfig->Release();
	            return -1;
	        }

	        // Check if the device support formattype == FORMAT_VideoInfo2 and
	        // FORMAT_VideoInfo. Prefer FORMAT_VideoInfo since some cameras (ZureCam) has
	        // been seen having problem with MJPEG and FORMAT_VideoInfo2 Interlace flag is
	        // only supported in FORMAT_VideoInfo2
	        bool supportFORMAT_VideoInfo2 = false;
	        bool supportFORMAT_VideoInfo = false;
	        bool foundInterlacedFormat = false;
	        GUID preferedVideoFormat = FORMAT_VideoInfo;
	        for (int tmp = 0; tmp < count; ++tmp) {
	            hr = iconfig->GetStreamCaps(tmp, &temppmt, reinterpret_cast<BYTE*>(&caps));
	            if (!FAILED(hr)) {
	                if (temppmt->majortype == MEDIATYPE_Video &&
	                    temppmt->formattype == FORMAT_VideoInfo2) {
	                    TSK_DEBUG_INFO("Device support FORMAT_VideoInfo2");
	                    supportFORMAT_VideoInfo2 = true;
	                    VIDEOINFOHEADER2* h =
	                        reinterpret_cast<VIDEOINFOHEADER2*>(temppmt->pbFormat);
	                    //assert(h);
	                    foundInterlacedFormat |=
	                        h->dwInterlaceFlags &
	                        (AMINTERLACE_IsInterlaced | AMINTERLACE_DisplayModeBobOnly);
	                } else if (temppmt->majortype == MEDIATYPE_Video &&
	                    temppmt->formattype == FORMAT_VideoInfo) {
	                    TSK_DEBUG_INFO("Device support FORMAT_VideoInfo");
	                    supportFORMAT_VideoInfo = true;
	                }
	            }
	        }

	        if (supportFORMAT_VideoInfo2) {
	            if (supportFORMAT_VideoInfo && !foundInterlacedFormat) {
	                preferedVideoFormat = FORMAT_VideoInfo;
	                TSK_DEBUG_INFO("dshow preferedVideoFormat is videoinfo\n");
	            } else {
	                preferedVideoFormat = FORMAT_VideoInfo2;
	                TSK_DEBUG_INFO("dshow preferedVideoFormat is videoinfo2\n");
	            }
	        }
	        MYFREEMEDIATYPE(*temppmt);
	        // preferedVideoFormat = FORMAT_VideoInfo2;
		   
		   AM_MEDIA_TYPE* pmt;    
		   if(iconfig->GetFormat(&pmt) !=S_OK) 
		   {
			  TSK_DEBUG_ERROR("getformat failed!");
			  iconfig->Release();
			  return   false;   
		   }
	      
		   VIDEOINFOHEADER*   phead;
		   if (pmt->formattype == FORMAT_VideoInfo || pmt->formattype == FORMAT_VideoInfo2)   
		   {   
			   int _Width = nWidth, _Height = nHeight, _Fps = 10000000 / (float)fps;
	           // BOOL isVideoInfo = (preferedVideoFormat == FORMAT_VideoInfo) ? true : false;

			   do{
				   int capabilityIndex = 0;
				   RawVideoType rawtype;
				   if (GetCaptureSize(_Width, _Height, _Fps, rawtype, nWidth, nHeight, _Fps, capabilityIndex, true))
				   {
					   TSK_DEBUG_INFO("dshow get mjpg success\n");
				   }
				   else
				   {
					   if (!GetCaptureSize(_Width, _Height, _Fps, rawtype, nWidth, nHeight, _Fps, capabilityIndex, false))
					   {
						   TSK_DEBUG_ERROR("dshow get size failed w:%d,h:%d, fps:%d\n", nWidth, nHeight, fps);
						   iconfig->Release();
						   return false;
					   }
				   }

				   TSK_DEBUG_INFO("dshow get camera size success w:%d,h:%d, fps:%d\n index:%d type:%d"
				   				, _Width, _Height, 10000000 / _Fps, capabilityIndex, rawtype);
				   /*phead = (VIDEOINFOHEADER*)pmt->pbFormat;
				   phead->bmiHeader.biWidth = _Width;
				   phead->bmiHeader.biHeight = _Height;
				   phead->AvgTimePerFrame =  _Fps;
				   phead->bmiHeader.biSizeImage = DIBSIZE(phead->bmiHeader);*/

				   VIDEO_STREAM_CONFIG_CAPS caps;
				   hr = iconfig->GetStreamCaps(capabilityIndex, &pmt, reinterpret_cast<BYTE*> (&caps));
				   if (FAILED(hr))
				   {
					   TSK_DEBUG_ERROR("getformat failed!");
					   iconfig->Release();
					   return   false;
				   }


				   // Set the sink filter to request this capability
				   VideoCaptureCapability capability;
				   capability.width = _Width;
				   capability.height = _Height;
				   capability.maxFPS = 10000000 / _Fps;
				   capability.rawType = rawtype;	

				   m_pSinkFilter->SetMatchingMediaType(capability);

				   hr = iconfig->SetFormat(pmt);
				   if (FAILED(hr))
				   {				   
					   if (_Width == nWidth - 1 && _Height == nHeight -1){
						   TSK_DEBUG_ERROR("dshow set size failed w:%d,h:%d, fps:%d\n err:%d", _Width, _Height, fps, hr);
						   iconfig->Release();
						   return false;
					   }
					   nWidth = _Width +1;
					   nHeight = _Height +1;
					   _Fps = 10000000 / (float)fps;
				   }
				   else{
					   break;
				   }

			   } while (true);
				TSK_DEBUG_INFO("dshow setformat success w:%d,h:%d, fps:%d\n", _Width, _Height, 10000000 / _Fps);
			}

			iconfig->Release();   
			MYFREEMEDIATYPE(*pmt);     
		}

		hr = m_pGraph->ConnectDirect(m_pCameraOutput, m_pInputSendPin, NULL);
		if (FAILED(hr))
		{
			switch (hr)
			{
			case E_FAIL:
				break;
			case E_INVALIDARG:
				break;
			case E_POINTER:
				break;
			}
			TSK_DEBUG_ERROR("graph Connect failed reuslt:%x\n", hr);
			return false;
		}
		
		hr = m_pMediaControl->Run();
	    if (FAILED(hr))
	    {
	        TSK_DEBUG_ERROR("Failed to start the Capture device.");
	        return false;
	    }

		// VIDEOINFOHEADER *videoHeader;
		// videoHeader = reinterpret_cast<VIDEOINFOHEADER*>(mt.pbFormat);
		// m_SampleCB.m_width = videoHeader->bmiHeader.biWidth;
		// m_SampleCB.m_height = videoHeader->bmiHeader.biHeight;
		// m_SampleCB.m_fps =  10000000 / videoHeader->AvgTimePerFrame;
		
	    m_bConnected = true;

	}

	TSK_DEBUG_INFO("dshow OpenCamera end\n");
	return true;
}

void CCameraCaptureDShow::SetCallBack(SampleGrabberCallBack callback, void *context)
{
	m_SampleCB.m_callback = callback;
	m_SampleCB.m_context = context;
}

YOUME_libyuv::RotationMode CCameraCaptureDShow::ConvertRotationMode(VideoRotation rotation) {
  switch(rotation) {
    case kVideoRotation_0:
      return YOUME_libyuv::kRotate0;
    case kVideoRotation_90:
      return YOUME_libyuv::kRotate90;
    case kVideoRotation_180:
      return YOUME_libyuv::kRotate180;
    case kVideoRotation_270:
      return YOUME_libyuv::kRotate270;
  }

  return YOUME_libyuv::kRotate0;
}

VideoType CCameraCaptureDShow::RawVideoTypeToCommonVideoVideoType(RawVideoType type) {
  switch (type) {
    case kVideoI420:
      return kI420;
    case kVideoIYUV:
      return kIYUV;
    case kVideoRGB24:
      return kRGB24;
    case kVideoARGB:
      return kARGB;
    case kVideoARGB4444:
      return kARGB4444;
    case kVideoRGB565:
      return kRGB565;
    case kVideoARGB1555:
      return kARGB1555;
    case kVideoYUY2:
      return kYUY2;
    case kVideoYV12:
      return kYV12;
    case kVideoUYVY:
      return kUYVY;
    case kVideoNV21:
      return kNV21;
    case kVideoNV12:
      return kNV12;
    case kVideoBGRA:
      return kBGRA;
    case kVideoMJPEG:
      return kMJPG;
    default:
      //assert(false);
		break;
  }
  return kUnknown;
}

int CCameraCaptureDShow::ConvertVideoType(VideoType video_type) {
  switch(video_type) {
    case kUnknown:
      return YOUME_libyuv::FOURCC_ANY;
    case  kI420:
      return YOUME_libyuv::FOURCC_I420;
    case kIYUV:  // same as KYV12
    case kYV12:
      return YOUME_libyuv::FOURCC_YV12;
    case kRGB24:
      return YOUME_libyuv::FOURCC_24BG;
    case kABGR:
      return YOUME_libyuv::FOURCC_ABGR;
    case kRGB565:
      return YOUME_libyuv::FOURCC_RGBP;
    case kYUY2:
      return YOUME_libyuv::FOURCC_YUY2;
    case kUYVY:
      return YOUME_libyuv::FOURCC_UYVY;
    case kMJPG:
      return YOUME_libyuv::FOURCC_MJPG;
    case kNV21:
      return YOUME_libyuv::FOURCC_NV21;
    case kNV12:
      return YOUME_libyuv::FOURCC_NV12;
    case kARGB:
      return YOUME_libyuv::FOURCC_ARGB;
    case kBGRA:
      return YOUME_libyuv::FOURCC_BGRA;
    case kARGB4444:
      return YOUME_libyuv::FOURCC_R444;
    case kARGB1555:
      return YOUME_libyuv::FOURCC_RGBO;
  }
  //assert(false);
  return YOUME_libyuv::FOURCC_ANY;
}

size_t CCameraCaptureDShow::CalcBufferSize(VideoType type, int width, int height) {
  size_t buffer_size = 0;
  switch (type) {
    case kI420:
    case kNV12:
    case kNV21:
    case kIYUV:
    case kYV12: {
      int half_width = (width + 1) >> 1;
      int half_height = (height + 1) >> 1;
      buffer_size = width * height + half_width * half_height * 2;
      break;
    }
    case kARGB4444:
    case kRGB565:
    case kARGB1555:
    case kYUY2:
    case kUYVY:
      buffer_size = width * height * 2;
      break;
    case kRGB24:
      buffer_size = width * height * 3;
      break;
    case kBGRA:
    case kARGB:
      buffer_size = width * height * 4;
      break;
    default:
      //assert(false);
      TSK_DEBUG_ERROR("CalcBufferSize unknown videotype:%d", type);
      break;
  }
  return buffer_size;
}

// Implement VideoCaptureExternal
// |capture_time| must be specified in NTP time format in milliseconds.
int32_t CCameraCaptureDShow::IncomingFrame(uint8_t* videoFrame,
                          size_t videoFrameLength,
                          const VideoCaptureCapability& frameInfo,
                          int64_t captureTime)
{
	if (!m_bConnected)
	{
		TSK_DEBUG_ERROR("IncomingFrame when m_bConnected is false\n");
		return -1;
	}
	if (!videoFrame || !videoFrameLength) {
		TSK_DEBUG_ERROR("IncomingFrame input video buffer is null\n");
		return -1;
	}
	// TSK_DEBUG_INFO("IncomingFrame w:%d h:%d fps:%d type:%d\n", frameInfo.width, frameInfo.height, frameInfo.maxFPS, frameInfo.rawType);

	// TSK_DEBUG_INFO("IncomingFrame [0x%x][0x%x][0x%x][0x%x]\n", videoFrame[0], videoFrame[1], videoFrame[2], videoFrame[3]);
    const int32_t width = abs(frameInfo.width);
    const int32_t height = abs(frameInfo.height);

    // if (frameInfo.codecType == kVideoCodecUnknown)
    {
        // Not encoded, convert to I420.
        const VideoType commonVideoType =
                  RawVideoTypeToCommonVideoVideoType(frameInfo.rawType);

        if (frameInfo.rawType != kVideoMJPEG &&
            CalcBufferSize(commonVideoType, width,
                           abs(height)) != videoFrameLength)
        {
            TSK_DEBUG_ERROR("Wrong incoming frame length.");
            return -1;
        }

        // int stride_y = width;
        // int stride_uv = (width + 1) / 2;
        // int target_width = width;
        // int target_height = height;

        // // SetApplyRotation doesn't take any lock. Make a local copy here.
        // bool apply_rotation = apply_rotation_;

        // if (apply_rotation) {
        //   // Rotating resolution when for 90/270 degree rotations.
        //   if (_rotateFrame == kVideoRotation_90 ||
        //       _rotateFrame == kVideoRotation_270) {
        //     target_width = abs(height);
        //     target_height = width;
        //   }
        // }

        int yuvBufSize = width * height * 3 / 2;
        uint8_t* yuvBuf = (uint8_t *)malloc(yuvBufSize);
        memset(yuvBuf, 0 , yuvBufSize);

	    int y_length = width * height;
	    int Dst_Stride_Y = width;
	    int uv_stride = (width+1) / 2;

	    int uv_length = uv_stride * ((height+1) / 2);
	    uint8_t *Y_data_Dst_rotate = yuvBuf;
	    uint8_t *U_data_Dst_rotate = yuvBuf + y_length;
	    uint8_t *V_data_Dst_rotate = U_data_Dst_rotate + uv_length;
	    
	    int ret = YOUME_libyuv::ConvertToI420(
	            (const uint8*)videoFrame, videoFrameLength,
	            Y_data_Dst_rotate, Dst_Stride_Y,
	            U_data_Dst_rotate, uv_stride,
	            V_data_Dst_rotate, uv_stride,
	            0, 0,
	            width, height,
	            width, height,
	            ConvertRotationMode(kVideoRotation_0),
                ConvertVideoType(commonVideoType));

	    // TSK_DEBUG_INFO("ConvertFrame w:%d h:%d ret:%d src_size:%d dst_size:%d\n"
	    // 				, width, height, ret, videoFrameLength, yuvBufSize);
	    // memcpy(videoFrame, yuvBuf, yuvBufSize);

	    if (m_SampleCB.m_callback) {
			m_SampleCB.m_callback(yuvBuf, yuvBufSize, frameInfo.width, frameInfo.height, frameInfo.maxFPS, captureTime, m_SampleCB.m_context);
	    }
	    if (yuvBuf) {
	    	free(yuvBuf);
	    	yuvBuf = NULL;
	    }
		
    }
   
	return 0;
}


bool CCameraCaptureDShow::BindFilter(int nCamID, IBaseFilter **pFilter)
{
	if (nCamID < 0)
		return false;
    IBaseFilter *_pFilter = NULL;
    // enumerate all video capture devices
	ICreateDevEnum *pCreateDevEnum=0;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
								IID_ICreateDevEnum, (void**)&pCreateDevEnum);
	if (hr != NOERROR)
	{
		return false;
	}

    IEnumMoniker *pEm=0;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEm, 0);
    if (hr != NOERROR) 
	{
		return false;
    }
	
    //pEm->Reset();
    ULONG cFetched;
    IMoniker *pM = NULL;
	int index = 0;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK, index <= nCamID)
    {
		IPropertyBag *pBag = 0;
		hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR) 
			{
				if (index == nCamID)
				{
					hr = pM->BindToObject(0, 0, IID_IBaseFilter, (void**)&_pFilter);
					if (hr == NOERROR)
					{
					}
				}
				SysFreeString(var.bstrVal);
			}
			pBag->Release();
		}
		pM->Release();
		index++;
    }
    pEm->Release();

	if (_pFilter)
	{
		*pFilter = _pFilter;
	}

	pCreateDevEnum->Release();
	pCreateDevEnum = NULL;
	return true;
}

#define MEDIASUBTYPE_WORK(mt) ( ((mt).subtype != MEDIASUBTYPE_MJPG)             && \
                                    ((mt).subtype != MEDIASUBTYPE_IYUV)         && \
                                    ((mt).subtype != MEDIASUBTYPE_RGB1)         && \
                                    ((mt).subtype != MEDIASUBTYPE_RGB24)        && \
                                    ((mt).subtype != MEDIASUBTYPE_RGB32)        && \
                                    ((mt).subtype != MEDIASUBTYPE_RGB4)         && \
                                    ((mt).subtype != MEDIASUBTYPE_RGB555)		&& \
                                    ((mt).subtype != MEDIASUBTYPE_RGB565)		&& \
                                    ((mt).subtype != MEDIASUBTYPE_RGB8)			&& \
                                    ((mt).subtype != MEDIASUBTYPE_UYVY)			&& \
									((mt).subtype != MEDIASUBTYPE_YUY2)			&& \
									((mt).subtype != MEDIASUBTYPE_YV12)			&& \
									((mt).subtype != MEDIASUBTYPE_YVU9)			&& \
                                    ((mt).subtype != MEDIASUBTYPE_YVYU) )

#define MEDIASUBTYPE_WORK_MJPG(mt) ( ((mt).subtype == MEDIASUBTYPE_MJPG) )

bool CCameraCaptureDShow::GetCaptureSize(int &desWidth, int &desHeight, int& desFps, int srcWidth, int srcHeight, int srcFps, int &capabilityIndex,BOOL onlyMJPG)
{
	HRESULT hr;
	IAMStreamConfig* pam;
	IBaseFilter* pCapFilter = m_pDeviceFilter;
	ICaptureGraphBuilder2 *pBuild = NULL;
	 hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
		CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
		(void **)&pBuild);
	if (hr != NOERROR)
	{
		return false;
	}
	hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		pCapFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pam)); // 得到媒体控制接口
	if (hr != NOERROR)
	{
		return false;
	}
	int tmpmaxw = 0;
	int tmpmaxh = 0;
	int tmpminw = 0;
	int tmpminh = 0;
	int tmpmaxf = 0;
	int tmpminf = 0;
	int nCount = 0;
	int nSize = 0;
	float diff1 = 8.0f;
	float diff2 = 0.0f;
	hr = pam->GetNumberOfCapabilities(&nCount, &nSize);
	bool isMatch = false;
	bool isFindBest = false;

	// 判断是否为视频信息
	if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == nSize) {
		TSK_DEBUG_INFO("get video count:%d", nCount);
		for (int i = 0; i<nCount; i++) {
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE* pmmt;

			hr = pam->GetStreamCaps(i, &pmmt, reinterpret_cast<BYTE*>(&scc));

			if (pmmt->formattype == FORMAT_VideoInfo) {
				VIDEOINFOHEADER* pvih = reinterpret_cast<VIDEOINFOHEADER*>(pmmt->pbFormat);
				
				char buf[20];
				sprintf(buf, "%x", pmmt->subtype);
				//if (strcmp(buf, "35363248") == 0 || strcmp(buf, "34363248") == 0 || (pmmt->subtype != MEDIASUBTYPE_MJPG))
				//if (pmmt->subtype != MEDIASUBTYPE_MJPG && pmmt->subtype != MEDIASUBTYPE_YUYV && pmmt->subtype != MEDIASUBTYPE_IYUV
				//	&& pmmt->subtype != MEDIASUBTYPE_RGB24 )
				if (onlyMJPG){
					if (!MEDIASUBTYPE_WORK_MJPG(*pmmt)){
						continue;
					}
				}
				else{
					if (MEDIASUBTYPE_WORK(*pmmt))
					{
						TSK_DEBUG_INFO("ignore subType:%x", pmmt->subtype);
						continue;
					}
				}
				//MEDIASUBTYPE_RGB24
				int nwidth = pvih->bmiHeader.biWidth; // 得到采集的宽 
				int nheight = pvih->bmiHeader.biHeight; //　得到采集的高 
				int nfps = pvih->AvgTimePerFrame;
				/*
				if (pmmt->subtype == MEDIASUBTYPE_MJPG){
					TSK_DEBUG_INFO("dshow ignore unsupport fmt w:%d, h:%d, fps:%d, subType:%x, format:%x\n",
						nwidth, nheight, 10000000 / nfps, pmmt->subtype, pmmt->formattype);
					continue;
				}
				*/
				diff2 = nwidth / (float)nheight - srcWidth / (float)srcHeight;
				TSK_DEBUG_INFO("dshow w:%d, h:%d, fps:%d, subType:%x, format:%x\n",
					nwidth, nheight, 10000000 / nfps, pmmt->subtype, pmmt->formattype);
				
				if (nwidth == srcWidth && nheight == srcHeight)
				{
					desWidth = nwidth;
					desHeight = nheight;
					desFps = nfps;
					capabilityIndex = i;
					isMatch = true;
					isFindBest = true;
					if (nfps == srcFps)
					{
						break;
					}
				}
				else if (nwidth >= srcWidth && nheight >= srcHeight)
				{
					if ((!tmpmaxw || !tmpmaxh) || (tmpmaxw >= nwidth && tmpmaxh >= nheight/* && abs(diff2) <= abs(diff1)*/))
					{
						tmpmaxw = nwidth;
						tmpmaxh = nheight;
						tmpmaxf = nfps;
						diff1 = tmpmaxw / (float)tmpmaxh - srcWidth / (float)srcHeight;
						if (!isFindBest) {
							capabilityIndex = i;
						}
						
					}
				}
				else if (tmpminw < nwidth && tmpminh < nheight/* && abs(diff2) <= abs(diff1)*/)
				{
					tmpminw = nwidth;
					tmpminh = nheight;
					tmpminf = nfps;
					diff1 = tmpminw / (float)tmpminh - srcWidth / (float)srcHeight;
					if (!isFindBest) {
						capabilityIndex = i;
					}					
				}
			}
		}
	}
	if (pBuild)
	   pBuild->Release();
	if (pam)
		pam->Release();
	if (!isMatch)
	{
		if (tmpmaxw && tmpmaxh){
			desWidth = tmpmaxw;
			desHeight = tmpmaxh;
			desFps = tmpmaxf < srcFps ? srcFps : tmpmaxf;
		}
		else if (tmpminw && tmpminh){
			desWidth = tmpminw;
			desHeight = tmpminh;
			desFps = tmpminf < srcFps ? srcFps : tmpminf;
		}
		else{
			return false;
		}
	}

	return true;
}

bool CCameraCaptureDShow::GetCaptureSize(int &desWidth, int &desHeight, int& desFps, RawVideoType &desttype, 
										int srcWidth, int srcHeight, int srcFps, 
										int &capabilityIndex, BOOL onlyMJPG)
{
	HRESULT hr;
	IAMStreamConfig* pam;
	IBaseFilter* pCapFilter = m_pDeviceFilter;
	ICaptureGraphBuilder2 *pBuild = NULL;
	 hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
		CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2,
		(void **)&pBuild);
	if (hr != NOERROR)
	{
		return false;
	}
	hr = pBuild->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video,
		pCapFilter, IID_IAMStreamConfig, reinterpret_cast<void**>(&pam)); // 得到媒体控制接口
	if (hr != NOERROR)
	{
		return false;
	}
	int tmpmaxw = 0;
	int tmpmaxh = 0;
	int tmpminw = 0;
	int tmpminh = 0;
	int tmpmaxf = 0;
	int tmpminf = 0;
	int nCount = 0;
	int nSize = 0;
	float diff1 = 8.0f;
	float diff2 = 0.0f;
	hr = pam->GetNumberOfCapabilities(&nCount, &nSize);
	bool isMatch = false;
	bool isFindBest = false;
	RawVideoType rawType = kVideoI420;

	// 判断是否为视频信息
	if (sizeof(VIDEO_STREAM_CONFIG_CAPS) == nSize) {
		TSK_DEBUG_INFO("get video count:%d", nCount);
		for (int i = 0; i<nCount; i++) {
			VIDEO_STREAM_CONFIG_CAPS scc;
			AM_MEDIA_TYPE* pmmt;

			hr = pam->GetStreamCaps(i, &pmmt, reinterpret_cast<BYTE*>(&scc));

			// if (isVideoInfo) 
			{
				if (onlyMJPG){
					if (!MEDIASUBTYPE_WORK_MJPG(*pmmt)){
						continue;
					}
				}
				else{
					if (MEDIASUBTYPE_WORK(*pmmt))
					{
						TSK_DEBUG_INFO("videoinfo ignore subType:%x", pmmt->subtype);
						continue;
					}
				}

				// get rawtype
				// can't switch MEDIATYPE :~(
				if (pmmt->subtype == MEDIASUBTYPE_I420)
	            {
	                rawType = kVideoI420;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_IYUV)
	            {
	                rawType = kVideoIYUV;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_RGB24)
	            {
	                rawType = kVideoRGB24;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_YUY2)
	            {
	                rawType = kVideoYUY2;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_RGB565)
	            {
	                rawType = kVideoRGB565;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_MJPG)
	            {
	                rawType = kVideoMJPEG;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_dvsl
					|| pmmt->subtype == MEDIASUBTYPE_dvsd
					|| pmmt->subtype == MEDIASUBTYPE_dvhd) // If this is an external DV camera
	            {
	                rawType = kVideoYUY2;// MS DV filter seems to create this type
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_UYVY) // Seen used by Declink capture cards
	            {
	                rawType = kVideoUYVY;
	            }
				else if (pmmt->subtype == MEDIASUBTYPE_HDYC) // Seen used by Declink capture cards. Uses BT. 709 color. Not entiry correct to use UYVY. http://en.wikipedia.org/wiki/YCbCr
	            {
	                TSK_DEBUG_INFO("Device support HDYC.");
	                rawType = kVideoUYVY;
	            }
	            else
	            {
	                WCHAR strGuid[39];
					StringFromGUID2(pmmt->subtype, strGuid, 39);
	                TSK_DEBUG_INFO("Device support unknown media type %ls, width %d, height %d",
	                             strGuid);
	                continue;
	            }

	            TSK_DEBUG_INFO("support raw type:%d", rawType);
				// VIDEOINFOHEADER* pvih = reinterpret_cast<VIDEOINFOHEADER*>(pmmt->pbFormat);
				
				char buf[20];
				sprintf(buf, "%x", pmmt->subtype);
				//if (strcmp(buf, "35363248") == 0 || strcmp(buf, "34363248") == 0 || (pmmt->subtype != MEDIASUBTYPE_MJPG))
				//if (pmmt->subtype != MEDIASUBTYPE_MJPG && pmmt->subtype != MEDIASUBTYPE_YUYV && pmmt->subtype != MEDIASUBTYPE_IYUV
				//	&& pmmt->subtype != MEDIASUBTYPE_RGB24 )


				int nwidth = 0;
				int nheight = 0;
				int nfps = 0;
				if (pmmt->formattype == FORMAT_VideoInfo) {
		            VIDEOINFOHEADER* pvih = 
		            	reinterpret_cast<VIDEOINFOHEADER*>(pmmt->pbFormat);

		            nwidth = pvih->bmiHeader.biWidth;
					nheight = pvih->bmiHeader.biHeight;
		            nfps = pvih->AvgTimePerFrame;
				}

				bool interlaced = false;
				if (pmmt->formattype == FORMAT_VideoInfo2) {
					VIDEOINFOHEADER2* pvih =
						reinterpret_cast<VIDEOINFOHEADER2*> (pmmt->pbFormat);

		            
					nwidth = pvih->bmiHeader.biWidth;
					nheight = pvih->bmiHeader.biHeight;
		            interlaced = pvih->dwInterlaceFlags
		                                    & (AMINTERLACE_IsInterlaced
		                                       | AMINTERLACE_DisplayModeBobOnly);
		            nfps = pvih->AvgTimePerFrame;
				}

				/*
				if (pmmt->subtype == MEDIASUBTYPE_MJPG){
					TSK_DEBUG_INFO("dshow ignore unsupport fmt w:%d, h:%d, fps:%d, subType:%x, format:%x\n",
						nwidth, nheight, 10000000 / nfps, pmmt->subtype, pmmt->formattype);
					continue;
				}
				*/
				diff2 = nwidth / (float)nheight - srcWidth / (float)srcHeight;
				TSK_DEBUG_INFO("videoinfo dshow w:%d, h:%d, fps:%d, subType:%x, format:%x\n",
					nwidth, nheight, 10000000 / nfps, pmmt->subtype, pmmt->formattype);
				
				if (nwidth == srcWidth && nheight == srcHeight)
				{
					desWidth = nwidth;
					desHeight = nheight;
					desFps = nfps;
					capabilityIndex = i;
					isMatch = true;
					isFindBest = true;
					desttype = rawType;
					if (nfps == srcFps)
					{
						break;
					}
				}
				else if (nwidth >= srcWidth && nheight >= srcHeight)
				{
					if ((!tmpmaxw || !tmpmaxh) || (tmpmaxw >= nwidth && tmpmaxh >= nheight/* && abs(diff2) <= abs(diff1)*/))
					{
						tmpmaxw = nwidth;
						tmpmaxh = nheight;
						tmpmaxf = nfps;
						diff1 = tmpmaxw / (float)tmpmaxh - srcWidth / (float)srcHeight;
						if (!isFindBest) {
							capabilityIndex = i;
						}
						
					}
				}
				else if (tmpminw < nwidth && tmpminh < nheight/* && abs(diff2) <= abs(diff1)*/)
				{
					tmpminw = nwidth;
					tmpminh = nheight;
					tmpminf = nfps;
					diff1 = tmpminw / (float)tmpminh - srcWidth / (float)srcHeight;
					if (!isFindBest) {
						capabilityIndex = i;
					}					
				}
			} 
		}
	}
	if (pBuild)
	   pBuild->Release();
	if (pam)
		pam->Release();
	if (!isMatch)
	{
		if (tmpmaxw && tmpmaxh){
			desWidth = tmpmaxw;
			desHeight = tmpmaxh;
			desFps = tmpmaxf < srcFps ? srcFps : tmpmaxf;
		}
		else if (tmpminw && tmpminh){
			desWidth = tmpminw;
			desHeight = tmpminh;
			desFps = tmpminf < srcFps ? srcFps : tmpminf;
		}
		else{
			return false;
		}
	}

	return true;
}

//将输入crossbar变成PhysConn_Video_Composite
void CCameraCaptureDShow::SetCrossBar()
{
	int i;
	IAMCrossbar *pXBar1 = NULL;
	ICaptureGraphBuilder2 *pBuilder = NULL;

 
	HRESULT hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL,
					CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, 
					(void **)&pBuilder);

	if (SUCCEEDED(hr))
	{
		hr = pBuilder->SetFiltergraph(m_pGraph);
	}


	hr = pBuilder->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, 
								m_pDeviceFilter,IID_IAMCrossbar, (void**)&pXBar1);

	if (SUCCEEDED(hr)) 
	{
  		long OutputPinCount;
		long InputPinCount;
		long PinIndexRelated;
		long PhysicalType;
		long inPort = 0;
		long outPort = 0;

		pXBar1->get_PinCounts(&OutputPinCount,&InputPinCount);
		for( i =0;i<InputPinCount;i++)
		{
			pXBar1->get_CrossbarPinInfo(TRUE,i,&PinIndexRelated,&PhysicalType);
			if(PhysConn_Video_Composite==PhysicalType) 
			{
				inPort = i;
				break;
			}
		}
		for( i =0;i<OutputPinCount;i++)
		{
			pXBar1->get_CrossbarPinInfo(FALSE,i,&PinIndexRelated,&PhysicalType);
			if(PhysConn_Video_VideoDecoder==PhysicalType) 
			{
				outPort = i;
				break;
			}
		}
  
		if(S_OK==pXBar1->CanRoute(outPort,inPort))
		{
			pXBar1->Route(outPort,inPort);
		}
		pXBar1->Release();  
	}
	pBuilder->Release();
}

/*
The returned image can not be released.
*/
#ifdef _JACKY_USING_OPENCV_
IplImage* CCameraCaptureDShow::QueryFrame()
#else
BYTE* CCameraCaptureDShow::QueryFrame(long *nsize)
#endif
{
	if (!m_bConnected)
	  return NULL;
	long evCode;
	long size = 0;
	HRESULT hr;
	/*HRESULT hr = pSeeking->SetPositions(frame, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
	if(FAILED(hr))
	{
		goto done;
	}*/
	//if (!graphStatus)
	//{
	//   m_pMediaControl->Run();
	//   graphStatus = true;
	//}
	if (0 == m_samplingMode) {
			hr = m_pMediaEvent->WaitForCompletion(INFINITE, &evCode);
		/*if (FAILED(hr))
		{
			printf("caputre av error evcode=%d\n",evCode);
		}*/
		hr = m_pSampleGrabber->GetCurrentBuffer(&size, NULL);
		if (FAILED(hr))
		{
			*nsize = 0;
			return NULL;
		}
	}

	//if the buffer size changed
	if (size != m_nBufferSize)
	{
#ifdef _JACKY_USING_OPENCV_
		if (m_pFrame)
			cvReleaseImage(&m_pFrame);
		m_nBufferSize = size;
		m_pFrame = cvCreateImage(cvSize(m_nWidth, m_nHeight), IPL_DEPTH_8U, 3);
#else
		if (m_pFrame)
		{
			delete[] m_pFrame;
			m_pFrame = NULL;
		}
		m_nBufferSize = size;
		m_pFrame = new BYTE[m_nBufferSize];
#endif
	}

#ifdef _JACKY_USING_OPENCV_
	m_pSampleGrabber->GetCurrentBuffer(&m_nBufferSize, (long*)m_pFrame->imageData);
	cvFlip(m_pFrame);
#else
	if (0 == m_samplingMode) {
		hr = m_pSampleGrabber->GetCurrentBuffer(&m_nBufferSize, (long*)m_pFrame);
		if (FAILED(hr))
		{
			*nsize = 0;
			return NULL;
		}	
	}
#endif
	if (nsize != NULL)
		*nsize = m_nBufferSize;

	return m_pFrame;
}

int CCameraCaptureDShow::CameraCount()
{
	int count = 0;
 	//CoInitialize(NULL);

   // enumerate all video capture devices
	CComPtr<ICreateDevEnum> pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
									IID_ICreateDevEnum, (void**)&pCreateDevEnum);

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
        &pEm, 0);
    if (hr != NOERROR) 
	{
		return count;
    }

    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;
    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
		count++;
		pM->Release();
    }

	pCreateDevEnum = NULL;
	pEm = NULL;
	return count;
}

std::string CCameraCaptureDShow::CameraName(int nCamID)
{
	std::string res = "";
	int count = 0;
 	//CoInitialize(NULL);

   // enumerate all video capture devices
	CComPtr<ICreateDevEnum> pCreateDevEnum;
    HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
									IID_ICreateDevEnum, (void**)&pCreateDevEnum);

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
        &pEm, 0);
    if (hr != NOERROR) return res;


    pEm->Reset();
    ULONG cFetched;
    IMoniker *pM;

    while(hr = pEm->Next(1, &pM, &cFetched), hr==S_OK)
    {
		if (count == nCamID)
		{
			IPropertyBag *pBag=0;
			hr = pM->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
			if(SUCCEEDED(hr))
			{
				VARIANT var;
				var.vt = VT_BSTR;
				hr = pBag->Read(L"FriendlyName", &var, NULL); //还有其他属性,像描述信息等等...
	            if(hr == NOERROR)
		        {
                    wchar_t* wstr = static_cast<wchar_t*>(var.bstrVal);
					int pSize = WideCharToMultiByte(CP_OEMCP, 0, wstr, wcslen(wstr), NULL, 0, NULL, NULL);
					char* pCStrKey = new char[pSize+1];
					WideCharToMultiByte(CP_OEMCP, 0, wstr, wcslen(wstr), pCStrKey, pSize, NULL, NULL);
					pCStrKey[pSize] = '\0';
					res = pCStrKey;
                    //  char* str = static_cast<char*>(var.bstrVal);
                    //strncpy_s(sName, strlen(strName) + 1, strName, nBufferSize);
                    SysFreeString(var.bstrVal);
		        }
			    pBag->Release();
			}
			pM->Release();

			break;
		}
		count++;
    }

	pCreateDevEnum = NULL;
	pEm = NULL;

	return res;
}

int CCameraCaptureDShow::ShowCaptureProperty(int nCamID)
{

	return 0;
}

