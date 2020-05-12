package com.youme.voiceengine;

public interface YouMeCallBackInterfacePcm {
	//远端pcm回调
	public void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
	//录音pcm回调
	public void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
	//远端和录音mix的pcm回调
	public void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
}
