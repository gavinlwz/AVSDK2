//
//  YouMeEngineVideoCodec.hpp
//  youme_voice_engine
//
//  Created by bhb on 2017/11/17.
//  Copyright © 2017年 Youme. All rights reserved.
//

#ifndef YouMeEngineVideoCodec_hpp
#define YouMeEngineVideoCodec_hpp

#include <thread>
#include <deque>
#include <mutex>
#include <condition_variable>
#include "YouMeConstDefine.h"
#include "IYouMeVideoCallback.h"
#include "ICameraManager.hpp"

class  YouMeEngineVideoCodec{
public:
    YouMeEngineVideoCodec();
    ~YouMeEngineVideoCodec();
    
    static YouMeEngineVideoCodec * getInstance();
    static void unInstance();
    
    void registerCodecCallback(CameraPreviewCallback *cb);
    void unregisterCodecCallback();
    void pushFrame(FrameImage* frame, bool isQiniu = false);
    
    void pushFrameNew(FrameImage* frame);
private:
    void startThread();
    void stopThread();
    void threadFunc();
	void threadFuncNew();
    void ClearMessageQueue();
	void ClearMessageQueueNew();
private:
    std::thread m_thread;
	std::thread m_threadNew;
    bool m_isLooping;
    
    std::deque<FrameImage*> m_msgQueue;
    std::mutex m_msgQueueMutex;
    std::condition_variable m_msgQueueCond;

	std::deque<FrameImage*> m_msgQueueNew;
	std::mutex m_msgQueueMutexNew;
	std::condition_variable m_msgQueueCondNew;
    
    CameraPreviewCallback * m_cameraPreviewCallback;
    
    bool m_isQiniuFrame;
    
    uint64_t m_lastTime;
    int  m_frameCount;
};

#endif /* YouMeEngineVideoCodec_hpp */
