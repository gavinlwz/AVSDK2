package com.youme.engine;
import com.youme.engine.VideoMgr.VideoFrameCallback;

public class IYouMeVideoCallback {

	public static VideoFrameCallback mVideoFrameRenderCallback = null;
	public static VideoFrameCallback mVideoFrameExternalCallback = null;

	public static void onVideoFrameCallback(String userId, byte[] data, int len, int width, int height, int fmt, long timestamp) {
		if(null != mVideoFrameRenderCallback) {
			mVideoFrameRenderCallback.onVideoFrameCallback(userId, data, len, width, height, fmt, timestamp);
		}
		if(null != mVideoFrameExternalCallback) {
			mVideoFrameExternalCallback.onVideoFrameCallback(userId, data, len, width, height, fmt, timestamp);
		}
	}
	
	public static void onVideoFrameMixedCallback(byte[] data, int len, int width, int height, int fmt, long timestamp) {
		if(null != mVideoFrameRenderCallback) {
			mVideoFrameRenderCallback.onVideoFrameMixed(data, len, width, height, fmt, timestamp);
		}
		if(null != mVideoFrameExternalCallback) {
			mVideoFrameExternalCallback.onVideoFrameMixed(data, len, width, height, fmt, timestamp);
		}
	}
	
	public static void onVideoFrameCallbackGLES(String userId, int fmt, int texture, float[] matrix, int width, int height,  long timestamp) {
		if(null != mVideoFrameRenderCallback) {
			mVideoFrameRenderCallback.onVideoFrameCallbackGLES(userId, fmt, texture, matrix, width, height, timestamp);
		}
		if(null != mVideoFrameExternalCallback) {
			mVideoFrameExternalCallback.onVideoFrameCallbackGLES(userId, fmt, texture, matrix, width, height, timestamp);
		}
	}
	
	public static void onVideoFrameMixedCallbackGLES(int fmt, int texture, float[] matrix, int width, int height,  long timestamp) {
		if(null != mVideoFrameRenderCallback) {
			mVideoFrameRenderCallback.onVideoFrameMixedGLES(fmt, texture, matrix, width, height, timestamp);
		}
		if(null != mVideoFrameExternalCallback) {
			mVideoFrameExternalCallback.onVideoFrameMixedGLES(fmt, texture, matrix, width, height, timestamp);
		}
	}
	
	public static int onVideoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror){
		if(null != mVideoFrameExternalCallback) {
			return mVideoFrameExternalCallback.onVideoRenderFilterCallback(texture, width, height, rotation, mirror);
		}
	    return 0;
	}
}
