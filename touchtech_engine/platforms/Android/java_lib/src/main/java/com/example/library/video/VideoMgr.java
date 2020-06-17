package main.java.com.example.library.video;

import android.util.Log;

import main.java.com.example.library.IYouMeVideoCallback;


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

    public static void setVideoFrameCallback(VideoFrameCallbackInterface cb) {
        IYouMeVideoCallback.mVideoFrameExternalCallback = cb;
    }
    /**
     *
     * @param width   合流后的画面宽
     * @param height  合流后的画面高
     */
//    public static void setMixVideoSize(int width, int height) {
//    	NativeEngine.setMixVideoSize(width, height);
//    }
    
    /**
     * @param userId   哪一路用户的视频
     * @param x  图像的坐标 x坐标
     * @param y  图像的坐标 y坐标
     * @param z  图像的坐标 z坐标       
     * @param width           
     * @param height                  
     * */
//    public static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height) {
//    	NativeEngine.addMixOverlayVideo(userId, x, y, z, width, height);
//    }
    
    /**
     * @param userId   哪一路用户的视频
     *                 
     * */
    public static void removeMixOverlayVideo(String userId) {
//    	NativeEngine.removeMixOverlayVideo(userId);
    }
}
