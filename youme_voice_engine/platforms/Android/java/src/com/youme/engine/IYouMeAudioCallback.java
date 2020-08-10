package com.youme.engine;

public class IYouMeAudioCallback {
	
	private final static String tag = IYouMeAudioCallback.class.getSimpleName();
	
	public static YouMeAudioCallbackInterface callback = null;
	public static YouMePcmCallBackInterface mCallbackPcm = null;
	public static YouMePcmCallBackInterfaceForUnity mCallbackPcmForUntiy = null;


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

	public static void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
	{
		if (mCallbackPcm != null) {
			mCallbackPcm.onPcmDataRemote(channelNum, samplingRateHz, bytesPerSample, data);
		}else if(mCallbackPcmForUntiy != null){
			mCallbackPcmForUntiy.onPcmDataRemote(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
		}
	}

	public static void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
	{
		if (mCallbackPcm != null) {
			mCallbackPcm.onPcmDataRecord(channelNum, samplingRateHz, bytesPerSample, data);
		}else if(mCallbackPcmForUntiy != null){
			mCallbackPcmForUntiy.onPcmDataRecord(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
		}
	}

	public static void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data)
	{
		if (mCallbackPcm != null) {
			mCallbackPcm.onPcmDataMix(channelNum, samplingRateHz, bytesPerSample, data);
		}else if(mCallbackPcmForUntiy != null){
			mCallbackPcmForUntiy.onPcmDataMix(channelNum, samplingRateHz, bytesPerSample, new YouMePcmDataForUnity(data));
		}
	}
}
