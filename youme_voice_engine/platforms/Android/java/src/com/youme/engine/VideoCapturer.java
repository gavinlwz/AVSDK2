package com.youme.engine;

import android.content.Context;
import android.util.Log;

/***
 * 摄像头采集
 * 
 * @author fire
 *
 */
public class VideoCapturer {
	private static String TAG = "VideoCapturer";
	/***
	 * 开始采集
	 */
	public static void StartCapturer() {
        NativeEngine.startCapture();
	}
	
	/***
	 * 设置本地视频渲染回调的分辨率
	 */
//	public static void SetVideoLocalResolution(int width, int height) {
//        api.setVideoLocalResolution(width, height);
//	}

	/***
	 * 设置是否前置摄像头
	 */
//	public static void SetCaptureFrontCameraEnable(boolean enable) {
//        api.setCaptureFrontCameraEnable(enable);
//	}
	
	
	/***
	 * 停止采集
	 */
	public static void StopCapturer() {
		NativeEngine.stopCapture();
	}
	
	/***
	 * 切换前后摄像头
	 */
	public static void SwitchCamera() {
		NativeEngine.switchCamera();
	}
	
	/***
	 * 设置美颜
	 */
//	public static void SetBeautyLevel(float level) {
//		NativeEngine.setBeautyLevel(level);
//	}
	
	/**
	 * 初始化camera
	 * @param context:Activity
	 */
	public static void initCamera(Context context)
	{
		CameraMgr.init(context);
	}
	
//	public static void Beauty(boolean enable)
//	{
//		CameraMgr.Beauty(enable);
//	}
	
	
	public static void VideoCaptuered (byte[] videoBuf, int bufSize, int fps, int rotationDegree, boolean mirror, int width, int height, int videoFmt) {
	    //api.inputVideoFrame(videoBuf, bufSize, width, height, rotationDegree, mirror, mFmt, System.currentTimeMillis());
		NativeEngine.VideoCaptureBufRefresh(videoBuf, bufSize, fps, rotationDegree, mirror, width, height, videoFmt);
	}
	
	
	
	/**
	 * YV12 转 YUV420P
	 * @param data
	 * @param w
	 * @param h
	 * @return
	 */
	public static void YV12ToYUV420P(byte[] data, int w, int h) {
        int yLen = w * h;
		int vLen = yLen / 4;
		
		if (data == null || data.length != yLen * 3 / 2) {
			Log.e(TAG, "YV12ToYUV420P data legth error! ");
			return;
		}
		
		byte[] vData = new byte[vLen];
		
		System.arraycopy(data, yLen, vData, 0, vLen); // v plane tmp
		System.arraycopy(data, yLen + vLen, data, yLen, vLen);
		System.arraycopy(vData, 0, data, yLen + vLen, vLen);
	}
    
    /**
     * NV21 转 YUV420P
     * @param data
     * @param w
     * @param h
     * @return
     */
    public static void NV21ToYUV420P(byte[] data, int w, int h) {
        int yLen = w * h;
        int vLen = yLen / 4;
        int index = 0;
        
        if (data == null || data.length != yLen * 3 / 2) {
            Log.e(TAG, "NV21ToYUV420P data legth error! ");
            return;
        }
        
        byte[] vData = new byte[vLen];
        
        for (index = 0; index < vLen; index++) { // v plane tmp
            vData[index] = data[yLen + index * 2];
        }
        for (index = 0; index < vLen; index++) { // shift the u plane
            data[yLen + index] = data[yLen + index * 2 + 1];
        }
        System.arraycopy(vData, 0, data, yLen + vLen, vLen); // move the v plane to data
    }
}
