package main.java.com.example.library;
import java.nio.ByteBuffer;

public class NativeEngine
{
	public native static void setModel (String model);
	public native static void setSysName (String sysname);
	public native static void setSysVersion (String sysversion);
	public native static void setVersionName (String versionName);
	public native static void setDeviceURN (String deviceURN);
	public native static void setDeviceIMEI (String deviceIMEI);
	public native static void setCPUChip(String strCPUChip);
	public native static void setCPUArch (String ABI);
	public native static void setBrand (String brand);
	public native static void setUUID (String UUID);
	public native static void setPackageName (String packageName);
	public native static void setDocumentPath (String path);

	public native static String getSoVersion();
	public native static void initWithAppkey(String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);
	public native static void inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp);
	public native static void inputAudioFrame(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved);
    public native static int  startCapture();
    public native static void stopCapture();
    public native static int  switchCamera();
	public native static void onNetWorkChanged (int type);

	public native static void AudioRecorderBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);
	public native static void AudioPlayerBufRefresh(ByteBuffer audBuf, int samplerate, int channelnum, int bps);

	public native static void setServerMode(int mode);
	public native static void setServerIpPort(String ip, int port);
}
