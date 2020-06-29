#include "stdafx.h"
#include "YouMeEventCallback.h"
#include <map>

CYouMeEventCallback::CYouMeEventCallback(HWND mHand)
{
	m_msgHand = mHand;
}


CYouMeEventCallback::~CYouMeEventCallback()
{
}


void CYouMeEventCallback::onEvent(const YouMeEvent_t eventType, const YouMeErrorCode iErrorCode, const char * room, const char * param)
{
	std::map<std::string, int> map1;
	std::map<std::string, std::string> map2;

	map1.insert(std::map<std::string, int>::value_type("event", eventType));
	map1.insert(std::map<std::string, int>::value_type("error", iErrorCode));
	map2.insert(std::map<std::string, std::string>::value_type("room", room));
	map2.insert(std::map<std::string, std::string>::value_type("param", param));

	SendMessage(m_msgHand, UM_CONTROL, (WPARAM)&map1, (LPARAM)&map2);
}