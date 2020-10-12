#pragma once
#include <windows.h>
#include <string>
#include "../../bindings/cocos2d-x/interface/IYouMeVoiceEngine.h"

class CYouMeEventCallback : public IYouMeEventCallback
{
public:
	CYouMeEventCallback(HWND mHand = NULL);
	~CYouMeEventCallback();
private:
	HWND m_msgHand;
public:
	virtual void onEvent(const YouMeEvent event, const YouMeErrorCode error, const char * room, const char * param) override;
};