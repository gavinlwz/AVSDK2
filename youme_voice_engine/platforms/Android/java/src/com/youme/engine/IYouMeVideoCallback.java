package com.youme.engine;
import com.youme.engine.VideoMgr.VideoFrameCallback;

public class IYouMeVideoCallback {

//	public static YouMeVideoCallbackInterface callback = null;
	public static VideoFrameCallback mVideoFrameRenderCallback = null;
	public static VideoFrameCallback mVideoFrameExternalCallback = null;
//	public static YouMeVideoPreDecodeCallbackInterface mVideoPreDecodeCallback = null;
//	public static void FrameRender(int renderId, int width, int height, int rotationDegree, byte[] yuvframe) 
//	{
//		//Log.i("YouMeApi", "IYouMeVideoCallback FrameRender");
//		
//		if (callback != null){
//			callback.FrameRender(renderId, width, height, rotationDegree, yuvframe);
//		}
//	}
//	
//	public static void FrameRenderGLES(int renderId, int width, int height, int type, int texture, float[]matrix){
//		if (callback != null){
//			callback.FrameRenderGLES(renderId, width, height, type, texture, matrix);
//			
//			//Log.d("YouMeApi", "IYouMeVideoCallback FrameRenderGLES texture:"+texture + " w:" +width+" h:"+height);
//		}
//	}
	
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

//	public static void onVideoPreDecode(String userID, byte[] data, int dataSizeInByte, long timestamp)
//	{
//		if (null != mVideoPreDecodeCallback)
//		{
//			mVideoPreDecodeCallback.onVideoPreDecode(userID, data, dataSizeInByte, timestamp);
//		}
//	}
}
