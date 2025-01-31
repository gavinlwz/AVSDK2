﻿#define YOUMEVIDEO_EXPORTS
#include "tsk.h"
#include "IYouMeVideoRenderEngine.h"
#include "VideoRenderMgr.h"
#include "IYouMeVoiceEngine.h"

static IYouMeVideoRenderEngine* mPVideoRenderEngine = NULL;
IYouMeVideoRenderEngine::IYouMeVideoRenderEngine()
{
}


IYouMeVideoRenderEngine::~IYouMeVideoRenderEngine()
{
}

IYouMeVideoRenderEngine* IYouMeVideoRenderEngine::getInstance(){
	if (!mPVideoRenderEngine)
		mPVideoRenderEngine = new IYouMeVideoRenderEngine();

	return  mPVideoRenderEngine;
}

bool IYouMeVideoRenderEngine::initRenderEngine(){
	IYouMeVoiceEngine::getInstance()->setVideoCallback(VideoRenderMgr::getInstance());
	return VideoRenderMgr::getInstance()->initRender();
}

void IYouMeVideoRenderEngine::destroy(){

	IYouMeVoiceEngine::getInstance()->setVideoCallback(NULL);
	VideoRenderMgr::destroy();
	if (mPVideoRenderEngine){
		delete mPVideoRenderEngine;
		mPVideoRenderEngine = NULL;
	}
	TSK_DEBUG_INFO("@@ IYouMeVideoRenderEngine destroy");
}



bool IYouMeVideoRenderEngine::createLocalRender(HWND  hWnd){
	return VideoRenderMgr::getInstance()->createLocalRender(hWnd);
}

void IYouMeVideoRenderEngine::deleteLocalRender(){
	VideoRenderMgr::getInstance()->deleteLocalRender();
}

bool IYouMeVideoRenderEngine::createRender(const char* userId, HWND  hWnd){
	std::string strUserID = userId;
	return VideoRenderMgr::getInstance()->createRender(strUserID, hWnd);
}

void IYouMeVideoRenderEngine::deleteRender(const char* userId){
	std::string strUserID = userId;
	VideoRenderMgr::getInstance()->deleteRender(strUserID);
}

void IYouMeVideoRenderEngine::deleteAllRender(){

	VideoRenderMgr::getInstance()->deleteAllRender();

}

int IYouMeVideoRenderEngine::setRenderRotation(const char* userId, int rotation) {
	if (!userId || (rotation != 0 && rotation != 90 && rotation != 180 && rotation != 270)) {

		TSK_DEBUG_INFO("@@ setRenderRotation invalid user:%s, rotation:%d", userId, rotation);
		return -1;
	}

	std::string strUserID = userId;
	VideoRenderMgr::getInstance()->setRenderRotation(strUserID, rotation);

	return 0;
}

