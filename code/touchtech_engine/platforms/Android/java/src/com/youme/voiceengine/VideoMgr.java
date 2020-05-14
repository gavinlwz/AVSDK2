package com.youme.voiceengine;

import com.youme.voiceengine.video.SurfaceViewRenderer;

import android.util.Log;

/**
 * 视频控制管理
 * 
 * @author fire
 *
 */
public class VideoMgr {
	private final static String TAG = "VideoMgr";
	private final static int DEFAULE_WIDTH = 320;
	private final static int DEFAULE_HEIGHT = 480;
	private final static int DEFAULE_FPS = 15;
	
	private static boolean isFrontCamera = true; /* 是否前置摄像头 */
	private static int width = DEFAULE_WIDTH; /* 宽 */
	private static int height = DEFAULE_HEIGHT; /* 高 */
	private static int fps = DEFAULE_FPS; /* 每秒帧数 */
	private static int rotation = 0; /* 摄像头旋转度数 */
	
	private static int cameraWidth; /* 摄像头实际使用的宽 */
	private static int cameraHeight; /* 摄像头实际使用的高 */
	private static int cameraFps; /* 摄像头实际使用的FPS */
	
	private static SurfaceViewRenderer localView;
	
	public static void init() {

	}
	
	public static void unInit() {

	}
	
	public static void setCamera(int width, int height, int fps) {
		
		boolean isNeedReset = false;
		
		if (VideoMgr.width != width) {
			VideoMgr.width = width;
			isNeedReset = true;
		}
		
		if (VideoMgr.height != height) {
			VideoMgr.height = height;
			isNeedReset = true;
		}
		
		if (VideoMgr.fps != fps) {
			VideoMgr.fps = fps;
			isNeedReset = true;
		}
		
		if (isNeedReset) {
			resetCamera(isFrontCamera, width, height, fps, rotation);
		}
        Log.d(TAG , "setCamera: width="+width+" height="+height+" fps="+fps);
	}
	
	public static void resetCamera (boolean front, int width, int height, int fps, int rotation) {
		
		Log.d(TAG, "front: " + front + "width: " + width + "height: " + height + "fps: " + fps + "rotation: " + rotation);
		if (localView != null) {
			localView.resetLayout(width, height, rotation);
		}
	}

	public static boolean isFrontCamera() {
		return isFrontCamera;
	}

	public static void setFrontCamera(boolean isFrontCamera) {
		if (VideoMgr.isFrontCamera == isFrontCamera) {
			return;
		}
		
		VideoMgr.isFrontCamera = isFrontCamera;
		// reset Camera front back
	}

	public static int getWidth() {
		return width;
	}

	public static void setWidth(int width) {
		VideoMgr.width = width;
	}

	public static int getHeight() {
		return height;
	}

	public static void setHeight(int height) {
		VideoMgr.height = height;
	}

	public static int getFps() {
		return fps;
	}

	public static void setFps(int fps) {
		VideoMgr.fps = fps;
	}

	public static int getRotation() {
		return rotation;
	}

	public static void setRotation(int rotation) {
		if (VideoMgr.rotation == rotation) {
			return;
		}
		VideoMgr.rotation = rotation;
		// reset Camera rotation
	}

	public static int getCameraWidth() {
		return cameraWidth;
	}

	public static void setCameraWidth(int cameraWidth) {
		VideoMgr.cameraWidth = cameraWidth;
	}

	public static int getCameraHeight() {
		return cameraHeight;
	}

	public static void setCameraHeight(int cameraHeight) {
		VideoMgr.cameraHeight = cameraHeight;
	}

	public static int getCameraFps() {
		return cameraFps;
	}

	public static void setCameraFps(int cameraFps) {
		VideoMgr.cameraFps = cameraFps;
	}

	public static SurfaceViewRenderer getLocalView() {
		return localView;
	}

	public static void setLocalView(SurfaceViewRenderer localView) {
		VideoMgr.localView = localView;
	}
	
	public interface VideoFrameCallback {
		/**
		 * 回调远端视频数据
		 * @param userId     用户id
		 * @param data       一帧视频数据
		 * @param len        视频数据的大小
		 * @param width      宽
		 * @param height     高
		 * @param fmt        数据类型，参考YOUME_VIDEO_FMT
		 * @param timestamp  时间戳
		 * @return void
		 */
		void onVideoFrameCallback(String userId, byte[] data, int len, int width, int height, int fmt, long timestamp);

		/**
    	 * 回调的合流后的视频数据
    	 * 
    	 * @param data       一帧视频数据
    	 * @param len        视频数据的大小
    	 * @param width      宽
    	 * @param height     高
    	 * @param fmt        数据类型，参考YOUME_VIDEO_FMT
    	 * @param timestamp  时间戳
    	 * @return void
    	 */
    	void onVideoFrameMixed(byte[] data, int len, int width, int height, int fmt, long timestamp);
    	
    	/**
		 * 远端视频数据回调
		 * @param userId     用户id
		 * @param type       数据类型，参考 YOUME_VIDEO_FMT
		 * @param texture    纹理id
		 * @param matrix     纹理坐标矩阵       
		 * @param width      纹理宽度
		 * @param height     纹理高度
		 * @param timestamp  时间戳
		 * @return void
		 */
    	void onVideoFrameCallbackGLES(String userId, int type, int texture, float[] matrix, int width, int height, long timestamp);
    	
    	/**
		 * 合流后数据回调
		 * @param userId     用户id
		 * @param texture    纹理id
		 * @param matrix     纹理坐标矩阵       
		 * @param width      纹理宽度
		 * @param height     纹理高度
		 * @param timestamp  时间戳
		 * @return void
		 */
    	void onVideoFrameMixedGLES(int type, int texture, float[] matrix, int width, int height, long timestamp);
    	
    	/**
		 * 自定义滤镜回调
		 * @param userId     用户id
		 * @param texture    纹理id  
		 * @param width      纹理宽度
		 * @param height     纹理高度
		 * @param rotation   图像需要旋转度数（0-360），目前为0
		 * @param mirror     图像水平镜像，1需要镜像，目前为0
		 * @return void
		 */
    	int onVideoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror);
    };
}
