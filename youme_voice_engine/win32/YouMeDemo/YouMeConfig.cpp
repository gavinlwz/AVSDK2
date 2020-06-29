#include "stdafx.h"
#include "YouMeConfig.h"



std::string CYouMeConfig::strJoinAppKey = "YOUME5BE427937AF216E88E0F84C0EF148BD29B691556";

std::string CYouMeConfig::g_strKey = "YOUMEBC2B3171A7A165DC10918A7B50A4B939F2A187D0";
std::string CYouMeConfig::g_strSecret = "r1+ih9rvMEDD3jUoU+nj8C7VljQr7Tuk4TtcByIdyAqjdl5lhlESU0D+SoRZ30sopoaOBg9EsiIMdc8R16WpJPNwLYx2WDT5hI/HsLl1NJjQfa9ZPuz7c/xVb8GHJlMf/wtmuog3bHCpuninqsm3DRWiZZugBTEj2ryrhK7oZncBAAE=";

CYouMeConfig::CYouMeConfig()
{

}
CYouMeConfig::~CYouMeConfig()
{
}
/**
 *
 */
BOOL CYouMeConfig::EngineInstance(HWND mHand)
{
	if (!m_pEngine)
	{
		m_pEngine = IYouMeVoiceEngine::getInstance();
	}

	if (!m_pEventCallback) {
		m_pEventCallback = new CYouMeEventCallback(mHand);
	}

	return (m_pEngine && m_pEventCallback);
}