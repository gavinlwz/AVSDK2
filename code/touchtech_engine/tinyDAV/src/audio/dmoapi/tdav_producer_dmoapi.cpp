/*
* Copyright (C) 2010-2015 Mamadou DIOP.
*
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file tdav_producer_dmoapi.c
* @brief Audio Producer for Win32 and WinCE platforms.
*/
#include "tinydav/audio/dmoapi/tdav_producer_dmoapi.h"

#include <windows.h>
#include <dmo.h>
#include <Mmsystem.h>
#include <objbase.h>
#include <mediaobj.h>
#include <uuids.h>
#include <propidl.h>
#include <wmcodecdsp.h>

#include <atlbase.h>
#include <ATLComCli.h>
#include <audioclient.h>
#include <MMDeviceApi.h>
#include <AudioEngineEndPoint.h>
//#include <DeviceTopology.h>
#include <propkey.h>
#include <strsafe.h>
#include <conio.h>


#include "tsk_thread.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_string.h"
#include "YouMeConstDefine.h"
#include "tinydav/codecs/mixer/speex_resampler.h"
#include "CrossPlatformDefine/PlatformDef.h"
#include "webrtc/common_audio/ring_buffer.h"

#define TDAV_DMOAPI_PRODUCER_BUFFER_DELAY_MS		40


#define DMO_SAFE_DELETE(p) {if (p) delete p; (p) = NULL;}
#define DMO_SAFE_ARRAYDELETE(p) {if (p) delete[] (p); (p) = NULL;}
#define DMO_SAFE_RELEASE(p) {if (NULL != p) {(p)->Release(); (p) = NULL;}}

#define DMO_VBFALSE (VARIANT_BOOL)0
#define DMO_VBTRUE  (VARIANT_BOOL)-1

#define DMO_CHECK_RET(hr, message) if (FAILED(hr)) { TSK_DEBUG_ERROR(message); goto bail;}
#define DMO_CHECKHR(x, message) hr = x; if (FAILED(hr)) {TSK_DEBUG_ERROR("message: , %08X\n", message, hr); goto bail;}
#define DMO_CHECK_ALLOC(pb, message) if (NULL == pb) { TSK_DEBUG_ERROR(message); goto bail;}


class CBaseMediaBuffer : public IMediaBuffer {
public:
	CBaseMediaBuffer() {}
	CBaseMediaBuffer(BYTE *pData, ULONG ulSize, ULONG ulData) :
		m_pData(pData), m_ulSize(ulSize), m_ulData(ulData), m_cRef(1) {}
	STDMETHODIMP_(ULONG) AddRef() {
		return InterlockedIncrement((long*)&m_cRef);
	}
	STDMETHODIMP_(ULONG) Release() {
		long l = InterlockedDecrement((long*)&m_cRef);
		if (l == 0)
			delete this;
		return l;
	}
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv) {
		if (riid == IID_IUnknown) {
			AddRef();
			*ppv = (IUnknown*)this;
			return NOERROR;
		}
		else if (riid == IID_IMediaBuffer) {
			AddRef();
			*ppv = (IMediaBuffer*)this;
			return NOERROR;
		}
		else
			return E_NOINTERFACE;
	}
	STDMETHODIMP SetLength(DWORD ulLength) { m_ulData = ulLength; return NOERROR; }
	STDMETHODIMP GetMaxLength(DWORD *pcbMaxLength) { *pcbMaxLength = m_ulSize; return NOERROR; }
	STDMETHODIMP GetBufferAndLength(BYTE **ppBuffer, DWORD *pcbLength) {
		if (ppBuffer) *ppBuffer = m_pData;
		if (pcbLength) *pcbLength = m_ulData;
		return NOERROR;
	}
protected:
	BYTE *m_pData;
	ULONG m_ulSize;
	ULONG m_ulData;
	ULONG m_cRef;
};

class CStaticMediaBuffer : public CBaseMediaBuffer {
public:
	STDMETHODIMP_(ULONG) AddRef() { return 2; }
	STDMETHODIMP_(ULONG) Release() { return 1; }
	void Init(BYTE *pData, ULONG ulSize, ULONG ulData) {
		m_pData = pData;
		m_ulSize = ulSize;
		m_ulData = ulData;
	}
};

