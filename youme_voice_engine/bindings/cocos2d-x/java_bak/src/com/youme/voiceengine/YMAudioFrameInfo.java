package com.youme.voiceengine;

public class YMAudioFrameInfo{
	public int channels;
	public int sampleRate;
	public int bytesPerFrame;
	public boolean isFloat;
	public boolean isBigEndian;
	public boolean isSignedInteger;
	public boolean isNonInterleaved;
	public long timestamp;

	public YMAudioFrameInfo(int channels, int sampleRate, int bytesPerFrame, boolean isFloat, boolean isBigEndian, boolean isSignedInteger, boolean isNonInterleaved, long timestamp){
		this.channels = channels;
		this.sampleRate = sampleRate;
		this.bytesPerFrame = bytesPerFrame;
		this.isFloat = isFloat;
		this.isBigEndian = isBigEndian;
		this.isSignedInteger = isSignedInteger;
		this.isNonInterleaved = isNonInterleaved;
		this.timestamp = timestamp;
	}
}