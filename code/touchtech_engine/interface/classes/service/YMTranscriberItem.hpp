//
//  YMTranscriberItem.hpp
//  youme_voice_engine
//
//  Created by pinky on 2019/8/2.
//  Copyright © 2019 Youme. All rights reserved.
//

#ifndef YMTranscriberItem_hpp
#define YMTranscriberItem_hpp

#include <stdio.h>
#include <string>
#include <stdint.h>
#include <mutex>
#include "ITranscriber.h"

class CTranscriber : public ITranscriber
{
public:
    virtual void setToken( std::string token ) override;
    virtual bool start() override;
    virtual bool stop() override ;
    virtual bool sendAudioData( uint8_t* data, int data_size ) override ;
	virtual void setCallback(ITranscriberCallback* pCallback) override ;
};

class YMTranscriberItem : public ITranscriberCallback
{
public:
    YMTranscriberItem( int sessionId );
    ~YMTranscriberItem();
    
public:
    void setToken( std::string token);
    bool isStarted();
    void start();
    void inputAudioData( uint8_t* data, int data_size );
    void sendAudioData();
    void stop();
    
public:
    //callback
    void onTranscribeStarted( int statusCode, std::string taskId ) override;
    void onTranscribeCompleted( int statusCode, std::string taskId ) override;
    void onTranscribeFailed( int statusCode, std::string taskId, std::string errMsg )override;
    
    void onSentenceBegin( int statusCode, std::string taskId, int sentenceIndex, int beforeSentenceTime )override;

	void onSentenceEnd(int statusCode, std::string taskId, int sentenceIndex, std::string result, int sentenceTime) override;
	void onSentenceChanged(int statusCode, std::string taskId, int sentenceIndex, std::string result, int sentenceTime) override;


private:
    int m_sessionId;
	std::string m_userid;
    ITranscriber* m_pTranscriber;
    bool m_isStarted;
    uint8_t* m_buffer;
    int m_bufferSize;
    int m_dataSize;

	std::mutex m_dataMutex;
    
};
#endif /* YMTranscriber_hpp */
