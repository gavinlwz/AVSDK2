#include "AliTranscriberWin.h"
#ifdef TRANSSCRIBER

#include "nlsClient.h"
#include "nlsEvent.h"


#include "tsk_debug.h"
#include <YouMeCommon/CrossPlatformDefine/PlatformDef.h>

#define AppKey "XdUdb8hYYSIsbePp"
#define SAMPLE_RATE 16000

using AlibabaNls::NlsClient;
using AlibabaNls::NlsEvent;
using AlibabaNls::LogDebug;
using AlibabaNls::LogInfo;
using AlibabaNls::SpeechTranscriberCallback;


/**
* @brief ����start(), �ɹ����ƶ˽�������, sdk�ڲ��߳��ϱ�started�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onTranscriptionStarted(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		pTranscriber->m_pCallback->onTranscribeStarted(cbEvent->getStausCode(), cbEvent->getTaskId());
	}
}

/**
* @brief ����˼�⵽��һ�仰�Ŀ�ʼ, sdk�ڲ��߳��ϱ�SentenceBegin�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onSentenceBegin(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		pTranscriber->m_pCallback->onSentenceBegin(cbEvent->getStausCode(), cbEvent->getTaskId(),
			cbEvent->getSentenceIndex(), cbEvent->getSentenceTime() );
	}
}

/**
* @brief ����˼�⵽��һ�仰����, sdk�ڲ��߳��ϱ�SentenceEnd�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onSentenceEnd(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		std::string str = cbEvent->getResult();
		
		XString strUnicode = LocalToXString( str );
		std::string strUtf8 = XStringToUTF8(strUnicode);

		pTranscriber->m_pCallback->onSentenceEnd(cbEvent->getStausCode(), cbEvent->getTaskId(),
			cbEvent->getSentenceIndex(), strUtf8, cbEvent->getSentenceTime());
	}
}

/**
* @brief ʶ���������˱仯, sdk�ڽ��յ��ƶ˷��ص����½��ʱ, sdk�ڲ��߳��ϱ�ResultChanged�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onTranscriptionResultChanged(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		std::string str = cbEvent->getResult();
		XString strUnicode = LocalToXString(str);
		std::string strUtf8 = XStringToUTF8(strUnicode);

		pTranscriber->m_pCallback->onSentenceChanged(cbEvent->getStausCode(), cbEvent->getTaskId(),
			cbEvent->getSentenceIndex(), strUtf8, cbEvent->getSentenceTime());
	}
}

/**
* @brief �����ֹͣʵʱ��Ƶ��ʶ��ʱ, sdk�ڲ��߳��ϱ�Completed�¼�
* @note �ϱ�Completed�¼�֮��SDK�ڲ���ر�ʶ������ͨ��. ��ʱ����sendAudio�᷵��-1, ��ֹͣ����.
*       �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onTranscriptionCompleted(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		pTranscriber->m_pCallback->onTranscribeCompleted(cbEvent->getStausCode(), cbEvent->getTaskId());
	}
}

/**
* @brief ʶ�����(����start(), send(), stop())�����쳣ʱ, sdk�ڲ��߳��ϱ�TaskFailed�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @note �ϱ�TaskFailed�¼�֮��, SDK�ڲ���ر�ʶ������ͨ��. ��ʱ����sendAudio�᷵��-1, ��ֹͣ����
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onTaskFailed(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;

	if (pTranscriber)
	{
		pTranscriber->m_bFailed = true;
	}

	if (pTranscriber && pTranscriber->m_pCallback)
	{
		pTranscriber->m_pCallback->onTranscribeFailed(cbEvent->getStausCode(), cbEvent->getTaskId(), 
			cbEvent->getErrorMessage());
	}
}

/**
* @brief ʶ����������쳣ʱ����ر�����ͨ��, sdk�ڲ��߳��ϱ�ChannelCloseed�¼�
* @note �������ڻص������ڲ�����stop(), releaseTranscriberRequest()�������, ������쳣
* @param cbEvent �ص��¼��ṹ, ���nlsEvent.h
* @param cbParam �ص��Զ��������Ĭ��ΪNULL, ���Ը��������Զ������
* @return
*/
void onChannelClosed(NlsEvent* cbEvent, void* cbParam) {
	AliTranscriberWin* pTranscriber = (AliTranscriberWin*)cbParam;
}



AliTranscriberWin::AliTranscriberWin()
	:m_token("")
	,m_pCallback( nullptr )
	, m_request( nullptr )
	, m_bTranscribing(false)
{
	NlsClient::getInstance()->setLogConfig("log-transcriber.txt", LogInfo);
}

