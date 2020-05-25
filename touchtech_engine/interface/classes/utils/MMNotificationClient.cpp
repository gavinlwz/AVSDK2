#ifdef WIN32

#include <tsk_debug.h>
#include <YouMeVoiceEngine.h>

#include "MMNotificationClient.h"


bool CMMNotificationClient::m_bInited = false;

void CMMNotificationClient::Init()
{
	if (m_bInited) {
		return;
	}
	m_bInited = true;

	IMMDeviceEnumerator *pMMDeviceEnumerator = NULL;
	HRESULT hr;
	CoInitialize(NULL);
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
		NULL, CLSCTX_INPROC_SERVER,
		__uuidof(IMMDeviceEnumerator),
		(void**)&pMMDeviceEnumerator);
	if (S_OK != hr)	{
		TSK_DEBUG_INFO("Failed to create MMDeviceEnumerator");
		return;
	}

	IMMNotificationClient *pMMNotifyClient = new CMMNotificationClient();
	if (pMMDeviceEnumerator && pMMNotifyClient)
	{
		hr = pMMDeviceEnumerator->RegisterEndpointNotificationCallback(pMMNotifyClient);
		if (S_OK == hr)
		{
			TSK_DEBUG_INFO("RegisterEndpointNotificationCallback OK");
		}
		else
		{
			TSK_DEBUG_INFO("RegisterEndpointNotificationCallback failed");
		}
	}
}

CMMNotificationClient::CMMNotificationClient() :
_cRef(1)
{
}


CMMNotificationClient::~CMMNotificationClient()
{
}

// IUnknown methods -- AddRef, Release, and QueryInterface

ULONG STDMETHODCALLTYPE CMMNotificationClient::AddRef()
{
	return InterlockedIncrement(&_cRef);
}

ULONG STDMETHODCALLTYPE CMMNotificationClient::Release()
{
	ULONG ulRef = InterlockedDecrement(&_cRef);
	if (0 == ulRef)
	{
		delete this;
	}
	return ulRef;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::QueryInterface(
	REFIID riid, VOID **ppvInterface)
{
	if (IID_IUnknown == riid)
	{
		AddRef();
		*ppvInterface = (IUnknown*)this;
	}
	else if (__uuidof(IMMNotificationClient) == riid)
	{
		AddRef();
		*ppvInterface = (IMMNotificationClient*)this;
	}
	else
	{
		*ppvInterface = NULL;
		return E_NOINTERFACE;
	}
	return S_OK;
}

// Callback methods for device-event notifications.

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDefaultDeviceChanged(
	EDataFlow flow, ERole role,
	LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceAdded(LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceRemoved(LPCWSTR pwstrDeviceId)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnDeviceStateChanged(
	LPCWSTR pwstrDeviceId,
	DWORD dwNewState)
{
	char  *pszState = "?????";

	switch (dwNewState)
	{
	case DEVICE_STATE_ACTIVE:
		pszState = "ACTIVE";
		CYouMeVoiceEngine::getInstance()->pauseChannel(false);
		CYouMeVoiceEngine::getInstance()->resumeChannel(false);
		break;
	case DEVICE_STATE_DISABLED:
		pszState = "DISABLED";
		break;
	case DEVICE_STATE_NOTPRESENT:
		pszState = "NOTPRESENT";
		break;
	case DEVICE_STATE_UNPLUGGED:
		pszState = "UNPLUGGED";
		break;
	}

	TSK_DEBUG_INFO("Win32 device state changed:%s", pszState);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CMMNotificationClient::OnPropertyValueChanged(
	LPCWSTR pwstrDeviceId,
	const PROPERTYKEY key)
{
	return S_OK;
}


#endif // WIN32