typedef struct tdav_producer_dmoapi_s {
	TDAV_DECLARE_PRODUCER_AUDIO;

	tsk_bool_t started;
	tsk_bool_t bCallbackThreadRunning;

	WAVEFORMATEX wfx;

	IMediaObject* pDMO = NULL;
	IPropertyStore* pPS = NULL;

	CStaticMediaBuffer* pOutputBuffer;
	DMO_OUTPUT_DATA_BUFFER OutputBufferStruct;
	DMO_MEDIA_TYPE mt;

	ULONG cbProduced;
	DWORD dwStatus;

	DWORD cOutputBufLen;
	BYTE *pbOutputBuffer = NULL;

	tsk_size_t bytes_per_notif;
	tsk_size_t bytes_per_output_frame;

	void* tid[2];
	HANDLE events[3];
	CRITICAL_SECTION csCallback;
	CRITICAL_SECTION csBuffer;

	SpeexResamplerState*    pResampler;
	void*                   pResampleBuf;
	tsk_size_t              nResampleBufSize;

	RingBuffer*	            pRingBuffer;

	tsk_bool_t              bFakedRecording;
	tsk_bool_t              isMuted;
} tdav_producer_dmoapi_t;


static void* TSK_STDCALL __callback_thread(void *param)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)param;
	DWORD dwEvent;

	TSK_DEBUG_INFO("__record_callback_thread -- START");

	void *pOutputBuf = (void *)tsk_calloc(1, producer->bytes_per_output_frame);
	if (NULL == pOutputBuf) {
		TSK_DEBUG_ERROR("failed to alloc pOutputBuf");
		return NULL;
	}
	
	for (;;) {
		if (!producer->bCallbackThreadRunning) {
			break;
		}
		dwEvent = WaitForSingleObject(producer->events[2], TMEDIA_PRODUCER(producer)->audio.ptime);
		if (WAIT_TIMEOUT == dwEvent) {

			if (TMEDIA_PRODUCER(producer)->enc_cb.callback) {
				void *outData = NULL;
				EnterCriticalSection(&producer->csBuffer);

				if (WebRtc_youme_available_read(producer->pRingBuffer) >= producer->bytes_per_output_frame / producer->wfx.nBlockAlign) {
					WebRtc_youme_ReadBuffer(producer->pRingBuffer, (void**)&outData, pOutputBuf, producer->bytes_per_output_frame / producer->wfx.nBlockAlign);
					if (outData != pOutputBuf) {
						// ReadBuffer() hasn't copied to |out| in this case.
						memcpy(pOutputBuf, outData, producer->bytes_per_output_frame);
					}
					TMEDIA_PRODUCER(producer)->enc_cb.callback(TMEDIA_PRODUCER(producer)->enc_cb.callback_data, pOutputBuf, producer->bytes_per_output_frame);
				}

#if 0
				{
					static FILE* f2 = NULL;
					if (!f2) f2 = fopen("./dmoapi_producer2.pcm", "wb");
					fwrite(pOutputBuf, 1, producer->bytes_per_output_frame, f2);
				}
#endif

				LeaveCriticalSection(&producer->csBuffer);
			}
		}
		else {
			break;
		}
	}
	TSK_DEBUG_INFO("__record_callback_thread -- STOP");
	TSK_FREE(pOutputBuf);
	pOutputBuf = NULL;

	return NULL;
}

