package com.youme.engine;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Map;

import com.youme.engine.VideoMgr.VideoFrameCallback;
import com.youme.engine.YouMeConst.YOUME_VIDEO_FMT;
import com.youme.engine.video.SurfaceViewRenderer;
import com.youme.engine.video.VideoBaseRenderer;
import com.youme.engine.video.VideoBaseRenderer.I420Frame;

import android.util.Log;

/**
 * 摄像头渲染
 * 
 * @author fire
 *
 */
public class VideoRenderer implements VideoFrameCallback {
	private final static String TAG = "VideoRenderer";
	private Map<String, SurfaceViewRenderer> renderers = new HashMap<String, SurfaceViewRenderer>();
	
	private static final VideoRenderer instance = new VideoRenderer();
	private int rotation = 0;
	private String localUserId ="";
	
	private VideoRenderer()	{
		// 私有构造
	}
	
	public static VideoRenderer getInstance() {
		return instance;
	}
	
	public void init() {
		renderers.clear();
	}

	private boolean mPause = false;
	public void pause(){
		mPause = true;
	}

	public void resume(){
		mPause = false;
	}
	
	public void setLocalUserId(String userId){
		localUserId = userId;
	}
	
	/**
	 * 添加渲染源
	 * @param view
	 * @return
	 */
	public int addRender(String userId, SurfaceViewRenderer view) {
		
		//int renderId = api.createRender(userId);
		
		renderers.put(userId, view);
		Log.d(TAG, "addRender userId:"+userId);
		return 0;
	}
	
	/**
	 * 删除视频源
	 * @param renderId
	 * @return
	 */
	public int deleteRender(String userId) {
		//int ret = api.deleteRender(renderId);
		SurfaceViewRenderer view = renderers.remove(userId);
		if (view == null) {
			return -1;
		}
		
		Log.d(TAG, "deleteRender userId:"+userId);
		return 0;
	}
	
	public void deleteAllRender(){
		renderers.clear();
	}
	
	/***
	 * Frame渲染
	 * @param renderId
	 * @param frame
	 */
//	public void FrameRender(int sessionId, int width, int height, int rotationDegree, byte[] yuvframe) {
//		SurfaceViewRenderer view = renderers.get(Integer.valueOf(sessionId));
//        //Log.d(TAG, "[paul debug] Render video frame!" + yuvframe.length + " sessionId:" + sessionId);
//		if (view != null) {
//			int[] yuvStrides = {width, width / 2, width / 2};
//			
//			int yLen = width * height;
//            int uLen = width * height / 4;
//            int vLen = width * height / 4;
//            byte[] yPlane = new byte[yLen];
//            byte[] uPlane = new byte[uLen];
//            byte[] vPlane = new byte[vLen];
//            
//            /*
//            try {
//            	File f = new File(Environment.getExternalStorageDirectory(), "/youme/video_render.yuv");
//            	if (!f.exists()) {
//            		f.createNewFile();
//            	}
//               	BufferedOutputStream out = new BufferedOutputStream(new FileOutputStream(f, true));
//               	out.write(yuvframe, 0, yuvframe.length);
//               	out.flush();
//            	out.close();
//            } catch (Exception e) {
//            	return ;
//            }*/
//            
//            System.arraycopy(yuvframe, 0, yPlane, 0, yLen);
//            System.arraycopy(yuvframe, yLen, uPlane, 0, uLen);
//            System.arraycopy(yuvframe, (yLen + uLen), vPlane, 0, vLen);
//            
//			ByteBuffer[] yuvPlanes = {ByteBuffer.wrap(yPlane), ByteBuffer.wrap(uPlane), ByteBuffer.wrap(vPlane)};
//			
//			//rotationDegree = 270; // for android
//			
//			I420Frame frame = new I420Frame(width, height, rotationDegree - rotation, yuvStrides, yuvPlanes);
//			view.renderFrame(frame);
//		} else {
//			Log.e(TAG, "SurfaceViewRenderer not found, sessionId : " + sessionId);
//		}
//	}
//	
//	public void FrameRenderGLES(int sessionId, int width, int height, int type, int texture, float[] matrix){
//		SurfaceViewRenderer view = renderers.get(Integer.valueOf(sessionId));
//		if (view != null) {
//			I420Frame frame = new I420Frame(width, height, 0, texture, matrix, type == 1);
//			view.renderFrame(frame);
//		}
//	}
	
