package com.youme.voiceengine;

import java.nio.ByteBuffer;

public class NativeEngine {
    public native static void setModel(String model);

    public native static void setSysName(String sysname);

    public native static void setSysVersion(String sysversion);

    public native static void setVersionName(String versionName);

    public native static void setDeviceURN(String deviceURN);

    public native static void setDeviceIMEI(String deviceIMEI);

    public native static void setCPUArch(String ABI);

    public native static void setCPUChip(String strCPUChip);

    public native static void setUUID(String UUID);

    public native static void setBrand(String brand);

    public native static void setPackageName(String packageName);

    public native static int init(String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);

    public native static void unInit();

    public native static void setDocumentPath(String path);

    public native static void setUserLogPath(String filePath);

    public native static void setSpeakerMute(boolean bOn);
    public native static boolean getSpeakerMute();
    public native static void setMicrophoneMute(boolean bOn);
    public native static boolean getMicrophoneMute();
    public native static void setVolume(int volume);
    public native static int getVolume();

    public native static void onNetWorkChanged(int type);

    public native static void onHeadSetPlugin(int state);

    public native static String getSoVersion();

    public native static void setScreenOrientation(int orientation);

    public native static boolean inputAudioFrame(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved);

    public native static boolean inputAudioFrameForMix(int streamId, byte[] data, int len, YMAudioFrameInfo frameInfo, long timestamp);

    public native static int inputCustomData(byte[] data, int len, long timestamp);

    public native static int inputCustomDataToUser(byte[] data, int len, long timestamp, String userId);

    public native static void VideoCaptureBufRefresh(byte[] videoBuf, int bufSize, int fps, int rotationDegree, boolean mirror, int width, int height, int videoFmt);

    public native static int startCapture();

    public native static void stopCapture();

    public native static int switchCamera();

    public native static void openBeautify(boolean open);

    public native static void setBeautyLevel(float level);

    public native static void callbackPermissionStatus(int errCode);

    public native static boolean inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static boolean inputVideoFrameEncrypt(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp, int streamID);

    public native static boolean inputVideoFrameForShare(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static boolean inputVideoFrameByteBuffer(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static boolean inputVideoFrameByteBufferForShare(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);

    public native static void stopInputVideoFrame();

    public native static void stopInputVideoFrameForShare();
    public native static int  switchResolutionForLandscape();

    public native static void setlocalVideoPreviewMirror(boolean enable);
}