static void* TSK_STDCALL __record_thread(void *param)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)param;

	DWORD dwEvent;

	TSK_DEBUG_INFO("__record_collect_thread -- START");

	/*BOOL iRet = SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	if (0 == iRet)
	{
	// call getLastError.
	puts("failed to set process priority\n");
	goto bail;
	}*/
	// SetPriorityClass(GetCurrentThread(), REALTIME_PRIORITY_CLASS);

	for (;;) {
		if (!producer->started) {
			return tsk_null;
		}
		dwEvent = WaitForSingleObject(producer->events[0], TMEDIA_PRODUCER(producer)->audio.ptime);


		if (!producer->bCallbackThreadRunning) {
			EnterCriticalSection(&producer->csBuffer);
			int awailable = WebRtc_youme_available_read(producer->pRingBuffer);
			LeaveCriticalSection(&producer->csBuffer);
			if (awailable > TDAV_DMOAPI_PRODUCER_BUFFER_DELAY_MS / TMEDIA_PRODUCER(producer)->audio.ptime * producer->bytes_per_output_frame) {
				tsk_thread_create(&producer->tid[1], __callback_thread, producer);
				producer->bCallbackThreadRunning = true;
			}
		}
		if (WAIT_TIMEOUT == dwEvent) {
			if (TMEDIA_PRODUCER(producer)->enc_cb.callback) {
				do {
					producer->pOutputBuffer->Init((byte*)(producer->pbOutputBuffer), producer->cOutputBufLen, 0);
					producer->OutputBufferStruct.dwStatus = 0;
					HRESULT hr = S_OK;
					hr = producer->pDMO->ProcessOutput(0, 1, &(producer->OutputBufferStruct), &(producer->OutputBufferStruct.dwStatus));
					DMO_CHECK_RET(hr, "ProcessOutput failed");

					if (hr == S_FALSE) {
						producer->cbProduced = 0;
					}
					else {
						hr = producer->pOutputBuffer->GetBufferAndLength(NULL, &(producer->cbProduced));
						DMO_CHECK_RET(hr, "GetBufferAndLength failed");
					}
				} while (producer->OutputBufferStruct.dwStatus & DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE);

				if (producer->pResampler) {
					uint32_t bytesPerSample = TMEDIA_PRODUCER(producer)->audio.channels * TMEDIA_PRODUCER(producer)->audio.bits_per_sample / 8;
					uint32_t inSampleNum = producer->cbProduced / bytesPerSample;

					uint32_t outSampleNum = producer->cbProduced * TMEDIA_PRODUCER(producer)->audio.rate / producer->wfx.nSamplesPerSec / bytesPerSample;
					uint32_t frameSizeInByte = outSampleNum * bytesPerSample;
					EnterCriticalSection(&producer->csBuffer);

					if (producer->nResampleBufSize < frameSizeInByte) {
						producer->pResampleBuf = tsk_realloc(producer->pResampleBuf, frameSizeInByte);
						memset(producer->pResampleBuf, 0, frameSizeInByte);
						producer->nResampleBufSize = frameSizeInByte;
					}
					
					speex_resampler_process_int(producer->pResampler, 0,
						(const spx_int16_t*)(producer->pbOutputBuffer), (unsigned int*)&inSampleNum,
						(spx_int16_t*)(producer->pResampleBuf), &outSampleNum);
					if (producer->isMuted) {
						memset(producer->pResampleBuf, 0, frameSizeInByte);
					}

					WebRtc_youme_WriteBuffer(producer->pRingBuffer, producer->pResampleBuf, outSampleNum);
					LeaveCriticalSection(&producer->csBuffer);

				}
				else {
					uint32_t outSampleNum = producer->cbProduced * TMEDIA_PRODUCER(producer)->audio.rate / producer->wfx.nSamplesPerSec / producer->wfx.nBlockAlign;
					if (producer->isMuted) {
						memset(producer->pbOutputBuffer, 0, producer->cbProduced);
					}
					EnterCriticalSection(&producer->csBuffer);
					if (WebRtc_youme_available_write(producer->pRingBuffer) <= producer->bytes_per_output_frame) {
						TSK_DEBUG_WARN("available write is not enough, reset ringbuffer!!");
						WebRtc_youme_InitBuffer(producer->pRingBuffer);
					}
					WebRtc_youme_WriteBuffer(producer->pRingBuffer, producer->pbOutputBuffer, outSampleNum);
					LeaveCriticalSection(&producer->csBuffer);
				}
				continue;
			bail:
				memset(producer->pbOutputBuffer, 0, producer->bytes_per_output_frame);
				EnterCriticalSection(&producer->csBuffer);
				WebRtc_youme_WriteBuffer(producer->pRingBuffer, producer->pbOutputBuffer, producer->bytes_per_output_frame / producer->wfx.nBlockAlign);
				LeaveCriticalSection(&producer->csBuffer);
			}
		}
		else {
			break;
		}
	}

	TSK_DEBUG_INFO("__record_collect_thread -- STOP");

	return tsk_null;
}

