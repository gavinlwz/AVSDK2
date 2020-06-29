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
* @brief 调用start(), 成功与云端建立连接, sdk内部线程上报started事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 服务端检测到了一句话的开始, sdk内部线程上报SentenceBegin事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 服务端检测到了一句话结束, sdk内部线程上报SentenceEnd事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 识别结果发生了变化, sdk在接收到云端返回到最新结果时, sdk内部线程上报ResultChanged事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 服务端停止实时音频流识别时, sdk内部线程上报Completed事件
* @note 上报Completed事件之后，SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送.
*       不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 识别过程(包含start(), send(), stop())发生异常时, sdk内部线程上报TaskFailed事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @note 上报TaskFailed事件之后, SDK内部会关闭识别连接通道. 此时调用sendAudio会返回-1, 请停止发送
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
* @brief 识别结束或发生异常时，会关闭连接通道, sdk内部线程上报ChannelCloseed事件
* @note 不允许在回调函数内部调用stop(), releaseTranscriberRequest()对象操作, 否则会异常
* @param cbEvent 回调事件结构, 详见nlsEvent.h
* @param cbParam 回调自定义参数，默认为NULL, 可以根据需求自定义参数
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
	callback->setOnTranscriptionStarted(onTranscriptionStarted, this ); // 设置识别启动回调函数
	callback->setOnTranscriptionResultChanged(onTranscriptionResultChanged, this); // 设置识别结果变化回调函数
	callback->setOnTranscriptionCompleted(onTranscriptionCompleted, this); // 设置语音转写结束回调函数
	callback->setOnSentenceBegin(onSentenceBegin, this); // 设置一句话开始回调函数
	callback->setOnSentenceEnd(onSentenceEnd, this); // 设置一句话结束回调函数
	callback->setOnTaskFailed(onTaskFailed, this); // 设置异常识别回调函数
	callback->setOnChannelClosed(onChannelClosed, this); // 设置识别通道关闭回调函数

	m_request = NlsClient::getInstance()->createTranscriberRequest( callback);
	if (m_request == nullptr)
	{
		TSK_DEBUG_ERROR("@@ start Transcriber create request failed!");
		return false;
	}

	m_request->setAppKey(AppKey);
	m_request->setFormat("pcm"); // 设置音频数据编码格式, 默认是pcm
	m_request->setSampleRate(SAMPLE_RATE); // 设置音频数据采样率, 可选参数，目前支持16000, 8000. 默认是16000
	m_request->setIntermediateResult(true); // 设置是否返回中间识别结果, 可选参数. 默认false
	m_request->setPunctuationPrediction(true); // 设置是否在后处理中添加标点, 可选参数. 默认false
	m_request->setInverseTextNormalization(true); // 设置是否在后处理中执行数字转写, 可选参数. 默认false

	//语音断句检测阈值，一句话之后静音长度超过该值，即本句结束，合法参数范围200～2000(ms)，默认值800ms
	//m_request->setMaxSentenceSilence(800);
	//m_request->setCustomizationId("TestId_123"); //定制模型id, 可选.
	//m_request->setVocabularyId("TestId_456"); //定制泛热词id, 可选.

	m_request->setToken( m_token.c_str());

	if (m_request->start() < 0)
	{
		TSK_DEBUG_ERROR("@@ start Transcriber start request failed!");
		NlsClient::getInstance()->releaseTranscriberRequest(m_request); // start()失败，释放request对象
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
	* 4: 发送音频数据. sendAudio返回-1表示发送失败, 需要停止发送. 对于第三个参数:
	* format为opu(发送原始音频数据必须为PCM, FRAME_SIZE大小必须为640)时, 需设置为true. 其它格式默认使用false.
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