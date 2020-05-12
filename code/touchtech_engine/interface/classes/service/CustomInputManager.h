#pragma once
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>
#include <YouMeCommon/XSharedArray.h>
#include <YouMeCommon/MessageHandle.h>

typedef enum CUSTOM_INPUT_TYPE{
    CUSTOM_INPUT_TYPE_SEND = 0,
    CUSTOM_INPUT_TYPE_RECV = 1,
}CUSTOM_INPUT_TYPE_t;

typedef void(*OnInputDataNotify)(const void*pData, int iDataLen, unsigned long long uTimeSpa, void* pParam, uint32_t uSessionId);

class CCustomInputManager
{
public:
	CCustomInputManager();
	~CCustomInputManager();

	static CCustomInputManager* getInstance();
	

	bool Init();
	void setInputCallback(OnInputDataNotify pDataCallback, void* pParam);

	void UnInit();
	//把用户的数据通过网络传给房间其他人, sessionId 为目的用户
	int PostInputData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSessionId);

	//接收其他用户的数据，回调上去, uSessionId 为发送用户sessionId
	void NotifyCustomData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSessionId);

	OnInputDataNotify m_pInputCallback = nullptr;
	void* m_pParam = nullptr;

	youmecommon::CMessageHandle* m_inputMsgHandle = nullptr;


	static CCustomInputManager *sInstance;
};