// If failed to start recording, fake the recording by outputting silence data.
static void* TSK_STDCALL __faked_record_thread(void *param)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)param;
	DWORD dwEvent;

	TSK_DEBUG_INFO("__faked_record_thread -- START");

	if (NULL == producer) {
		TSK_DEBUG_ERROR("producer is NULL");
		return NULL;
	}

	uint8_t *pSilenceBuf = (uint8_t *)tsk_calloc(1, producer->bytes_per_output_frame);
	if (NULL == pSilenceBuf) {
		TSK_DEBUG_ERROR("failed to alloc pSilenceBuf");
		return NULL;
	}

	for (;;) {
		dwEvent = WaitForSingleObject(producer->events[1], TMEDIA_PRODUCER(producer)->audio.ptime);

		if (WAIT_TIMEOUT == dwEvent) {
			memset(pSilenceBuf, 0, producer->bytes_per_output_frame);
			EnterCriticalSection(&producer->csCallback);
			TMEDIA_PRODUCER(producer)->enc_cb.callback(TMEDIA_PRODUCER(producer)->enc_cb.callback_data, pSilenceBuf, producer->bytes_per_output_frame);
			LeaveCriticalSection(&producer->csCallback);
		}
		else {
			break;
		}
	}

	TSK_FREE(pSilenceBuf);
	pSilenceBuf = NULL;
	TSK_DEBUG_INFO("__faked_record_thread() -- STOP");


	return tsk_null;
}

/* ============ Media Producer Interface ================= */
static int tdav_producer_dmoapi_get(tmedia_producer_t *_self, tmedia_param_t *param)
{
	tdav_producer_dmoapi_t *self = (tdav_producer_dmoapi_t *)_self;
	if (NULL == self) {
		return -1;
	}

	if (param->plugin_type == tmedia_ppt_producer)
	{
		if (param->value_type == tmedia_pvt_int32)
		{
			if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR)) {
				if (self->bFakedRecording) {
					*(int32_t*)param->value = YOUME_ERROR_REC_OTHERS;
				}
				return 0;
			}
			else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_RECORDING_ERROR_EXTRA)) {
				*(int32_t*)param->value = 0;
				return 0;
			}
		}
	}

	return -1;
}

static int tdav_producer_dmoapi_set(tmedia_producer_t *_self, const tmedia_param_t *param)
{
	tdav_producer_dmoapi_t *self = (tdav_producer_dmoapi_t *)_self;
	if (NULL == self) {
		return -1;
	}

	if (param->plugin_type == tmedia_ppt_producer)
	{
		if (param->value_type == tmedia_pvt_int32)
		{
			if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_MICROPHONE_MUTE))
			{
				self->isMuted = (*((int32_t *)param->value) != 0);
				TSK_DEBUG_INFO("Set mic mute:%d", self->isMuted);
				return 0;
			}
			else if (tsk_striequals(param->key, TMEDIA_PARAM_KEY_MIC_VOLUME))
			{
				return -1;
			}
		}
	}

	return tdav_producer_audio_set(TDAV_PRODUCER_AUDIO(self), param);
}

extern "C" int tdav_producer_dmoapi_issupported() {

	// 1) Check if Windows version is Vista SP1 or later.
	//
	// CoreAudio is only available on Vista SP1 and later.
	//
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_LESS_EQUAL;
	HRESULT hr = S_OK;

	IMediaObject* pDMO = NULL;
	IPropertyStore* pPS = NULL;

	// Initialize the OSVERSIONINFOEX structure.
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 0;
	osvi.wServicePackMajor = 0;
	osvi.wServicePackMinor = 0;
	osvi.wProductType = VER_NT_WORKSTATION;

	// Initialize the condition mask.
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, op);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMINOR, op);
	VER_SET_CONDITION(dwlConditionMask, VER_PRODUCT_TYPE, VER_EQUAL);

	DWORD dwTypeMask = VER_MAJORVERSION | VER_MINORVERSION |
		VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR |
		VER_PRODUCT_TYPE;

	// Perform the test.
	BOOL isVistaRTMorXP = VerifyVersionInfo(&osvi, dwTypeMask,
		dwlConditionMask);
	if (isVistaRTMorXP != 0)
	{
		TSK_DEBUG_INFO("*** Windows Core Audio is only supported on Vista SP1 or later "
			"=> will revert to the Wave API ***");
		return 0;
	}

	// Check if the DMO API is available.
	DMO_CHECKHR(CoCreateInstance(CLSID_CWMAudioAEC, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&(pDMO)), "CoCreateInstance failed");
	DMO_CHECKHR(pDMO->QueryInterface(IID_IPropertyStore, (void**)&(pPS)), "QueryInterface failed");

	DMO_SAFE_RELEASE(pDMO);
	DMO_SAFE_RELEASE(pPS);

	TSK_DEBUG_INFO("*** Windows Core Audio is supported ***");

	return 1;