void AliTranscriberWin::setToken(std::string token)
{
	m_token = token;
}

void AliTranscriberWin::setCallback(ITranscriberCallback* pCallback)
{
	m_pCallback = pCallback;
}

bool AliTranscriberWin::start()
{
	SpeechTranscriberCallback* callback = new SpeechTranscriberCallback();
	callback->setOnTranscriptionStarted(onTranscriptionStarted, this ); // ����ʶ�������ص�����
	callback->setOnTranscriptionResultChanged(onTranscriptionResultChanged, this); // ����ʶ�����仯�ص�����
	callback->setOnTranscriptionCompleted(onTranscriptionCompleted, this); // ��������תд�����ص�����
	callback->setOnSentenceBegin(onSentenceBegin, this); // ����һ�仰��ʼ�ص�����
	callback->setOnSentenceEnd(onSentenceEnd, this); // ����һ�仰�����ص�����
	callback->setOnTaskFailed(onTaskFailed, this); // �����쳣ʶ��ص�����
	callback->setOnChannelClosed(onChannelClosed, this); // ����ʶ��ͨ���رջص�����

	m_request = NlsClient::getInstance()->createTranscriberRequest( callback);
	if (m_request == nullptr)
	{
		TSK_DEBUG_ERROR("@@ start Transcriber create request failed!");
		return false;
	}

	m_request->setAppKey(AppKey);
	m_request->setFormat("pcm"); // ������Ƶ���ݱ����ʽ, Ĭ����pcm
	m_request->setSampleRate(SAMPLE_RATE); // ������Ƶ���ݲ�����, ��ѡ������Ŀǰ֧��16000, 8000. Ĭ����16000
	m_request->setIntermediateResult(true); // �����Ƿ񷵻��м�ʶ����, ��ѡ����. Ĭ��false
	m_request->setPunctuationPrediction(true); // �����Ƿ��ں�������ӱ��, ��ѡ����. Ĭ��false
	m_request->setInverseTextNormalization(true); // �����Ƿ��ں�����ִ������תд, ��ѡ����. Ĭ��false

	//�����Ͼ�����ֵ��һ�仰֮�������ȳ�����ֵ��������������Ϸ�������Χ200��2000(ms)��Ĭ��ֵ800ms
	//m_request->setMaxSentenceSilence(800);
	//m_request->setCustomizationId("TestId_123"); //����ģ��id, ��ѡ.
	//m_request->setVocabularyId("TestId_456"); //���Ʒ��ȴ�id, ��ѡ.

	m_request->setToken( m_token.c_str());

	if (m_request->start() < 0)
	{
		TSK_DEBUG_ERROR("@@ start Transcriber start request failed!");
		NlsClient::getInstance()->releaseTranscriberRequest(m_request); // start()ʧ�ܣ��ͷ�request����
		m_request = nullptr;
		return false;
	}

	m_bTranscribing = true;
	m_bFailed = false;

	TSK_DEBUG_ERROR("@@ start Transcriber start request !");
	return true;
	
}

bool AliTranscriberWin::stop()
{
	m_bTranscribing = false;
	if (m_request)
	{
		m_request->stop();
		NlsClient::getInstance()->releaseTranscriberRequest(m_request);
	}

	return true;
}

bool AliTranscriberWin::sendAudioData(uint8_t* data, int data_size)
{
	if (!m_bTranscribing || m_request == nullptr)
	{
		return false;
	}

	if ( m_bFailed )
	{
		return false;
	}

	/*
	* 4: ������Ƶ����. sendAudio����-1��ʾ����ʧ��, ��Ҫֹͣ����. ���ڵ���������:
	* formatΪopu(����ԭʼ��Ƶ���ݱ���ΪPCM, FRAME_SIZE��С����Ϊ640)ʱ, ������Ϊtrue. ������ʽĬ��ʹ��false.
	*/
	int nLen = m_request->sendAudio( (const char*)data, data_size, false);
	if (nLen < 0)
	{
		TSK_DEBUG_ERROR("Transcriber sendAudio Failed.");
		m_bFailed = true;
		return false;
	}
	else {
		//static FILE* fileRecord = fopen("E:\\workspace\\record_win.pcm", "wb");
		//int n = fwrite(data, 1, nLen, fileRecord);
		//fflush(fileRecord);
	}

	//TSK_DEBUG_ERROR("Pinky________________Transcriber sendAudio :%d, %d.", data_size, nLen );

	return true;
}
#endif //TRANSSCRIBER