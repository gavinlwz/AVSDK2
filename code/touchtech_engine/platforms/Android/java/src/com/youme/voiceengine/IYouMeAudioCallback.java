package com.youme.voiceengine;

import android.util.Log;

public class IYouMeAudioCallback {
	
	private final static String tag = IYouMeAudioCallback.class.getSimpleName();
	
	public static YouMeAudioCallbackInterface callback = null;
	
	public static void onAudioFrameCallback(String userId, byte[] data, int len, long timestamp) {
//		Log.i(tag, "onAudioFrameCallback.");
		
		if(callback != null) {
			callback.onAudioFrameCallback(userId, data, len, timestamp);
		}
	}

	public static void onAudioFrameMixedCallback(byte[] data, int len, long timestamp) {
//		Log.i(tag, "onAudioFrameMixedCallback.");
		
		if(callback != null) {
			callback.onAudioFrameMixed(data, len, timestamp);
		}
	}
}