bail:
	TSK_DEBUG_INFO("*** Failed to create the required COM object ***");
	TSK_DEBUG_INFO("*** Windows Core Audio is NOT supported => will revert to the Wave API ***");

	DMO_SAFE_RELEASE(pDMO);
	DMO_SAFE_RELEASE(pPS);

	return 0;
}

int tdav_producer_dmoapi_prepare(tmedia_producer_t* self, const tmedia_codec_t* codec)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)self;

	if (!producer || !codec && codec->plugin) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	TMEDIA_PRODUCER(producer)->audio.channels = TMEDIA_CODEC_CHANNELS_AUDIO_ENCODING(codec);
	TMEDIA_PRODUCER(producer)->audio.rate = TMEDIA_CODEC_RATE_ENCODING(codec);
	TMEDIA_PRODUCER(producer)->audio.ptime = TMEDIA_CODEC_PTIME_AUDIO_ENCODING(codec);

	// DMO output format sample rate must be one of 16000/8000/11025/22050
	uint32_t inSampleRate = 16000;

	if (producer->pResampler) {
		speex_resampler_destroy(producer->pResampler);
		producer->pResampler = NULL;
	}
	if (producer->pResampleBuf) {
		TSK_SAFE_FREE(producer->pResampleBuf);
		producer->pResampleBuf = NULL;
	}
	if (TMEDIA_PRODUCER(producer)->audio.rate != inSampleRate) {
		producer->nResampleBufSize = (TMEDIA_PRODUCER(producer)->audio.rate
			* TMEDIA_PRODUCER(producer)->audio.channels
			* (TMEDIA_PRODUCER(producer)->audio.bits_per_sample / 8)
			* TMEDIA_PRODUCER(producer)->audio.ptime) / 1000;

		producer->pResampleBuf = tsk_calloc(1, producer->nResampleBufSize);
		producer->pResampler = speex_resampler_init(TMEDIA_PRODUCER(producer)->audio.channels,
			inSampleRate,
			TMEDIA_PRODUCER(producer)->audio.rate,
			3,
			NULL);
		if (producer->pResampler) {
			//speex_resampler_skip_zeros(producer->pResampler);
		}
	}

	if (producer->pbOutputBuffer) {
		DMO_SAFE_ARRAYDELETE(producer->pbOutputBuffer);
		producer->pbOutputBuffer = NULL;
	}
	if (producer->pDMO) {
		DMO_SAFE_RELEASE(producer->pDMO);
		producer->pDMO = NULL;
	}
	if (producer->pPS) {
		DMO_SAFE_RELEASE(producer->pPS);
		producer->pPS = NULL;
	}


	/* Format */
	ZeroMemory(&producer->wfx, sizeof(WAVEFORMATEX));
	producer->wfx.wFormatTag = WAVE_FORMAT_PCM;
	producer->wfx.nChannels = TMEDIA_PRODUCER(producer)->audio.channels;
	producer->wfx.nSamplesPerSec = inSampleRate; // TMEDIA_PRODUCER(producer)->audio.rate;
	producer->wfx.wBitsPerSample = TMEDIA_PRODUCER(producer)->audio.bits_per_sample;
	producer->wfx.nBlockAlign = (producer->wfx.nChannels * producer->wfx.wBitsPerSample / 8);
	producer->wfx.nAvgBytesPerSec = (producer->wfx.nSamplesPerSec * producer->wfx.nBlockAlign);

	/* Average bytes (count) for each notification */
	producer->bytes_per_notif = ((producer->wfx.nAvgBytesPerSec * TMEDIA_PRODUCER(producer)->audio.ptime) / 1000);
	producer->bytes_per_output_frame = (producer->bytes_per_notif * TMEDIA_PRODUCER(producer)->audio.rate) / inSampleRate;

	return 0;
}

