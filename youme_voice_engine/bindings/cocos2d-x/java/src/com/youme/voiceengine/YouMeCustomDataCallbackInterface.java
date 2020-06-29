package com.youme.voiceengine;

import android.util.Log;

public interface YouMeCustomDataCallbackInterface {
	/**
	 * Android 回调的合流后的音频数据
	 *
	 * @param data
	 * @param timestamp     
	 * @return void
	 */
	public void onRecvCustomData(byte[] data, long timestamp);
}
