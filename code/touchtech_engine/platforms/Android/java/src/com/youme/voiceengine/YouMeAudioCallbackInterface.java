package com.youme.voiceengine;

import android.util.Log;

public interface YouMeAudioCallbackInterface {
	
	/**
	 * Android 回调的音频数据
	 *
	 * @param userId
	 * @param data
	 * @param len
	 * @param timestamp     
	 * @return void
	 */
	public void onAudioFrameCallback(String userId, byte[] data, int len, long timestamp);

	/**
	 * Android 回调的合流后的音频数据
	 *
	 * @param data
	 * @param len
	 * @param timestamp     
	 * @return void
	 */
	public void onAudioFrameMixed(byte[] data, int len, long timestamp);
}
