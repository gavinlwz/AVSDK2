package com.youme.engine;

import com.youme.engine.audio.YMAudioFrameInfo;

import java.nio.ByteBuffer;

public class NativeEngine {
    //系统参数
    public native static void setModel(String model);

    public native static void setSysName(String sysname);

    public native static void setSysVersion(String sysversion);

    public native static void setVersionName(String versionName);

    public native static void setDeviceURN(String deviceURN);

    public native static void setDeviceIMEI(String deviceIMEI);

    public native static void setCPUArch(String ABI);

    public native static void setPackageName(String packageName);

    public native static void setUUID(String UUID);

    public native static void setDocumentPath(String path);

//    public native static void onNetWorkChanged(int type);

//    public native static void onHeadSetPlugin(int state);

    public native static String getSoVersion();

    public static native String getSdkInfo();

    public native static void setBrand(String brand);

    public native static void setCPUChip(String strCPUChip);


//    public native static void setScreenOrientation(int orientation);

//    public native static void setServerMode(int mode);

//    public native static void setServerIpPort(String ip, int port);

//    public native static void AudioRecorderBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);

//    public native static void AudioPlayerBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);

//    public native static int inputCustomData(byte[] data, int len, long timestamp);

//    public native static int inputCustomDataToUser(byte[] data, int len, long timestamp, String userId);

//    public native static void VideoCaptureBufRefresh(byte[] videoBuf, int bufSize, int fps, int rotationDegree, boolean mirror, int width, int height, int videoFmt);

    public native static void callbackPermissionStatus(int errCode);

//    public native static void setPcmCallbackEnable(int flag, boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel);

//    public native static void setVideoPreDecodeCallbackEnable(boolean enable, boolean needDecodeandRender);

//    public native static void maskVideoByUserId(String userId, boolean mask);

//    public native static void setMixVideoSize(int width, int height);

//    public native static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height);

//    public native static void removeMixOverlayVideo(String userId);

    //    public native static void removeMixAllOverlayVideo();
    public static native void setLogLevel(int consoleLevel, int fileLevel);

    public static native void setServerRegion(int region, String extServerName, boolean bAppend);

    public static native int init(String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);

    public static native int unInit();

    public static native boolean isInited();

    public native static void setExternalInputMode(boolean bInputModeEnabled);

    public static native int setExternalInputSampleRate(int inputSampleRate, int mixedCallbackSampleRate);

    public native static int setVideoCallback();

//    public native static void setTokenV3(String strToken, long timeStamp);

//    public native static int setTCPMode(boolean bUseTcp);

//    public static native int setUserLogPath(String filePath);

//    public static native boolean getUseMobileNetworkEnabled();

//    public static native void setUseMobileNetworkEnabled(boolean bEnabled);

    public static native int setVideoNetResolution(int width, int height);

    public static native int setVideoLocalResolution(int width, int height);

    public static native void setVideoCodeBitrate(int maxBitrate, int minBitrate);

    public static native void setVBR(boolean useVBR);

    public static native int getCurrentVideoCodeBitrate();

    public static native int setVideoPreviewFps(int fps);

    public static native int setVideoFps(int fps);

    public static native void setVideoHardwareCodeEnable(boolean bEnable);

    public static native boolean getVideoHardwareCodeEnable();

    public static native boolean getUseGL();

    public static native void setVideoNoFrameTimeout(int timeout);

    public static native int setVadCallbackEnabled(boolean enabled);


    public static native int joinChannelSingleMode(String strUserID, String strRoomID, int userRole, boolean autoRecv);

    public static native int leaveChannelAll();


    public native static int startCapture();

    public native static void stopCapture();

    public native static int switchCamera();

    public native static void resetCamera();

    public native static boolean inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

//    public native static boolean inputVideoFrameEncrypt(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp, int streamID);

    public native static boolean inputVideoFrameForShare(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

//    public native static boolean inputVideoFrameByteBuffer(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

//    public native static boolean inputVideoFrameByteBufferForShare(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static boolean inputVideoFrameGLES(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static boolean inputVideoFrameGLESForShare(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static void stopInputVideoFrame();

    public native static void stopInputVideoFrameForShare();


//    public static native int setOtherMicMute(String strUserID, boolean status);
//    public static native int setMaskOtherVoice(String strUserID, boolean on);

    public static native int setOutputToSpeaker(boolean bOutputToSpeaker);

    public static native void setSpeakerMute(boolean bOn);

    public static native boolean getSpeakerMute();

    public static native boolean getMicrophoneMute();

    public static native void setMicrophoneMute(boolean mute);

    public native static void resetMicrophone();

    public static native int getVolume();

    public static native void setVolume(int uiVolume);

    public static native int setMicLevelCallback(int maxLevel);

    public static native int setFarendVoiceLevelCallback(int maxLevel);

//    public static native int setReleaseMicWhenMute(boolean enabled);

    public native static boolean inputAudioFrame(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved);

    public native static boolean inputAudioFrameForMix(int streamId, byte[] data, int len, YMAudioFrameInfo frameInfo, long timestamp);


    public static native int pauseChannel();

    public static native int resumeChannel();

    //	public static native int setExitCommModeWhenHeadsetPlugin(boolean enabled);

//    public static native void setAVStatisticInterval(int interval);

    public static native boolean isJoined();

//    public static native int queryUsersVideoInfo(String[] userArray);

//    public static native int setUsersVideoInfo(String[] userArray, int[] resolutionArray);

//    public static native int setVideoDecodeRawCbEnabled(boolean enabled);

//    public static native int setVideoFrameRawCbEnabled(boolean enabled);

//    public static native void setlocalVideoPreviewMirror(boolean enable);

    public static native int switchResolutionForLandscape();

    public static native int resetResolutionForPortrait();

    //直播业务
    // public static native int setUserRole(int userRole);
    // public static native int getUserRole();
    // public static native boolean isInChannel(String strChannelID);
    // public static native boolean isBackgroundMusicPlaying();

    //无用接口
    // public static native int setCaptureFrontCameraEnable(boolean enable);

    //小流配置
    // public static native int setVideoNetResolutionForSecond(int width, int height);
    // public static native void setVideoCodeBitrateForSecond(int maxBitrate, int minBitrate);
    // public static native void setVBRForSecond(boolean useVBR);
    // public static native int setVideoFpsForSecond(int fps);

    //共享流配置
    // public static native int setVideoNetResolutionForShare(int width, int height);
    // public static native void setVideoCodeBitrateForShare(int maxBitrate, int minBitrate);
    // public static native int setVBRForShare(boolean useVBR);
    // public static native int setVideoFpsForShare(int fps);


    // public native static Object sharedEGLContext();
    public native static void logcat(int mode, String tag, String msg);

}