int tdav_producer_dmoapi_start(tmedia_producer_t* self)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)self;

	if (!producer) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if (producer->started) {
		TSK_DEBUG_WARN("Producer already started");
		return 0;
	}

	/* create events */
	if (!producer->events[0]) {
		producer->events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	if (!producer->events[1]) {
		producer->events[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
	if (!producer->events[2]) {
		producer->events[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
	}

	HRESULT hr = S_OK;

	// Parameters to config DMO
	int  iSystemMode = 0;    		// AEC-MicArray system mode. The valid modes are
									//   SINGLE_CHANNEL_AEC = 0
									//   OPTIBEAM_ARRAY_ONLY = 2
									//   OPTIBEAM_ARRAY_AND_AEC = 4
									//   SINGLE_CHANNEL_NSAGC = 5


	int  iMicDevIdx = -1;           // microphone device index
	int  iSpkDevIdx = -1;         	// speaker device index
	BOOL bFeatrModeOn = 0;      	// turn feature mode on/off
	BOOL bNoiseSup = 1;            	// turn noise suppression on/off
	BOOL bAGC = 1;                 	// turn digital auto gain control on/off
	BOOL bCntrClip = 0;           	// turn center clippng on/off

	// DMO initialization
	DMO_CHECKHR(CoCreateInstance(CLSID_CWMAudioAEC, NULL, CLSCTX_INPROC_SERVER, IID_IMediaObject, (void**)&(producer->pDMO)), "CoCreateInstance failed");
	DMO_CHECKHR(producer->pDMO->QueryInterface(IID_IPropertyStore, (void**)&(producer->pPS)), "QueryInterface failed");

	// Set AEC mode and other parameters
	// Not all user changeable options are given in this sample code.
	// Please refer to readme.txt for more options.

	// Set AEC-MicArray DMO system mode.
	// This must be set for the DMO to work properly
	//TSK_DEBUG_INFO("AEC settings:");
	PROPVARIANT pvSysMode;
	PropVariantInit(&pvSysMode);
	pvSysMode.vt = VT_I4;
	pvSysMode.lVal = (LONG)(iSystemMode);
	DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, pvSysMode), "SetValue MFPKEY_WMAAECMA_SYSTEM_MODE failed");
	DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_SYSTEM_MODE, &pvSysMode), "GetValue MFPKEY_WMAAECMA_SYSTEM_MODE failed");
	//TSK_DEBUG_INFO("%20s %5d \n", "System Mode is", pvSysMode.lVal);
	PropVariantClear(&pvSysMode);

	// Tell DMO which capture and render device to use 
	// This is optional. If not specified, default devices will be used
	if (iMicDevIdx >= 0 || iSpkDevIdx >= 0)
	{
		PROPVARIANT pvDeviceId;
		PropVariantInit(&pvDeviceId);
		pvDeviceId.vt = VT_I4;
		pvDeviceId.lVal = (unsigned long)(iSpkDevIdx << 16) + (unsigned long)(0x0000ffff & iMicDevIdx);
		DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_DEVICE_INDEXES, pvDeviceId), "SetValue MFPKEY_WMAAECMA_DEVICE_INDEXES failed");
		DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_DEVICE_INDEXES, &pvDeviceId), "GetValue MFPKEY_WMAAECMA_DEVICE_INDEXES failed");
		PropVariantClear(&pvDeviceId);
	}

	if (bFeatrModeOn)
	{
		// Turn on feature modes
		PROPVARIANT pvFeatrModeOn;
		PropVariantInit(&pvFeatrModeOn);
		pvFeatrModeOn.vt = VT_BOOL;
		pvFeatrModeOn.boolVal = bFeatrModeOn ? DMO_VBTRUE : DMO_VBFALSE;
		DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_FEATURE_MODE, pvFeatrModeOn), "SetValue MFPKEY_WMAAECMA_FEATURE_MODE failed");
		DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_FEATURE_MODE, &pvFeatrModeOn), "GetValue MFPKEY_WMAAECMA_FEATURE_MODE failed");
		TSK_DEBUG_INFO("%20s %5d \n", "Feature Mode is", pvFeatrModeOn.boolVal);
		PropVariantClear(&pvFeatrModeOn);

		// Turn on/off noise suppression
		PROPVARIANT pvNoiseSup;
		PropVariantInit(&pvNoiseSup);
		pvNoiseSup.vt = VT_I4;
		pvNoiseSup.lVal = (LONG)bNoiseSup;
		DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_FEATR_NS, pvNoiseSup), "SetValue MFPKEY_WMAAECMA_FEATR_NS failed");
		DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_FEATR_NS, &pvNoiseSup), "GetValue MFPKEY_WMAAECMA_FEATR_NS failed");
		TSK_DEBUG_INFO("%20s %5d \n", "Noise suppresion is", pvNoiseSup.lVal);
		PropVariantClear(&pvNoiseSup);

		// Turn on/off AGC
		PROPVARIANT pvAGC;
		PropVariantInit(&pvAGC);
		pvAGC.vt = VT_BOOL;
		pvAGC.boolVal = bAGC ? DMO_VBTRUE : DMO_VBFALSE;
		DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_FEATR_AGC, pvAGC), "SetValue MFPKEY_WMAAECMA_FEATR_AGC failed");
		DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_FEATR_AGC, &pvAGC), "GetValue MFPKEY_WMAAECMA_FEATR_AGC failed");
		TSK_DEBUG_INFO("%20s %5d \n", "AGC is", pvAGC.boolVal);
		PropVariantClear(&pvAGC);

		// Turn on/off center clip
		PROPVARIANT pvCntrClip;
		PropVariantInit(&pvCntrClip);
		pvCntrClip.vt = VT_BOOL;
		pvCntrClip.boolVal = bCntrClip ? DMO_VBTRUE : DMO_VBFALSE;
		DMO_CHECKHR(producer->pPS->SetValue(MFPKEY_WMAAECMA_FEATR_CENTER_CLIP, pvCntrClip), "SetValue MFPKEY_WMAAECMA_FEATR_CENTER_CLIP failed");
		DMO_CHECKHR(producer->pPS->GetValue(MFPKEY_WMAAECMA_FEATR_CENTER_CLIP, &pvCntrClip), "GetValue MFPKEY_WMAAECMA_FEATR_CENTER_CLIP failed");
		TSK_DEBUG_INFO("%20s %5d \n", "Center clip is", (BOOL)pvCntrClip.boolVal);
		PropVariantClear(&pvCntrClip);
	}

	DMO_SAFE_DELETE(producer->pOutputBuffer);

	producer->pOutputBuffer = new CStaticMediaBuffer();
	producer->OutputBufferStruct = { 0 };
	producer->OutputBufferStruct.pBuffer = producer->pOutputBuffer;
	producer->mt = { 0 };

	// Set DMO output format
	hr = MoInitMediaType(&(producer->mt), sizeof(WAVEFORMATEX));
	DMO_CHECK_RET(hr, "MoInitMediaType failed");

	producer->mt.majortype = MEDIATYPE_Audio;
	producer->mt.subtype = MEDIASUBTYPE_PCM;
	producer->mt.lSampleSize = 0;
	producer->mt.bFixedSizeSamples = TRUE;
	producer->mt.bTemporalCompression = FALSE;
	producer->mt.formattype = FORMAT_WaveFormatEx;
	memcpy(producer->mt.pbFormat, &producer->wfx, sizeof(WAVEFORMATEX));

	hr = producer->pDMO->SetOutputType(0, &(producer->mt), 0);
	DMO_CHECK_RET(hr, "SetOutputType failed");
	MoFreeMediaType(&(producer->mt));

	// Allocate streaming resources. This step is optional. If it is not called here, it
	// will be called when first time ProcessInput() is called. However, if you want to 
	// get the actual frame size being used, it should be called explicitly here.
	hr = producer->pDMO->AllocateStreamingResources();
	DMO_CHECK_RET(hr, "AllocateStreamingResources failed");

	// allocate output buffer
	producer->cOutputBufLen = producer->wfx.nSamplesPerSec * producer->wfx.nBlockAlign;
	producer->pbOutputBuffer = new BYTE[producer->cOutputBufLen];
	DMO_CHECK_ALLOC(producer->pbOutputBuffer, "out of memory.\n");

	if (producer->pRingBuffer) {
		WebRtc_youme_FreeBuffer(producer->pRingBuffer);
		producer->pRingBuffer = NULL;
	}
	producer->pRingBuffer = WebRtc_youme_CreateBuffer(1 * producer->wfx.nSamplesPerSec, producer->wfx.nBlockAlign);
	if (producer->pRingBuffer == NULL) {
		goto bail;
	}

	/* start thread */
	producer->started = tsk_true;
	tsk_thread_create(&producer->tid[0], __record_thread, producer);

	return 0;