	public void onVideoFrameCallback(String userId, byte[] data, int len, int width, int height, int fmt, long timestamp){
		SurfaceViewRenderer view = renderers.get(userId);
		if (!mPause && view != null) {
			int[] yuvStrides = {width, width / 2, width / 2};
			
			int yLen = width * height;
            int uLen = width * height / 4;
            int vLen = width * height / 4;
            byte[] yPlane = new byte[yLen];
            byte[] uPlane = new byte[uLen];
            byte[] vPlane = new byte[vLen];
            
            System.arraycopy(data, 0, yPlane, 0, yLen);
            System.arraycopy(data, yLen, uPlane, 0, uLen);
            System.arraycopy(data, (yLen + uLen), vPlane, 0, vLen);
            
			ByteBuffer[] yuvPlanes = {ByteBuffer.wrap(yPlane), ByteBuffer.wrap(uPlane), ByteBuffer.wrap(vPlane)};
			
			//rotationDegree = 270; // for android
			
			VideoBaseRenderer.I420Frame frame = new I420Frame(width, height, 0, yuvStrides, yuvPlanes);
			view.renderFrame(frame);
		} else {
			//Log.e(TAG, "SurfaceViewRenderer not found, sessionId : " + userId);
		}
		
     		
	}

	public void onVideoFrameMixed(byte[] data, int len, int width, int height, int fmt, long timestamp){
		String userId = localUserId;
		SurfaceViewRenderer view = renderers.get(userId);
		if (!mPause && view != null) {
			int[] yuvStrides = {width, width / 2, width / 2};
			
			int yLen = width * height;
            int uLen = width * height / 4;
            int vLen = width * height / 4;
            byte[] yPlane = new byte[yLen];
            byte[] uPlane = new byte[uLen];
            byte[] vPlane = new byte[vLen];
            
            System.arraycopy(data, 0, yPlane, 0, yLen);
            System.arraycopy(data, yLen, uPlane, 0, uLen);
            System.arraycopy(data, (yLen + uLen), vPlane, 0, vLen);
            
			ByteBuffer[] yuvPlanes = {ByteBuffer.wrap(yPlane), ByteBuffer.wrap(uPlane), ByteBuffer.wrap(vPlane)};
			
			//rotationDegree = 270; // for android
			
			VideoBaseRenderer.I420Frame frame = new I420Frame(width, height, 0, yuvStrides, yuvPlanes);
			view.renderFrame(frame);
		} else {
			//Log.e(TAG, "SurfaceViewRenderer not found, sessionId : " + userId);
		}
	}
	
	public void onVideoFrameCallbackGLES(String userId, int type, int texture, float[] matrix, int width, int height, long timestamp){
		SurfaceViewRenderer view = renderers.get(userId);
		if (!mPause && view != null) {
			VideoBaseRenderer.I420Frame frame = new I420Frame(width, height, 0, texture, matrix, type == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES);
			view.renderFrame(frame);
		}
	}
	
	public void onVideoFrameMixedGLES(int type, int texture, float[] matrix, int width, int height, long timestamp){
		String userId = localUserId;
		SurfaceViewRenderer view = renderers.get(userId);
		if (!mPause && view != null) {
			VideoBaseRenderer.I420Frame frame = new I420Frame(width, height, 0, texture, matrix, type == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES);
			view.renderFrame(frame);
		}
	}
	
	public int onVideoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror){
		return 0;
	}

	public int getRotation() {
		return rotation;
	}

	public void setRotation(int rotation) {
		this.rotation = rotation;
	}
}
