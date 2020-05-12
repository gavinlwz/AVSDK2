#include "CustomInputManager.h"
#include "YouMeVoiceEngine.h"
#define MAX_CUSTOMDATA_LEN 1024


CCustomInputManager *CCustomInputManager::sInstance = nullptr;



class CustomOutputRunable :public youmecommon::IMessageRunable
{
public:
	CustomOutputRunable(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t m_uSessionId)
	{
		m_buffer.Allocate(iDataLen);
		memcpy(m_buffer.Get(), pData, iDataLen);
		m_ulTimeSpan = uTimeSpan;
		m_uSendSessionId = m_uSessionId;
	}

	virtual void Run() override
	{
		CYouMeVoiceEngine::getInstance()->NotifyCustomData(m_buffer.Get(), m_buffer.GetBufferLen(), m_ulTimeSpan, m_uSendSessionId);
	}
	youmecommon::CXSharedArray<byte> m_buffer;
	XINT64 m_ulTimeSpan = 0;
	uint32_t m_uSendSessionId = 0;
};

class CustomInputRunable :public youmecommon::IMessageRunable
{
public:
	CustomInputRunable(CCustomInputManager* pThis,const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t m_uSessionId)
	{
		m_buffer.Allocate(iDataLen);
		memcpy(m_buffer.Get(), pData, iDataLen);
		m_pThis = pThis;
		m_ulTimeSpan = uTimeSpan;
		m_uRecvSessionId = m_uSessionId;
	}

	virtual void Run() override
	{
		if (m_pThis->m_pInputCallback != nullptr)
		{
			m_pThis->m_pInputCallback(m_buffer.Get(), m_buffer.GetBufferLen(), m_ulTimeSpan,m_pThis->m_pParam, m_uRecvSessionId);
		}
	}

	CCustomInputManager* m_pThis;
	youmecommon::CXSharedArray<byte> m_buffer;
	XINT64 m_ulTimeSpan = 0;
	uint32_t m_uRecvSessionId = 0;
};


CCustomInputManager::CCustomInputManager()
{
}


CCustomInputManager::~CCustomInputManager()
{
	UnInit();
}

CCustomInputManager* CCustomInputManager::getInstance()
{
	if (nullptr == sInstance)
	{
		sInstance = new CCustomInputManager;
	}
	return sInstance;
}

bool CCustomInputManager::Init()
{
	if (m_inputMsgHandle != nullptr)
	{
		return true;
	}
	m_inputMsgHandle = new youmecommon::CMessageHandle;
	m_inputMsgHandle->Init();
	return true;
}

void CCustomInputManager::setInputCallback(OnInputDataNotify pDataCallback, void* pParam)
{
	m_pInputCallback = pDataCallback;
	m_pParam = pParam;
}

void CCustomInputManager::UnInit()
{
	delete m_inputMsgHandle;
	m_inputMsgHandle = nullptr;
}

int CCustomInputManager::PostInputData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSessionId)
{
	if (nullptr == m_inputMsgHandle)
	{
		return YOUME_ERROR_WRONG_STATE;
	}
	if ((nullptr == pData) || (iDataLen > MAX_CUSTOMDATA_LEN))
	{
		return YOUME_ERROR_INVALID_PARAM;
	}
	m_inputMsgHandle->Post(std::shared_ptr<youmecommon::IMessageRunable>(new CustomInputRunable(this,pData,iDataLen,uTimeSpan, uSessionId)));
	return 0;
}

void CCustomInputManager::NotifyCustomData(const void*pData, int iDataLen, XINT64 uTimeSpan, uint32_t uSessionId)
{
	if (nullptr == m_inputMsgHandle)
	{
		return ;
	}
	m_inputMsgHandle->Post(std::shared_ptr<youmecommon::IMessageRunable>(new CustomOutputRunable( pData, iDataLen, uTimeSpan, uSessionId)));
}
