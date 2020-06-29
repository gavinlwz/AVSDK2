#ifndef AUDIO_DEVICE_MGR_WIN_H
#define AUDIO_DEVICE_MGR_WIN_H

#ifndef MAX_DEVICE_ID_LENGTH
#define MAX_DEVICE_ID_LENGTH 512
#endif


class AudioDeviceMgr_Win
{
public :
	static  AudioDeviceMgr_Win* getInstance();

	int getAudioInputDeviceCount();
	bool getAudioInputDevice(int index, char deviceName[MAX_DEVICE_ID_LENGTH], char deviceId[MAX_DEVICE_ID_LENGTH]);

	int getDeviceIDByUID(const char* deviceUid);
};

#endif