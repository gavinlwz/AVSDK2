package com.youme.voiceengine;
import java.nio.ByteBuffer;

public class NativeEngine
{
	public native static void setModel (String model);

	public native static void setSysName (String sysname);

	public native static void setSysVersion (String sysversion);

	public native static void setVersionName (String versionName);

	public native static void setDeviceURN (String deviceURN);

	public native static void setDeviceIMEI (String deviceIMEI);

	public native static void setCPUArch (String ABI);

	public native static void setPackageName (String packageName);

	public native static void setUUID (String UUID);

	public native static void setDocumentPath (String path);
	
	public native static void onNetWorkChanged (int type);
	public native static void onHeadSetPlugin (int state);
	
	public native static String getSoVersion();
	public native static void setBrand (String brand);
	
	public native static void setCPUChip(String strCPUChip);
	
    // Definitions of server mode available. See definition for TEST_MODE_t in the native layer.
	public static final int SERVER_MODE_FORMAL  = 0; // Formal environment.
	public static final int SERVER_MODE_TEST    = 1; // Using test servers.
	public static final int SERVER_MODE_DEV     = 2; // Using development servers.
	public static final int SERVER_MODE_DEMO    = 3; // Using special servers for buniness demo only.
	public static final int SERVER_MODE_FIXED_IP_VALIDATE    = 4;
	public static final int SERVER_MODE_FIXED_IP_REDIRECT    = 5;
	public static final int SERVER_MODE_FIXED_IP_MCU         = 6;
	public static final int SERVER_MODE_FIXED_IP_PRIVATE_SERVICE	= 7;
	
	public native static void setServerMode(int mode);
	public native static void setServerIpPort(String ip, int port);

	public native static void setScreenOrientation(int orientation);
    public native static void AudioRecorderBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);
    public native static void AudioPlayerBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);
    public native static boolean inputAudioFrame(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved);
	public native static boolean inputAudioFrameForMix(int streamId, byte[] data, int len, YMAudioFrameInfo frameInfo, long timestamp);
	public native static int inputCustomData(byte[] data, int len, long timestamp);
	public native static int inputCustomDataToUser(byte[] data, int len, long timestamp, String userId);
    public native static void VideoCaptureBufRefresh(byte[] videoBuf, int bufSize, int fps, int rotationDegree, boolean mirror, int width, int height, int videoFmt);
    public native static int  startCapture();
    public native static void stopCapture();
    public native static int  switchCamera();

	public native static void openBeautify( boolean open );
    public native static void setBeautyLevel(float level);

    public native static void resetMicrophone();
	public native static void callbackPermissionStatus(int errCode);
    public native static void resetCamera();
    public native static void setPcmCallbackEnable ( int flag , boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel);
	public native static void setVideoPreDecodeCallbackEnable ( boolean enable, boolean needDecodeandRender);
	
    public native static void maskVideoByUserId(String userId, boolean mask);
	public native static boolean inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static boolean inputVideoFrameEncrypt(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp, int streamID);
	public native static boolean inputVideoFrameForShare(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static boolean inputVideoFrameByteBuffer(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static boolean inputVideoFrameByteBufferForShare(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static boolean inputVideoFrameGLES(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static boolean inputVideoFrameGLESForShare(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static void stopInputVideoFrame();
	public native static void stopInputVideoFrameForShare();
    public native static void setMixVideoSize(int width, int height);
    public native static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height);
    public native static void removeMixOverlayVideo(String userId);
    public native static void removeMixAllOverlayVideo();
	public native static  boolean isJoined();
    public native static Object sharedEGLContext();
	public native static void  setLogLevel(  int consoleLevel, int fileLevel );
    public static final int LOG_INFO = 1;
    public static final int LOG_WARN = 2;
    public static final int LOG_ERROR = 3;
    public native static void logcat(int mode, String tag, String msg);

}
