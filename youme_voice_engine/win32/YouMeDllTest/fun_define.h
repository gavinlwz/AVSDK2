#ifndef _FUN_DEFINE_
#define _FUN_DEFINE_

#include <windows.h>

#define BAD_POINT(FUN) \
    if (!FUN)          \
    {                  \
        goto __EXIT;   \
    }                  \

typedef int(*fun_youme_init)(const char* strAPPKey, const char* strAPPSecret);
typedef int(*fun_youme_unInit)();
typedef int(*fun_youme_setcallback)(const char* strObjName);
typedef int(*fun_youme_setSpeakerMute)(bool bOn);
typedef int(*fun_youme_getSpeakerMute)();
typedef int(*fun_youme_getMicrophoneMute)();
typedef int(*fun_youme_setMicrophoneMute)(bool mute);
typedef int(*fun_youme_getVolume)();
typedef int(*fun_youme_setVolume)(unsigned int uiVolume);
typedef int(*fun_youme_getUseMobileNetworkEnabled)();
typedef int(*fun_youme_setUseMobileNetworkEnabled)(bool bEnabled);
typedef int(*fun_youme_getState)();
typedef int(*fun_youme_joinConference)(const char* pUserID, const char* pRoomID);
typedef int(*fun_youme_joinConference_2)(const char* pUserID, const char* pRoomID, bool needUserList,
    bool needMic, bool needSpeaker, bool autoSendStatus);
typedef int(*fun_youme_joinConferenceMulti)(const char* pUserID, const char* pRoomID, int eUserRole);
typedef int(*fun_youme_speakToConference)(const char* pRoomID);
typedef int(*fun_youme_leaveConference)(const char* strRoomID);
typedef int(*fun_youme_setSoundtouchEnabled)(bool bEnable);
typedef int(*fun_youme_getSoundtouchEnabled)();
typedef int(*fun_youme_setSoundtouchTempo)(float nTempo);
typedef int(*fun_youme_setSoundtouchRate)(float nRate);
typedef int(*fun_youme_setSoundtouchPitch)(float nPitch);
typedef int(*fun_youme_setOtherMicStatus)(const char*  strUserID, bool isOn);
typedef int(*fun_youme_setOtherSpeakerStatus)(const char*  strUserID, bool isOn);
typedef int(*fun_youme_avoidOtherVoice)(const char*  strUserID, bool isOn);
typedef int(*fun_youme_mixAudioTrack)(const void* pBuf, int nSizeInByte, int nChannelNUm, int nSampleRateHz,
    int nBytesPerSample, bool bFloat, bool bLittleEndian, bool bInterleaved, bool bForSpeaker);
typedef int(*fun_youme_playBackgroundMusic)(const char* pFilePath, bool bRepeat, int mode);
typedef int(*fun_youme_stopBackgroundMusic)(int mode);
typedef int(*fun_youme_setBackgroundMusicVolume)(int mode, int vol);
typedef int(*fun_youme_setMicBypassToSpeaker)(bool enabled);

