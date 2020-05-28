package main.java.com.example.library.audio;

public class YouMeAudioCallback {
	
	private final static String tag = YouMeAudioCallback.class.getSimpleName();
	
	public static AudioFrameCallbackInterface callback = null;
	
	public static void onAudioFrameCallback(String userId, byte[] data, int len, long timestamp) {
		if(callback != null) {
			callback.onAudioFrameCallback(userId, data, len, timestamp);
		}
	}

	public static void onAudioFrameMixedCallback(byte[] data, int len, long timestamp) {
		if(callback != null) {
			callback.onAudioFrameMixed(data, len, timestamp);
		}
	}
}
