package com.youme.voiceengine;

public interface YouMeVideoPreDecodeCallbackInterface {

	public void onVideoPreDecode(String userID, byte[]data, int dataSizeInByte, long timestamp);
}
