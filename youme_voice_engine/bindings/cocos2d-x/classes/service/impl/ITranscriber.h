#ifndef ITRANSCRIBER_H
#define ITRANSCRIBER_H

#include <string>
#include <stdint.h>

class ITranscriberCallback
{
public:
	virtual void onTranscribeStarted(int statusCode, std::string taskId) = 0;
	virtual void onTranscribeCompleted(int statusCode, std::string taskId) = 0;
	virtual void onTranscribeFailed(int statusCode, std::string taskId, std::string errMsg) = 0;

	virtual void onSentenceBegin(int statusCode, std::string taskId, int sentenceIndex, int beforeSentenceTime) = 0;
	virtual void onSentenceEnd(int statusCode, std::string taskId, int sentenceIndex, std::string result, int sentenceTime ) = 0;

	virtual void onSentenceChanged(int statusCode, std::string taskId, int sentenceIndex, std::string result, int sentenceTime) = 0;
};


class ITranscriber
{
public:
	virtual void setToken(std::string token) = 0;
	virtual void setCallback(ITranscriberCallback* pCallback) = 0;
	virtual bool start() = 0;
	virtual bool stop() = 0;
	virtual bool sendAudioData(uint8_t* data, int data_size) = 0;
};

#endif
