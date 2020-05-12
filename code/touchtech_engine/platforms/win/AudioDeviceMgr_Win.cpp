#include "AudioDeviceMgr_Win.h"

#include <windows.h>
#include <MMSystem.h>
#include <stdio.h>
#include <YouMeCommon/StringUtil.hpp>

AudioDeviceMgr_Win* AudioDeviceMgr_Win::getInstance()
{
	static AudioDeviceMgr_Win* s_ins = new AudioDeviceMgr_Win();
	return s_ins;
}

int AudioDeviceMgr_Win::getAudioInputDeviceCount()
{
	int count = waveInGetNumDevs();
	return count;
}
bool AudioDeviceMgr_Win::getAudioInputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH])
{
	int count = getAudioInputDeviceCount();
	if (index >= count)
	{
		return false;
	}

	WAVEINCAPS  waveIncaps = { 0 };
	MMRESULT mmResult = waveInGetDevCaps(index, &waveIncaps, sizeof(WAVEINCAPS));
	
	strcpy(deviceName, XStringToUTF8(Local_to_Unicode(waveIncaps.szPname, strlen(waveIncaps.szPname))).c_str());
	sprintf(deviceId, "%d_%s", index, XStringToUTF8(Local_to_Unicode(waveIncaps.szPname, strlen(waveIncaps.szPname))).c_str());

	return true;
}

int AudioDeviceMgr_Win::getDeviceIDByUID(const char* deviceUid)
{
	int count = getAudioInputDeviceCount();

	for (int i = 0; i < count; i++)
	{
		char deviceName[MAX_DEVICE_ID_LENGTH] = { 0 };
		char deviceId[MAX_DEVICE_ID_LENGTH] = { 0 };

		bool bRet = getAudioInputDevice(i, deviceName, deviceId);

		if (bRet == true && strcmp(deviceId, deviceUid) == 0)
		{
			return i;
		}
	}

	return -1;
}