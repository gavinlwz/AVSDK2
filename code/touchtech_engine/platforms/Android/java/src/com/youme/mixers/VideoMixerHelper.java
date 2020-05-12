package com.youme.mixers;

import com.youme.voiceengine.NativeEngine;

import android.graphics.SurfaceTexture;
import android.opengl.EGLContext;

/**
 * Created by bhb on 2018/1/15.
 */

public class VideoMixerHelper {
	
	public static void initMixer(){
		GLESVideoMixer.getInstance().startMixer();
	}

    public static void setVideoMixEGLContext(Object eglContext) {
        GLESVideoMixer.getInstance().setShareEGLContext(eglContext);
    }

    public static Object getVideoMixEGLContext() {
        return GLESVideoMixer.getInstance().sharedContext();
    }

    public static void setVideoMixSize(int width, int height) {
        GLESVideoMixer.getInstance().setVideoMixSize(width, height);
    }

    public static void setVideoNetResolutionWidth(int width, int height) {
        GLESVideoMixer.getInstance().setVideoNetResolutionWidth(width, height);
    }

    public static void setVideoNetResolutionWidthForSecond(int width, int height) {
        GLESVideoMixer.getInstance().setVideoNetResolutionWidthForSecond(width, height);
    }

    public static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height) {
        GLESVideoMixer.getInstance().addMixOverlayVideo(userId, x, y, z, width, height);
    }

    public static void removeMixOverlayVideo(String userId) {
        GLESVideoMixer.getInstance().removeMixOverlayVideo(userId);
    }

    public static void removeAllOverlayVideo() {
        GLESVideoMixer.getInstance().removeAllOverlayVideo();
    }


    public static void pushVideoFrameGLESForLocal(String userId, int type, int texture, float[] matrix, int width, int height, int rotation, int mirror, long timestamp) {
        GLESVideoMixer.getInstance().pushTextureForLocal(userId, type, texture, matrix, width, height, rotation, mirror, timestamp);
    }

    public static void pushVideoFrameRawForLocal(String userId, int type, byte[] data, int size, int width, int height, int rotation, int mirror, long timestamp) {
        GLESVideoMixer.getInstance().pushTextureForLocal(userId, type, data, size, width, height, rotation, mirror, timestamp);
    }

    public static void pushVideoFrameGLESForRemote(String userId, int type, int texture, float[] matrix, int width, int height, long timestamp) {
        GLESVideoMixer.getInstance().pushTextureForRemote(userId, type, texture, matrix, width, height, timestamp);
    }

    public static void pushVideoFrameRawForRemote(String userId, int type, byte[] data, int size, int width, int height, long timestamp) {
        GLESVideoMixer.getInstance().pushVideoDataForRemote(userId, type, data, size, width, height, timestamp);
    }

    public static void openBeautify() {
        GLESVideoMixer.getInstance().openBeautify();
    }

    public static void closeBeautify() {
        GLESVideoMixer.getInstance().closeBeautify();
    }

    public static void setBeautify(float Level) {
        GLESVideoMixer.getInstance().setBeautifyLevel(Level);
    }

    public static void onPause() {
        GLESVideoMixer.getInstance().onPause();
    }

    public static void onResume() {
        GLESVideoMixer.getInstance().onResume();
    }

    public static void mixExit() {
        GLESVideoMixer.getInstance().releaseGLES();
        GLESVideoMixer.getInstance().exit();
        GLESVideoMixer.sGLESVideoMixer = null;
    }

    public static void setVideoFps(int fps) {
        GLESVideoMixer.getInstance().setVideoFps(fps);
    }

    public static GLESVideoMixer.SurfaceContext getCameraSurfaceContext(){
        return GLESVideoMixer.getInstance().getCameraSurfaceContext();
    }

    public static GLESVideoMixer.SurfaceContext createSurfaceContext() {
        return GLESVideoMixer.getInstance().createSurfaceContext();
    }
    
    public static boolean setVideoEncoderRawCallbackEnabled(boolean enabled){
    	return GLESVideoMixer.setNetYUVCallBack(enabled);
    }
    
    public static boolean setExternalFilterEnabled(boolean enabled){
    	return GLESVideoMixer.setExternalFilterEnabled(enabled);
    }

    public static void setVideoFrameMixCallback() {

        GLESVideoMixer.getInstance().setVideoMixCallback(new IVideoMixerCallback() {

            public void videoFrameMixerCallback(int type, int texture, float[] matrix, int width, int height, long timestmap) {
                VideoMixerNative.videoFrameMixerCallback(type, texture, matrix, width, height, timestmap);
            }

            public void videoNetFirstCallback(int type, int texture, float[] matrix, int width, int height, long timestmap) {
                VideoMixerNative.videoNetFirstCallback(type, texture, matrix, width, height, timestmap);
            }

            public void videoNetSecondCallback(int type, int texture, float[] matrix, int width, int height, long timestmap) {
                VideoMixerNative.videoNetSecondCallback(type, texture, matrix, width, height, timestmap);
            }
            
            public void videoFrameMixerDataCallback(int type, byte[] data, int width, int height, long timestmap){
            	if(data != null)
            		VideoMixerNative.videoFrameMixerDataCallback(type, data, width, height, timestmap);
            }
            public void videoNetDataFirstCallback(int type, byte[] data,  int width, int height, long timestmap){
            	if(data != null)
            		VideoMixerNative.videoNetDataFirstCallback(type, data, width, height, timestmap);
            }
            public void videoNetDataSecondCallback(int type, byte[] data,  int width, int height, long timestmap){
            	if(data != null)
            		VideoMixerNative.videoNetDataSecondCallback(type, data, width, height, timestmap);
            }
            
            public int  videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror){
            	return VideoMixerNative.videoRenderFilterCallback( texture,  width, height, rotation, mirror);
            }


        });
    }

}