bail:
	DMO_SAFE_ARRAYDELETE(producer->pbOutputBuffer);

	DMO_SAFE_RELEASE(producer->pDMO);
	DMO_SAFE_RELEASE(producer->pPS);

	CoUninitialize();

	/* start thread */
	producer->started = tsk_true;
	producer->bFakedRecording = tsk_true;
	tsk_thread_create(&producer->tid[0], __faked_record_thread, producer);

	return 0;
}

int tdav_producer_dmoapi_pause(tmedia_producer_t* self)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)self;

	if (!producer) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	return 0;
}

int tdav_producer_dmoapi_stop(tmedia_producer_t* self)
{
	tdav_producer_dmoapi_t* producer = (tdav_producer_dmoapi_t*)self;

	if (!self) {
		TSK_DEBUG_ERROR("Invalid parameter");
		return -1;
	}

	if (!producer->started) {
		TSK_DEBUG_WARN("Producer not started");
		return 0;
	}

	/* stop thread */
	if (producer->tid[0]) {
		SetEvent(producer->events[0]);
		SetEvent(producer->events[1]);
		tsk_thread_join(&(producer->tid[0]));
	}
	if (producer->tid[1]) {
		producer->bCallbackThreadRunning = false;
		SetEvent(producer->events[2]);
		tsk_thread_join(&(producer->tid[1]));
	}

	/* should be done here */
	producer->started = tsk_false;

	DMO_SAFE_ARRAYDELETE(producer->pbOutputBuffer);

	if (producer->pRingBuffer) {
		WebRtc_youme_FreeBuffer(producer->pRingBuffer);
		producer->pRingBuffer = NULL;
	}

	DMO_SAFE_RELEASE(producer->pDMO);
	DMO_SAFE_RELEASE(producer->pPS);

	DMO_SAFE_DELETE(producer->pOutputBuffer);

	CoUninitialize();


	return 0;
}