extern fun_youme_init                         youme_init = NULL;
extern fun_youme_unInit                       youme_unInit = NULL;
extern fun_youme_setcallback                  youme_setcallback = NULL;
extern fun_youme_setSpeakerMute               youme_setSpeakerMute = NULL;
extern fun_youme_getSpeakerMute               youme_getSpeakerMute = NULL;
extern fun_youme_getMicrophoneMute            youme_getMicrophoneMute = NULL;
extern fun_youme_setMicrophoneMute            youme_setMicrophoneMute = NULL;
extern fun_youme_getVolume                    youme_getVolume = NULL;
extern fun_youme_setVolume                    youme_setVolume = NULL;
extern fun_youme_getUseMobileNetworkEnabled   youme_getUseMobileNetworkEnabled = NULL;
extern fun_youme_setUseMobileNetworkEnabled   youme_setUseMobileNetworkEnabled = NULL;
extern fun_youme_getState                     youme_getState = NULL;
extern fun_youme_joinConference               youme_joinConference = NULL;
extern fun_youme_joinConference_2             youme_joinConference_2 = NULL;
extern fun_youme_joinConferenceMulti          youme_joinConferenceMulti = NULL; 
extern fun_youme_speakToConference            youme_speakToConference = NULL; 
extern fun_youme_leaveConference              youme_leaveConference = NULL;
extern fun_youme_setSoundtouchEnabled         youme_setSoundtouchEnabled = NULL;
extern fun_youme_getSoundtouchEnabled         youme_getSoundtouchEnabled = NULL;
extern fun_youme_setSoundtouchTempo           youme_setSoundtouchTempo = NULL;
extern fun_youme_setSoundtouchRate            youme_setSoundtouchRate = NULL;
extern fun_youme_setSoundtouchPitch           youme_setSoundtouchPitch = NULL;
extern fun_youme_setOtherMicStatus            youme_setOtherMicStatus = NULL;
extern fun_youme_setOtherSpeakerStatus        youme_setOtherSpeakerStatus = NULL;
extern fun_youme_avoidOtherVoice              youme_avoidOtherVoice = NULL;
extern fun_youme_mixAudioTrack                youme_mixAudioTrack = NULL;
extern fun_youme_playBackgroundMusic          youme_playBackgroundMusic = NULL;
extern fun_youme_stopBackgroundMusic          youme_stopBackgroundMusic = NULL;
extern fun_youme_setBackgroundMusicVolume     youme_setBackgroundMusicVolume = NULL;
extern fun_youme_setMicBypassToSpeaker        youme_setMicBypassToSpeaker = NULL;

HMODULE hDll = NULL;

BOOL InitFunction()
{
    BOOL bRet = FALSE;
    hDll = LoadLibrary("youme_voice_engine.dll"); BAD_POINT(hDll);
    //(const char* strAPPKey, const char* strAPPSecret);
    youme_init                       = (fun_youme_init                       )GetProcAddress(hDll, "youme_init"); 				        BAD_POINT(youme_init);
    youme_setcallback                = (fun_youme_setcallback                )GetProcAddress(hDll, "youme_setcallback"); 				BAD_POINT(youme_setcallback);
    youme_unInit                     = (fun_youme_unInit                     )GetProcAddress(hDll, "youme_unInit"); 					BAD_POINT(youme_unInit);
    youme_setSpeakerMute             = (fun_youme_setSpeakerMute             )GetProcAddress(hDll, "youme_setSpeakerMute"); 			BAD_POINT(youme_setSpeakerMute);
    youme_getSpeakerMute             = (fun_youme_getSpeakerMute             )GetProcAddress(hDll, "youme_getSpeakerMute"); 			BAD_POINT(youme_getSpeakerMute);
    youme_getMicrophoneMute          = (fun_youme_getMicrophoneMute          )GetProcAddress(hDll, "youme_getMicrophoneMute"); 			BAD_POINT(youme_getMicrophoneMute);
    youme_setMicrophoneMute          = (fun_youme_setMicrophoneMute          )GetProcAddress(hDll, "youme_setMicrophoneMute");			BAD_POINT(youme_setMicrophoneMute);
    youme_getVolume                  = (fun_youme_getVolume                  )GetProcAddress(hDll, "youme_getVolume"); 					BAD_POINT(youme_getVolume);
    youme_setVolume                  = (fun_youme_setVolume                  )GetProcAddress(hDll, "youme_setVolume"); 					BAD_POINT(youme_setVolume);
    youme_getUseMobileNetworkEnabled = (fun_youme_getUseMobileNetworkEnabled )GetProcAddress(hDll, "youme_getUseMobileNetworkEnabled"); BAD_POINT(youme_getUseMobileNetworkEnabled);
    youme_setUseMobileNetworkEnabled = (fun_youme_setUseMobileNetworkEnabled )GetProcAddress(hDll, "youme_setUseMobileNetworkEnabled"); BAD_POINT(youme_setUseMobileNetworkEnabled);
    youme_getState                   = (fun_youme_getState                   )GetProcAddress(hDll, "youme_getState"); 					BAD_POINT(youme_getState);
    youme_joinConference             = (fun_youme_joinConference             )GetProcAddress(hDll, "youme_joinConference"); 			BAD_POINT(youme_joinConference);
    youme_joinConference_2           = (fun_youme_joinConference_2           )GetProcAddress(hDll, "youme_joinConference_2"); 			BAD_POINT(youme_joinConference_2);
	youme_joinConferenceMulti        = (fun_youme_joinConferenceMulti        )GetProcAddress(hDll, "youme_joinConferenceMulti");        BAD_POINT(youme_joinConferenceMulti);
	youme_speakToConference          = (fun_youme_speakToConference          )GetProcAddress(hDll, "youme_speakToConference");          BAD_POINT(youme_speakToConference);
    youme_leaveConference            = (fun_youme_leaveConference            )GetProcAddress(hDll, "youme_leaveConference"); 			BAD_POINT(youme_leaveConference);
    youme_setSoundtouchEnabled       = (fun_youme_setSoundtouchEnabled       )GetProcAddress(hDll, "youme_setSoundtouchEnabled"); 		BAD_POINT(youme_setSoundtouchEnabled);
    youme_getSoundtouchEnabled       = (fun_youme_getSoundtouchEnabled       )GetProcAddress(hDll, "youme_getSoundtouchEnabled"); 		BAD_POINT(youme_getSoundtouchEnabled);
    youme_setSoundtouchTempo         = (fun_youme_setSoundtouchTempo         )GetProcAddress(hDll, "youme_setSoundtouchTempo"); 		BAD_POINT(youme_setSoundtouchTempo);
    youme_setSoundtouchRate          = (fun_youme_setSoundtouchRate          )GetProcAddress(hDll, "youme_setSoundtouchRate"); 			BAD_POINT(youme_setSoundtouchRate);
    youme_setSoundtouchPitch         = (fun_youme_setSoundtouchPitch         )GetProcAddress(hDll, "youme_setSoundtouchPitch"); 		BAD_POINT(youme_setSoundtouchPitch);
    youme_setOtherMicStatus          = (fun_youme_setOtherMicStatus          )GetProcAddress(hDll, "youme_setOtherMicStatus"); 			BAD_POINT(youme_setOtherMicStatus);
    youme_setOtherSpeakerStatus      = (fun_youme_setOtherSpeakerStatus      )GetProcAddress(hDll, "youme_setOtherSpeakerStatus"); 		BAD_POINT(youme_setOtherSpeakerStatus);
    youme_avoidOtherVoice            = (fun_youme_avoidOtherVoice            )GetProcAddress(hDll, "youme_avoidOtherVoiceStatus"); 		BAD_POINT(youme_avoidOtherVoice);
    youme_mixAudioTrack              = (fun_youme_mixAudioTrack              )GetProcAddress(hDll, "youme_mixAudioTrack");       		BAD_POINT(youme_mixAudioTrack);
    youme_playBackgroundMusic        = (fun_youme_playBackgroundMusic        )GetProcAddress(hDll, "youme_playBackgroundMusic"); 		BAD_POINT(youme_playBackgroundMusic);
    youme_stopBackgroundMusic        = (fun_youme_stopBackgroundMusic        )GetProcAddress(hDll, "youme_stopBackgroundMusic"); 		BAD_POINT(youme_stopBackgroundMusic);
    youme_setBackgroundMusicVolume   = (fun_youme_setBackgroundMusicVolume   )GetProcAddress(hDll, "youme_setBackgroundMusicVolume"); 	BAD_POINT(youme_setBackgroundMusicVolume);
    youme_setMicBypassToSpeaker      = (fun_youme_setMicBypassToSpeaker      )GetProcAddress(hDll, "youme_setMicBypassToSpeaker"); 		BAD_POINT(youme_setMicBypassToSpeaker);
    bRet = TRUE;

__EXIT:
    if (!bRet && hDll)
    {
        FreeLibrary(hDll);
        hDll = NULL;
    }
    return bRet;
}
#endif