//
//	WaveAPI producer object definition
//
/* constructor */
static tsk_object_t* tdav_producer_dmoapi_ctor(tsk_object_t * self, va_list * app)
{
	tdav_producer_dmoapi_t *producer = (tdav_producer_dmoapi_t*)self;
	if (producer) {
		/* init base */
		tdav_producer_audio_init(TDAV_PRODUCER_AUDIO(producer));
		/* init self */
		InitializeCriticalSection(&producer->csCallback);
		InitializeCriticalSection(&producer->csBuffer);
		CoInitialize(NULL);

	}
	return self;
}
/* destructor */
static tsk_object_t* tdav_producer_dmoapi_dtor(tsk_object_t * self)
{
	tdav_producer_dmoapi_t *producer = (tdav_producer_dmoapi_t*)self;
	if (producer) {
		/* stop */
		if (producer->started) {
			tdav_producer_dmoapi_stop((tmedia_producer_t*)self);
		}

		/* deinit base */
		tdav_producer_audio_deinit(TDAV_PRODUCER_AUDIO(producer));

		/* deinit self */
		if (producer->events[0]) {
			CloseHandle(producer->events[0]);
		}
		if (producer->events[1]) {
			CloseHandle(producer->events[1]);
		}
		if (producer->events[2]) {
			CloseHandle(producer->events[2]);
		}

		DMO_SAFE_ARRAYDELETE(producer->pbOutputBuffer);

		DMO_SAFE_RELEASE(producer->pDMO);
		DMO_SAFE_RELEASE(producer->pPS);

		if (producer->pResampler) {
			speex_resampler_destroy(producer->pResampler);
			producer->pResampler = NULL;
		}
		if (producer->pResampleBuf) {
			TSK_SAFE_FREE(producer->pResampleBuf);
			producer->pResampleBuf = NULL;
		}
		DeleteCriticalSection(&producer->csBuffer);
		DeleteCriticalSection(&producer->csCallback);

		CoUninitialize();
	}

	return self;
}

/* object definition */
static const tsk_object_def_t tdav_producer_dmoapi_def_s =
{
	sizeof(tdav_producer_dmoapi_t),
	tdav_producer_dmoapi_ctor,
	tdav_producer_dmoapi_dtor,
	tdav_producer_audio_cmp,
};
/* plugin definition*/
static const tmedia_producer_plugin_def_t tdav_producer_dmoapi_plugin_def_s =
{
	&tdav_producer_dmoapi_def_s,

	tmedia_audio,
	"Microsoft DMOAPI producer",

	tdav_producer_dmoapi_set,
	tdav_producer_dmoapi_get,
	tdav_producer_dmoapi_prepare,
	tdav_producer_dmoapi_start,
	tdav_producer_dmoapi_pause,
	tdav_producer_dmoapi_stop
};
const tmedia_producer_plugin_def_t *tdav_producer_dmoapi_plugin_def_t = &tdav_producer_dmoapi_plugin_def_s;