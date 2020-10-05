package com.youme.mixers;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.EGLContext;
import android.opengl.GLES10Ext;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.os.Looper;
import android.util.Log;
import android.os.Build;
import android.os.Handler;
import android.os.Message;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.youme.gles.EglCore;
import com.youme.gles.GlUtil;
import com.youme.gles.WindowSurface;
import com.youme.engine.YouMeConst.YOUME_VIDEO_FMT;
import com.youme.engine.YouMeConst.YouMeVideoMirrorMode;
import com.youme.engine.video.EglBase;
import com.youme.engine.video.EglBase10;
import com.youme.engine.video.EglBase14;
import com.youme.engine.video.RendererCommon;
import com.youme.engine.video.VideoBaseCapturer;
@TargetApi(19)
@SuppressLint({ "WrongCall", "NewApi" }) public class GLESVideoMixer implements Runnable {

    private static final String TAG = "GLESVideoMixer";
    private static final String GLBEAUTYID = "GLESVideoMixerBeauty";
    private static final String GLYUV = "GLESVideoMixerYUV";
    private static final String FILTER_TEXTURE_TO_YUV = "FILTER_TEXTURE_TO_YUV";
    private static final String FILTER_TEXTUREOES_TO_YUV = "FILTER_TEXTUREOES_TO_YUV";
    private static final String FILTER_TEXTUREOES_TO_TEXTURE = "FILTER_TEXTUREOES_TO_TEXTURE";
    private static final int GLES_MIXER_TYPE_MIX = 0;
    private static final int GLES_MIXER_TYPE_NET = 2;
    private static final int GLES_MIXER_TYPE_NET_SECOND = 4;

    private static final int MSG_SETUP_GLES = 0;
    private static final int MSG_RELEASE_GLES = 1;
    private static final int MSG_ADD_MIXOVERLAY = 2;
    private static final int MSG_REMOVE_MIXOVERLAY = 3;
    private static final int MSG_REMOVE_ALL_MIXOVERLAY = 4;
    private static final int MSG_SET_MIXER_SIZE = 5;
    private static final int MSG_SET_NET_FIRST_SIZE = 6;
    private static final int MSG_SET_NET_SECOND_SIZE = 7;
    private static final int MSG_PUSH_LOCAL_TEXTURE = 8;
    private static final int MSG_PUSH_REMOTE_TEXTURE = 9;
    private static final int MSG_PUSH_REMOTE_VIDEODATA = 10;
    private static final int MSG_MIXER_DRAW = 11;
    private static final int MSG_CREATE_SURFACETEXTURE = 12;
    private static final int MSG_CLEAN_PBO = 13;
    private static final int MSG_QUIT = 15;

    private static Object sharedEGLContext;
    private static boolean mOutNetYuv = false;
    private static boolean mOutMixerYuv = false;
    private static boolean mFilterExt = false;
    
    private int mMixBackGroundWidth = 0;
    private int mMixBackGroundHeight = 0;
    private int mNetWidth = 0;
    private int mNetHeight = 0;
    private int mNetSecondWidth = 0;
    private int mNetSecondHeight = 0;
    private boolean mCurrentBeauty = false;
    private float mBeautyLevel = 0.5f;
    private Fbo mYuvFbo;
    private Fbo mBeautyFbo;
    private Fbo mMixerFbo;
    
    private Fbo mTextureFbo;
    
    private Fbo mNetFirstFbo;
    private Fbo mNetSecondFbo;
    private Fbo mMixerFbo2;

    private Pbo mMixerPbo;
    private Pbo mNetFirstPbo;
    private Pbo mNetSecondPbo;
    
    private TaskThread mTaskThreadNetFirst;
    private TaskThread mTaskThreadNetSecond;

    private boolean mUpdateTextImage = false;
    private int mTextureId;
    private SurfaceTexture mSurfaceTexture;
    private EglBase eglBase;

    private int mCameraTextureId;
    private SurfaceTexture mCameraSurfaceTexture;
    private long mLastMixTimestmap;
    private List<MixInfo> mMixInfoList;
    private Map<String, VideoFrameBuffer> mVideoBuffers;

    private final float[] mTexMatrix = new float[16];
    private String mLocalUserId = "";
    private boolean mExiting = true;
    private boolean mPause = false;

    private int mVideoFps = 20;
    private boolean mFpsNeedFilter = false;
    private int mFpsFilterIndexFirst = 0;
    private long mFpsFilterTimeFirst = 0;
    private int mFpsFilterIndex = 0;

    private  IVideoMixerCallback mVideoMixerCallback;

    private boolean mCreateSurfaceOk = false;
    private boolean mReady;
    private boolean mRunning;
    private final Object mReadyFence = new Object();
    private volatile GLESMixerHandler mHandler;
    public static GLESVideoMixer sGLESVideoMixer;
    
    private static SurfaceTexture.OnFrameAvailableListener mFrameAvailableListener = null;
    
    private boolean mAsyncCbEnabled = false;
    private boolean mMixSizeOutSet = false;
    
    private static VideoMixerFilterListener mMixerFilterListener; 
    public static GLESVideoMixer getInstance() {
        if (sGLESVideoMixer == null) {
            sGLESVideoMixer = new GLESVideoMixer();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            	sGLESVideoMixer.startMixer();
              }
        }
        return sGLESVideoMixer;
    }

    public static void setShareEGLContext(Object eglContext) {
        sharedEGLContext = eglContext;
    }
    
    public static void setFrameAvailableListener(SurfaceTexture.OnFrameAvailableListener listener){
    	mFrameAvailableListener = listener;
    }

    public static class SurfaceContext  {
        public int  textureId;
        public final SurfaceTexture surfaceTexture;
        public SurfaceContext(int textureId, SurfaceTexture surfaceTexture) {
            this.textureId = textureId;
            this.surfaceTexture = surfaceTexture;
        }
    }
    
    
    public static boolean setMixerYUVCallBack(boolean enabled){
    	if (!Pbo.isSupportPBO()) {
    		return false;
    	}
    	mOutMixerYuv = enabled;
    	Log.i(TAG,"setMixerYUVCallBack enabled:"+enabled);
    	return true;
    }
    
    public static boolean setNetYUVCallBack(boolean enabled){
    	if (!Pbo.isSupportPBO()) {
    		return false;
    	}
    	mOutNetYuv = enabled;
    	Log.i(TAG,"setNetYUVCallBack enabled:"+enabled);
    	return true;
    }

    public static void setFilterListener(VideoMixerFilterListener listener){
    	mMixerFilterListener = listener;
    	if(mMixerFilterListener != null){
    		VideoMixerNative.videoMixerUseTextureOES(true);
    		getInstance().drawFrame();
    	}
    	else{
    		VideoMixerNative.videoMixerUseTextureOES(false);
    	}
    }
    
    public static boolean  setExternalFilterEnabled(boolean enabled){
    	if (!Pbo.isSupportPBO()) {
    		return false;
    	}
    	mFilterExt = enabled;
    	if(enabled){
    		VideoMixerNative.videoMixerUseTextureOES(true);
    		//getInstance().drawFrame();
    	}
    	else{
    		VideoMixerNative.videoMixerUseTextureOES(false);
    	}
    	return false;
    }
    

    public SurfaceContext getCameraSurfaceContext(){
    	 synchronized (mReadyFence) {
             if (!mReady) {
                 return null;
             }
         }
    	 mCreateSurfaceOk = false;
         mHandler.sendMessage(mHandler.obtainMessage(MSG_CREATE_SURFACETEXTURE, 0, 0));
         synchronized (mReadyFence) {
             while (!mCreateSurfaceOk) {
                 try {
                     mReadyFence.wait();
                 } catch (InterruptedException ie) {
                     // ignore
                 }
             }
         }
         mUpdateTextImage = true;
         return new SurfaceContext(mCameraTextureId, mCameraSurfaceTexture);	
    	
    }

    public SurfaceContext createSurfaceContext(){
    	if(eglBase == null)
    		return null;
        SurfaceContext surfaceContext = null;
        try {
            eglBase.makeCurrent();
            int textureId = GlUtil.createTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
            SurfaceTexture surfaceTexture = new SurfaceTexture(textureId);
            surfaceContext = new SurfaceContext(textureId, surfaceTexture);
            eglBase.detachCurrent();
        }
        catch (Exception e){
            e.printStackTrace();
        }
        return surfaceContext;
    }

    public Object sharedContext() {
        if (sharedEGLContext != null)
            return sharedEGLContext;

        if (eglBase != null) {
            if (EglBase14.isEGL14Supported()) {
                return ((EglBase14.Context) eglBase.getEglBaseContext()).getEGLContext();
            } else {
                return ((EglBase10.Context) eglBase.getEglBaseContext()).getEGLContext();
            }
        }
        return null;
    }
    
    public EglBase.Context sharedEGLBaseContext(){
    	if(eglBase == null)
    		return null;
    	return eglBase.getEglBaseContext();
    }
    
    public boolean useTextureOES(){
    	return mUpdateTextImage;
    }
    
    public void setAsyncCbEnabled(boolean enabled){
    	mAsyncCbEnabled = enabled;
    	if(mTaskThreadNetFirst != null)
    		mTaskThreadNetFirst.setAsyncEnabled(enabled);
    	if(mTaskThreadNetSecond != null)
    		mTaskThreadNetSecond.setAsyncEnabled(enabled);
    }

    public GLESVideoMixer() {
        mMixInfoList = new ArrayList<MixInfo>();
        mVideoBuffers = new HashMap<String, VideoFrameBuffer>();
    }

    public int getTextureTarget() {
        // return GLES11Ext.GL_TEXTURE_EXTERNAL_OES;
        return GLES20.GL_TEXTURE_2D;
    }

    public void setupGLES(EGLContext context) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SETUP_GLES, 0, 0, context));
        synchronized (mReadyFence) {
            while (mExiting) {
                try {
                    mReadyFence.wait();
                } catch (InterruptedException ie) {
                    // ignore
                }
            }
        }
        Log.d(TAG, "setupGLES success!");
    }

    public void releaseGLES() {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_RELEASE_GLES, 0, 0, null));
        Log.d(TAG, "releaseGLES !");
    }

    public void setVideoFps(int fps) {
        mVideoFps = fps;
        mFpsNeedFilter = true;
        mFpsFilterTimeFirst = 0;
        Log.d(TAG, "setVideoFps fps:"+fps);
    }

    public void setVideoMixSize(int width, int height) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mMixSizeOutSet = true;
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_MIXER_SIZE, width, height, null));
        Log.d(TAG, "setMixVideoSize w:" + width + " h:" + height);
    }

    public void setVideoNetResolutionWidth(int width, int height) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_NET_FIRST_SIZE, width, height, null));
        Log.d(TAG, "setVideoNetResolutionWidth w:" + width + " h:" + height);
    }

    public void setVideoNetResolutionWidthForSecond(int width, int height) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_SET_NET_SECOND_SIZE, width, height, null));
        Log.d(TAG, "setVideoNetResolutionWidthForSecond w:" + width + " h:" + height);
    }

    public void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        Rect rc = new Rect(x, y, x + width, y + height);
        MixInfo mixinfo = new MixInfo(userId, z, rc);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_ADD_MIXOVERLAY, 0, 0, mixinfo));
        Log.d(TAG, "addMixOverlayVideo userId:" + userId + " x:" + x + " y:" + y + " z:" + z + " w:" + width + " h:" + height);
    }

    public void removeMixOverlayVideo(String userId) {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_REMOVE_MIXOVERLAY, 0, 0, userId));
        Log.d(TAG, "removeMixOverlayVideo userid:" + userId);
    }

    public void removeAllOverlayVideo() {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_REMOVE_ALL_MIXOVERLAY, 0, 0, null));
        Log.d(TAG, "removeAllOverlayVideo !");
    }

    public void drawFrame() {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.removeMessages(MSG_MIXER_DRAW);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_MIXER_DRAW, 0, 0, null));
    }

    public void exit() {
        synchronized (mReadyFence) {
            if (!mReady) {
                return;
            }
        }
        mHandler.sendMessage(mHandler.obtainMessage(MSG_QUIT, 0, 0, null));
        Log.d(TAG, "exit !");
    }

    public boolean needMixing(String userId) {
        for (MixInfo info : mMixInfoList) {
            if (info.getUserId().equals(userId)) {
                return true;
            }
        }
        return false;
    }
    

    public void pushTextureForLocal(String userId, int type, int texture, float[] texMatrix, int width, int height, int rotation, int mirror, long timestamp) {
        synchronized (mReadyFence) {
            if (mExiting || mPause)
                return;
        }
        if (!mLocalUserId.equals(userId))
            mLocalUserId = userId;
        VideoFrameBuffer videoBuffer = new VideoFrameBuffer(type, texture, texMatrix, width, height, rotation, mirror, timestamp);
        videoBuffer.userId = userId;
        mHandler.removeMessages(MSG_PUSH_LOCAL_TEXTURE);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_PUSH_LOCAL_TEXTURE, 0, 0, videoBuffer));
        drawFrame();
        //Log.d("mix", "pushTextureForLocal1 userid:" + userId + " timestamp1:" + timestamp + " texture:"+texture + " w:"+width+ " mirror:"+mirror);
    }

    public void pushTextureForLocal(String userId, int type, byte[] data, int size, int width, int height, int rotation, int mirror, long timestamp) {
        synchronized (mReadyFence) {
            if (mExiting || mPause)
                return;
        }
        if (!mLocalUserId.equals(userId))
            mLocalUserId = userId;
        VideoFrameBuffer videoBuffer = new VideoFrameBuffer(type, data, size, width, height, rotation, mirror, timestamp);
        videoBuffer.userId = userId;
        mHandler.removeMessages(MSG_PUSH_LOCAL_TEXTURE);
        mHandler.sendMessage(mHandler.obtainMessage(MSG_PUSH_LOCAL_TEXTURE, 0, 0, videoBuffer));
        drawFrame();
        //Log.d("mix", "pushTextureForLocal2 timestamp1:" + timestamp + " mirror:"+mirror);
    }

    public void pushTextureForRemote(String userId, int type, int texture, float[] texMatrix, int width, int height, long timestamp) {
        synchronized (mReadyFence) {
            if (mExiting || mPause)
                return;
        }
        VideoFrameBuffer videoBuffer = new VideoFrameBuffer(type, texture, texMatrix, width, height, timestamp);
        videoBuffer.userId = userId;
        mHandler.sendMessage(mHandler.obtainMessage(MSG_PUSH_REMOTE_TEXTURE, 0, 0, videoBuffer));
        //Log.d("mix", "pushTextureForRemote userid:" + userId + " timestamp1:" + timestamp + " texture:"+texture + " w:"+width);
        
    }

    public void pushVideoDataForRemote(String userId, int type, byte[] data, int size, int width, int height, long timestamp) {
        synchronized (mReadyFence) {
            if (mExiting || mPause)
                return;
        }

        VideoFrameBuffer videoBuffer = new VideoFrameBuffer(type, data, size, width, height, timestamp);
        videoBuffer.userId = userId;
        mHandler.sendMessage(mHandler.obtainMessage(MSG_PUSH_REMOTE_VIDEODATA, 0, 0, videoBuffer));
    }

    public void openBeautify() {
        mCurrentBeauty = true;
        Log.i(TAG,"openBeautify");
    }

    public void closeBeautify() {
        mCurrentBeauty = false;
        Log.i(TAG,"closeBeautify");
    }

    public void setBeautifyLevel(float level) {
        mBeautyLevel = level;
        Log.i(TAG,"setBeautifyLevel level:"+level);
    }

    public void onPause() {
        mPause = true;
    }

    public void onResume() {
        mPause = false;
    }


    public void startMixer() {
    	if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2 || mRunning) {
    		return;
    	}
        synchronized (mReadyFence) {
            if (mRunning) {
                Log.w(TAG, "Encoder thread already running");
                return;
            }
            mRunning = true;
            new Thread(this, "GLESVideoMixer").start();
            while (!mReady) {
                try {
                    mReadyFence.wait();
                } catch (InterruptedException ie) {
                    // ignore
                }
            }
        }
        setupGLES((EGLContext) sharedEGLContext);
        Log.d(TAG, "startMixer ---");

    }

    @Override
    public void run() {
        // Establish a Looper for this thread, and define a Handler for it.
        Looper.prepare();
        synchronized (mReadyFence) {
            mHandler = new GLESMixerHandler(this);
            mReady = true;
            mReadyFence.notify();
        }

        Looper.loop();

        Log.d(TAG, "mixer thread exiting");
        synchronized (mReadyFence) {
            mReady = mRunning = false;
            mReadyFence.notify();
            mHandler = null;
        }
    }


    public static class GLESMixerHandler extends Handler {

        private WeakReference<GLESVideoMixer> mWeakMixer;

        public GLESMixerHandler(GLESVideoMixer mixer) {
            mWeakMixer = new WeakReference<GLESVideoMixer>(mixer);
        }

        @Override  // runs on encoder thread
        public void handleMessage(Message inputMessage) {
            int what = inputMessage.what;
            Object obj = inputMessage.obj;
            int param1 = inputMessage.arg1;
            int param2 = inputMessage.arg2;
            GLESVideoMixer videoMixer = mWeakMixer.get();
            if (videoMixer == null) {
                Log.w(TAG, "GLESMixerHandler.handleMessage: mixer is null");
                return;
            }

            switch (what) {
                case MSG_SETUP_GLES:
                    videoMixer.onSetupGLES((EGLContext) obj);
                    break;
                case MSG_RELEASE_GLES:
                    videoMixer.onReleaseGLES();
                    break;
                case MSG_ADD_MIXOVERLAY:
                    videoMixer.onAddMixOverlayVideo((MixInfo) obj);
                    break;
                case MSG_REMOVE_MIXOVERLAY:
                    videoMixer.onRemoveMixOverlayVideo((String) obj);
                    break;
                case MSG_REMOVE_ALL_MIXOVERLAY:
                    videoMixer.onRemoveAllOverlayVideo();
                    break;
                case MSG_SET_MIXER_SIZE:
                    videoMixer.onSetVideoMixSize(param1, param2);
                    break;
                case MSG_SET_NET_FIRST_SIZE:
                    videoMixer.onSetVideoNetResolutionWidth(param1, param2);
                    break;
                case MSG_SET_NET_SECOND_SIZE:
                    videoMixer.onSetVideoNetResolutionWidthForSecond(param1, param2);
                    break;
                case MSG_PUSH_LOCAL_TEXTURE:
                case MSG_PUSH_REMOTE_TEXTURE:
                case MSG_PUSH_REMOTE_VIDEODATA:
                    videoMixer.onPushVideoFrame(((VideoFrameBuffer)obj).userId, (VideoFrameBuffer)obj);
                    break;
                case MSG_MIXER_DRAW:
                    videoMixer.onDrawFrame();
                    break;
                case MSG_CREATE_SURFACETEXTURE:
                	videoMixer.onCreateSurfaceTexture();
                	break;
                case MSG_CLEAN_PBO:
                	videoMixer.cleanPbo();
                	break;
                case MSG_QUIT:
                    Looper.myLooper().quit();
                    break;
                default:
                    throw new RuntimeException("Unhandled msg what=" + what);
            }
        }
    }

    public void setVideoMixCallback(IVideoMixerCallback callback) {
    	mVideoMixerCallback = callback;
    }

    public void onPushVideoFrame(String userId, VideoFrameBuffer buffer){
        mVideoBuffers.put(userId, buffer);
    }

    public void onSetVideoMixSize(int width, int height) {
        if (width < 0 || height < 0)
            return;
        mMixBackGroundWidth = width;
        mMixBackGroundHeight = height;
        Log.d(TAG, "mMixBackGroundWidth w:"+width + " h:"+height);
    }

    public void onSetVideoNetResolutionWidth(int width, int height) {
        if (width < 0 || height < 0)
            return;
        mNetWidth = width;
        mNetHeight = height;
        Log.d(TAG, "onSetVideoNetResolutionWidth w:"+width + " h:"+height);
    }

    public void onSetVideoNetResolutionWidthForSecond(int width, int height) {
        if (width < 0 || height < 0)
            return;
        mNetSecondWidth = width;
        mNetSecondHeight = height;
        Log.d(TAG, "onSetVideoNetResolutionWidthForSecond w:"+width + " h:"+height);
    }

    public void onAddMixOverlayVideo(MixInfo mixinfo) {
        for (MixInfo info : mMixInfoList) {
            if (info.getUserId().equals(mixinfo.getUserId())) {
                mMixInfoList.remove(info);
                break;
            }
        }
        if (mMixInfoList.size() == 0) {
            mMixInfoList.add(mixinfo);
        } else {
            int i = 0;
            for (; i < mMixInfoList.size(); i++) {
                if (mixinfo.getZ() < mMixInfoList.get(i).getZ()) {
                    mMixInfoList.add(i, mixinfo);
                    break;
                }
            }
            if (i == mMixInfoList.size()) {
                mMixInfoList.add(mixinfo);
            }
        }
        Log.d(TAG, "onAddMixOverlayVideo x:" + mixinfo.getRect().left + " y:"+mixinfo.getRect().top +
        " w:"+mixinfo.getRect().width() + " h:"+mixinfo.getRect().height());
    }

    public void onRemoveMixOverlayVideo(String userId) {

        mVideoBuffers.remove(userId);
        for (MixInfo info : mMixInfoList) {
            if (info.getUserId().equals(userId)) {
                mMixInfoList.remove(info);
                break;
            }
        }
    }

    public void onRemoveAllOverlayVideo() {
        if(mYuvFbo != null)
        mYuvFbo.uninitFBO();
        if(mMixerFbo != null)
        mMixerFbo.uninitFBO();
        if(mBeautyFbo != null)
        mBeautyFbo.uninitFBO();
        if (mNetFirstFbo != null) 
          mNetFirstFbo.uninitFBO();
        if (mNetSecondFbo != null)
            mNetSecondFbo.uninitFBO();
        if (mMixerFbo2 != null) 
            mMixerFbo2.uninitFBO();
        cleanPbo();
        mMixInfoList.clear();
        mVideoBuffers.clear();
        FilterManager.getInstance().releaseFilter();
        mCurrentBeauty = false;
        mMixBackGroundWidth = 0;
        mMixBackGroundHeight = 0;
        mNetWidth = 0;
        mNetHeight = 0;
        mNetSecondWidth = 0;
        mNetSecondHeight = 0;
        mUpdateTextImage = false;
        mMixSizeOutSet = false;
    }

    public void localLayerChange()
    {
    	boolean added = false;
    	VideoFrameBuffer videoBuffer = mVideoBuffers.get(mLocalUserId);
    	if(videoBuffer != null){
    		 if((mMixBackGroundWidth == 0 || mMixBackGroundHeight == 0)||
    			(!mMixSizeOutSet && (mMixBackGroundWidth != videoBuffer.getWidth() || mMixBackGroundHeight != videoBuffer.getHeight()))){
    			 mMixBackGroundWidth = videoBuffer.getWidth();
    		     mMixBackGroundHeight = videoBuffer.getHeight();
    			 addMixOverlayVideo(mLocalUserId,0,0,0,mMixBackGroundWidth,mMixBackGroundHeight);
    			 added = true;
    	      }
    	}
    	if(added || !needMixing(mLocalUserId)){
    		 Rect rc = new Rect(0, 0, mMixBackGroundWidth, mMixBackGroundHeight);
		     MixInfo mixinfo = new MixInfo(mLocalUserId, 0, rc);
			 onAddMixOverlayVideo(mixinfo);
    	}
    }

    public void onSetupGLES(EGLContext context) {
        try {
            eglBase = EglBase.create(EglBase.createContext(context), EglBase.CONFIG_PLAIN);
        } catch (Exception e) {
        	try {
            eglBase = EglBase.create(null, EglBase.CONFIG_PLAIN);
            }
            catch (Exception e1) {
               e1.printStackTrace();
               return;
            }
        }

        try {
            mTextureId = GlUtil.createTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
            mSurfaceTexture = new SurfaceTexture(mTextureId);
            mSurfaceTexture.setOnFrameAvailableListener(onFrameAvailableListener);
            eglBase.createSurface(mSurfaceTexture);
            eglBase.makeCurrent();
        } catch (Exception e) {
            eglBase.release();
            e.printStackTrace();
        }

        mExiting = false;
        mPause = false;
        //mCameraTextureId = GlUtil.createTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        //mCameraSurfaceTexture = new SurfaceTexture(mCameraTextureId);
        //sharedEGLContext = EGL14.eglGetCurrentContext();
        synchronized (mReadyFence) {
            mReadyFence.notify();
        }

        mTaskThreadNetFirst = new TaskThread();
        mTaskThreadNetFirst.setAsyncEnabled(mAsyncCbEnabled);
        mTaskThreadNetSecond = new TaskThread();
        mTaskThreadNetSecond.setAsyncEnabled(mAsyncCbEnabled);
    }

    public void onReleaseGLES() {
        if(mYuvFbo != null){
            mYuvFbo.uninitFBO();
            mYuvFbo =  null;
        }
        if (mBeautyFbo != null) {
            mBeautyFbo.uninitFBO();
            mBeautyFbo = null;
        }
        if (mMixerFbo != null) {
            mMixerFbo.uninitFBO();
            mMixerFbo = null;
        }
        if (mNetFirstFbo != null) {
            mNetFirstFbo.uninitFBO();
            mNetFirstFbo = null;
        }
        if (mNetSecondFbo != null) {
            mNetSecondFbo.uninitFBO();
            mNetSecondFbo = null;
        }
        if (mMixerFbo2 != null) {
            mMixerFbo2.uninitFBO();
            mMixerFbo2 = null;
        }
        
        if(mTextureFbo != null){
        	mTextureFbo.uninitFBO();
        	mTextureFbo = null;
        }
       
        if (eglBase != null) {
            eglBase.release();
            eglBase = null;
        }
        if (mTaskThreadNetFirst != null) {
            mTaskThreadNetFirst.release();
            mTaskThreadNetFirst = null;
        }
        if (mTaskThreadNetSecond != null) {
            mTaskThreadNetSecond.release();
            mTaskThreadNetSecond = null;
        }
        cleanPbo();
        FilterManager.getInstance().releaseFilter();
        mExiting = true;
        mUpdateTextImage = false;
    }
    
    public void cleanPbo(){

        if (mNetFirstPbo != null){
            mNetFirstPbo.unInitPBO();
            mNetFirstPbo = null;
        }
        if (mNetSecondPbo != null){
            mNetSecondPbo.unInitPBO();
            mNetSecondPbo = null;
        }
        if (mMixerPbo != null){
            mMixerPbo.unInitPBO();
            mMixerPbo = null;
         }
    }

    private void onNetFirstCallback(final int type, final int texture, final float[] matrix, final int width, final int height, final long timestmap) {
        mTaskThreadNetFirst.postToEncoderThread(new Runnable() {
             @Override
             public void run() {
             	mVideoMixerCallback.videoNetFirstCallback(type, texture, matrix, width, height, timestmap);
             }
        });
    }

    private void onNetSecondCallback(final int type, final int texture, final float[] matrix, final int width, final int height, final long timestmap) {
        mTaskThreadNetSecond.postToEncoderThread(new Runnable() {
            @Override
            public void run() {
            	mVideoMixerCallback.videoNetSecondCallback(type, texture, matrix, width, height, timestmap);
            }
        });
    }
    
    private void onNetDataFirstCallback(final int type, final byte[] data,  final int width, final int height, final long timestmap){
        mTaskThreadNetFirst.postToEncoderThread(new Runnable() {
            @Override
            public void run() {
                mVideoMixerCallback.videoNetDataFirstCallback(type, data, width, height, timestmap);
            }
        });
    }
    private void onNetDataSecondCallback(final int type, final byte[] data,  final int width, final int height, final long timestmap){
        mTaskThreadNetFirst.postToEncoderThread(new Runnable() {
            @Override
            public void run() {
            	mVideoMixerCallback.videoNetDataSecondCallback(type, data,  width, height, timestmap);
            }
        });
    }


    public void onDrawFrame() {
        synchronized (mReadyFence) {
            if (mExiting || mPause)
                return;
        }
        int mirror = 0;
        int textureNew = 0;
        float[] matrixNet = new float[16];
        boolean output = needOut();
        try{
          eglBase.makeCurrent();
          if (mUpdateTextImage) {
               mCameraSurfaceTexture.updateTexImage();
          }
        }catch(Exception e){
        	e.printStackTrace();
        	return;
        }
        if (!output)
            return;

        localLayerChange();
      //  synchronized (mReadyFence) {
            if (mVideoMixerCallback == null)
                return;
            VideoFrameBuffer videoBuffer = mVideoBuffers.get(mLocalUserId);
            if (videoBuffer == null)
                return;
            if (videoBuffer.getType() >= YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P &&
            		videoBuffer.getType() <= YOUME_VIDEO_FMT.VIDEO_FMT_YV12)
                if (uploadYuv() == 0)
                    return;

            if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES) {
                int textureid = textureOesToTexture();
                if (textureid == 0) {
                    return;
                }
            }

        if(mMixerFilterListener != null) {
            if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE) {

                    int textureid = mMixerFilterListener.onDrawTexture(videoBuffer.getTexture(), videoBuffer.getWidth(), videoBuffer.getHeight());
                    if (textureid > 0 && textureid != videoBuffer.getTexture()) {
                        videoBuffer.setTexture(textureid);
                        videoBuffer.setRotation(0);
                        videoBuffer.setMatrix(VideoFrameBuffer.samplingMatrix);
                        videoBuffer.setMirror(1);
                    } else if (textureid == -1) {
                        return;
                    }
                }
            }

            if(mFilterExt ){
            	
            	if(videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE){
            		int textureid = mVideoMixerCallback.videoRenderFilterCallback(videoBuffer.getTexture(), videoBuffer.getWidth(), videoBuffer.getHeight(), 0, 0);
            		if(textureid > 0 && textureid != videoBuffer.getTexture()){
            	    	videoBuffer.setTexture(textureid);
            	    	videoBuffer.setMatrix(VideoFrameBuffer.samplingMatrix);
            	    	videoBuffer.setMirror(0);
            		}
            	}
            	
            }

      //  }
        if (mCurrentBeauty) 
        	beautyDraw();
        
        
        if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED ||
        	videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR) {
            mirror = 1;
        }
        
        if (mNetWidth != 0 && mNetHeight != 0) {
            if (mOutNetYuv){
            	videoBuffer.getTransformMatrix(matrixNet);
            	byte[] data = textureToYUV(0, videoBuffer.getTexture(), false, matrixNet, videoBuffer.getRotation(), mirror,
            			videoBuffer.getWidth(), videoBuffer.getHeight() , mNetWidth, mNetHeight);
                onNetDataFirstCallback(YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P, data, mNetWidth, mNetHeight, videoBuffer.getTimestamp());
            }else{
                videoBuffer.getTransformMatrix(matrixNet);
                GLESMatrix.makeTexMatrix(matrixNet, videoBuffer.getWidth(), videoBuffer.getHeight(), mNetWidth, mNetHeight, videoBuffer.getRotation(), mirror);
                onNetFirstCallback(videoBuffer.getType(), videoBuffer.getTexture(), matrixNet, mNetWidth, mNetHeight, videoBuffer.getTimestamp());
            }
        }
       
        if (mNetSecondWidth != 0 && mNetSecondHeight != 0) {
            if (mOutNetYuv){
            	videoBuffer.getTransformMatrix(matrixNet);
            	byte[] data = textureToYUV(1, videoBuffer.getTexture(), false, matrixNet, videoBuffer.getRotation(), mirror,
            			videoBuffer.getWidth(), videoBuffer.getHeight() , mNetSecondWidth, mNetSecondHeight);
                onNetDataSecondCallback(YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P, data, mNetSecondWidth, mNetSecondHeight, videoBuffer.getTimestamp());
            }else{
            	videoBuffer.getTransformMatrix(matrixNet);
                GLESMatrix.makeTexMatrix(matrixNet, videoBuffer.getWidth(), videoBuffer.getHeight(), mNetSecondWidth, mNetSecondHeight, videoBuffer.getRotation(), mirror);
                onNetSecondCallback(videoBuffer.getType(), videoBuffer.getTexture(), matrixNet, mNetSecondWidth, mNetSecondHeight, videoBuffer.getTimestamp());
            }
        }
        
        if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED ||
            videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR) {
             mirror = 1;
        }else{
        	 mirror = 0;
        }
        if (mMixInfoList.size() != 1 && mVideoBuffers.size() != 1){
        	textureNew = mixDraw();
        	  if (mMixBackGroundWidth != 0 && mMixBackGroundHeight != 0) {
                  if (mOutMixerYuv){
                  	videoBuffer.getTransformMatrix(matrixNet);
                  	byte[] data = textureToYUV(2, textureNew, false, matrixNet, videoBuffer.getRotation(), 0,
                  			videoBuffer.getWidth(), videoBuffer.getHeight() , mMixBackGroundWidth, mMixBackGroundHeight);
                      mVideoMixerCallback.videoFrameMixerDataCallback (YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P, data, mMixBackGroundWidth, mMixBackGroundHeight, videoBuffer.getTimestamp());
                  }else{
                  	  videoBuffer.getTransformMatrix(matrixNet);
                      GLESMatrix.makeTexMatrix(matrixNet, videoBuffer.getWidth(), videoBuffer.getHeight(), mMixBackGroundWidth, mMixBackGroundHeight, 0, 0);
                      mVideoMixerCallback.videoFrameMixerCallback(videoBuffer.getType(), textureNew, matrixNet, mMixBackGroundWidth, mMixBackGroundHeight, videoBuffer.getTimestamp());
                  }
              }
        }
        else{
        	  if (mMixBackGroundWidth != 0 && mMixBackGroundHeight != 0) {
                  if (mOutMixerYuv){
                  	videoBuffer.getTransformMatrix(matrixNet);
                  	byte[] data = textureToYUV(2, videoBuffer.getTexture(), false, matrixNet, videoBuffer.getRotation(), mirror,
                  			videoBuffer.getWidth(), videoBuffer.getHeight() , mMixBackGroundWidth, mMixBackGroundHeight);
                  	 mVideoMixerCallback.videoFrameMixerDataCallback (YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P, data, mMixBackGroundWidth, mMixBackGroundHeight, videoBuffer.getTimestamp());
                  }else{
                  	  videoBuffer.getTransformMatrix(matrixNet);
                      GLESMatrix.makeTexMatrix(matrixNet, videoBuffer.getWidth(), videoBuffer.getHeight(), mMixBackGroundWidth, mMixBackGroundHeight, videoBuffer.getRotation(), mirror);
                      mVideoMixerCallback.videoFrameMixerCallback(videoBuffer.getType(), videoBuffer.getTexture(), matrixNet, mMixBackGroundWidth, mMixBackGroundHeight, videoBuffer.getTimestamp());
                  }
              }
        }
      
        
      // Log.d("mix", "pushTextureForLocal3 timestamp2:" + timestamp);
    }

    private int uploadYuv(){
       // synchronized (mReadyFence) {
    	   GlUtil.checkGlError("uploadYuv start");
            VideoFrameBuffer videoBuffer = mVideoBuffers.get(mLocalUserId);
            if (videoBuffer == null)
                return 0;
            int width =  videoBuffer.getWidth();
            int height = videoBuffer.getHeight();
            int fwidth = width;
            int fheight = height;
            if(videoBuffer.getRotation() % 180 != 0){
            	width = videoBuffer.getHeight();
            	height = videoBuffer.getWidth();
            }
            int mirror = 0;
            int mirrorNew = videoBuffer.getMirror();
            if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED) {
                mirror = 1;
                mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_DISABLED;
            }
            else if(videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR){
            	mirror = 1;
            	mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR;
            }
            
            IFilter filter;
            if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P)
                filter = FilterManager.getInstance().getCameraFilter(GLYUV, FilterManager.FilterType.YV21);
            else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_YV12)
                filter = FilterManager.getInstance().getCameraFilter(GLYUV, FilterManager.FilterType.YV12);
            else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_NV21)
                filter = FilterManager.getInstance().getCameraFilter(GLYUV, FilterManager.FilterType.NV21);
            else
                return 0;

            if (mYuvFbo == null)
                mYuvFbo = new Fbo();
            if (!mYuvFbo.initFBO(fwidth, fheight))
                return 0;

            GLES20.glViewport(0, 0, fwidth, fheight);
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mYuvFbo.getFrameBuffer());
            videoBuffer.getTransformMatrix(mTexMatrix);
            filter.setDrawRect(new Rect(0, 0, width, height));
            filter.setTextureSize(width, height);
            filter.setFrameBufferSize(width, height);
            //filter.makeMatrix(mTexMatrix, 0, 0);
            filter.makeMatrix(mTexMatrix, videoBuffer.getRotation(), mirror);
            if (videoBuffer.getData() != null) {
                filter.onDraw(width,
                        height,
                        videoBuffer.getData(),
                        videoBuffer.getSize(),
                        mTexMatrix);
            }
            videoBuffer.setType(YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE);
            videoBuffer.setTexture(mYuvFbo.getTexture());
            videoBuffer.setRotation(0);
            videoBuffer.setMirror(mirrorNew);
            videoBuffer.setMatrix(videoBuffer.samplingMatrix);
       // }

        GLES20.glFlush();
        //GLES20.glFinish();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        GlUtil.checkGlError("uploadYuv end");
        return mYuvFbo.getTexture();
    }
    
    private int textureOesToTexture(){
    	GlUtil.checkGlError("textureOes start");
        VideoFrameBuffer videoBuffer = mVideoBuffers.get(mLocalUserId);
        if (videoBuffer == null || videoBuffer.getType() != YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES)
            return 0;
        int width =  videoBuffer.getWidth();
        int height = videoBuffer.getHeight();
        int fwidth = width;
        int fheight = height;
//        if(videoBuffer.getRotation() % 180 != 0){
//        	width = videoBuffer.getHeight();
//        	height = videoBuffer.getWidth();
//        }

        int mirror = 0;
        int mirrorNew = videoBuffer.getMirror();
        if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED) {
            mirror = 1;
            mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_DISABLED;
        }
        else if(videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR){
        	mirror = 1;
        	mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR;
        }
        IFilter filter = FilterManager.getInstance().getCameraFilter(FILTER_TEXTUREOES_TO_TEXTURE, FilterManager.FilterType.NormalOES);
    	if(filter == null)
    		return 0;
    	  if (mTextureFbo == null)
    		  mTextureFbo = new Fbo();
          if (!mTextureFbo.initFBO(fwidth, fheight))
              return 0;
          GLES20.glViewport(0, 0, fwidth, fheight);
          GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
          GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
          GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mTextureFbo.getFrameBuffer());
          if (mUpdateTextImage && mCameraSurfaceTexture!=null) {
        	  mCameraSurfaceTexture.getTransformMatrix(mTexMatrix);
          }else{
        	  videoBuffer.getTransformMatrix(mTexMatrix);
          }
          filter.setDrawRect(new Rect(0, 0, width, height));
          filter.setTextureSize(width, height);
          filter.setFrameBufferSize(width, height);
          filter.makeMatrix(mTexMatrix, videoBuffer.getRotation(), mirror);
          filter.onDraw(videoBuffer.getTexture(), mTexMatrix);
          videoBuffer.setType(YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE);
          videoBuffer.setTexture(mTextureFbo.getTexture());
          videoBuffer.setRotation(0);
          videoBuffer.setMirror(mirrorNew);
          videoBuffer.setMatrix(videoBuffer.samplingMatrix);
      

      GLES20.glFlush();
      GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
      GlUtil.checkGlError("textureOes end");
      return mTextureFbo.getTexture();
    }

    private int beautyDraw() {

      //  synchronized (mReadyFence) {
            VideoFrameBuffer videoBuffer = mVideoBuffers.get(mLocalUserId);
            if (videoBuffer == null)
                return 0;
            int width = videoBuffer.getWidth();
            int height = videoBuffer.getHeight();
            int mirror = 0;
            int mirrorNew = videoBuffer.getMirror();
            if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED) {
                mirror = 1;
                mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_DISABLED;
            }
            else if(videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR){
            	mirror = 1;
            	mirrorNew = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR;
            }
            
            IFilter filter = null;
            if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE)
                filter = FilterManager.getInstance().getCameraFilter(GLBEAUTYID, FilterManager.FilterType.Beautify);
            else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES)
                filter = FilterManager.getInstance().getCameraFilter(GLBEAUTYID, FilterManager.FilterType.BeautifyOES);
            
            if(filter == null)
            	return 0;

            filter.setBeautyLevel(mBeautyLevel);
            if (mBeautyFbo == null)
                mBeautyFbo = new Fbo();
            if (!mBeautyFbo.initFBO(width, height))
                return 0;

            GLES20.glViewport(0, 0, width, height);
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mBeautyFbo.getFrameBuffer());
            if (mUpdateTextImage && videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES) {
                mCameraSurfaceTexture.getTransformMatrix(mTexMatrix);
            } else{
                videoBuffer.getTransformMatrix(mTexMatrix);
            }
            filter.setDrawRect(new Rect(0, 0, width, height));
            filter.setTextureSize(width, height);
            filter.setFrameBufferSize(width, height);
            filter.makeMatrix(mTexMatrix, videoBuffer.getRotation(), mirror);
            filter.onDraw(videoBuffer.getTexture(), mTexMatrix);
            videoBuffer.setType(YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE);
            videoBuffer.setTexture(mBeautyFbo.getTexture());
            videoBuffer.setMirror(mirrorNew);
            videoBuffer.setRotation(0);
            videoBuffer.setMatrix(videoBuffer.samplingMatrix);
       // }

        GLES20.glFlush();
        //GLES20.glFinish();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        return mBeautyFbo.getTexture();
    }


    @SuppressLint({ "WrongCall", "NewApi" }) public int mixDraw() {

    	int result = 0;
      //  synchronized (mReadyFence) {
            IFilter filter;
            int width = mMixBackGroundWidth;
            int height = mMixBackGroundHeight;
            if (mMixerFbo == null)
                mMixerFbo = new Fbo();
            if (width == 0 || height== 0 || !mMixerFbo.initFBO(width, height))
                return 0;
            GLES20.glViewport(0, 0, width, height);
            GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, mMixerFbo.getFrameBuffer());
            for (MixInfo info : mMixInfoList) {
                VideoFrameBuffer videoBuffer = mVideoBuffers.get(info.getUserId());
                if (videoBuffer == null)
                    continue;
                
               
                if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE)
                    filter = FilterManager.getInstance().getCameraFilter(info.getUserId(), FilterManager.FilterType.Normal);
                else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES)
                    filter = FilterManager.getInstance().getCameraFilter(info.getUserId(), FilterManager.FilterType.NormalOES);
                else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P)
                    filter = FilterManager.getInstance().getCameraFilter(info.getUserId(), FilterManager.FilterType.YV21);
                else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_YV12)
                    filter = FilterManager.getInstance().getCameraFilter(info.getUserId(), FilterManager.FilterType.YV12);
                else if (videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_NV21)
                    filter = FilterManager.getInstance().getCameraFilter(info.getUserId(), FilterManager.FilterType.NV21);
                else
                    continue;

                int mirror = 0;
                if (videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED ||
                	videoBuffer.getMirror() == YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR) {
                    mirror = 1;
                }
                filter.setDrawRect(info.getRect());
                filter.setTextureSize(videoBuffer.getWidth(), videoBuffer.getHeight());
                filter.setFrameBufferSize(width, height);
                if (info.getUserId() == mLocalUserId && mUpdateTextImage && videoBuffer.getType() == YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES) {
                    mCameraSurfaceTexture.getTransformMatrix(mTexMatrix);
                } else{
                    videoBuffer.getTransformMatrix(mTexMatrix);
                }
                filter.makeMatrix(mTexMatrix, videoBuffer.getRotation(), mirror);
                if (videoBuffer.getType() >= YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P && 
                		videoBuffer.getType() <= YOUME_VIDEO_FMT.VIDEO_FMT_YV12) {
                		result = filter.onDraw(videoBuffer.getWidth(),
                                 videoBuffer.getHeight(),
                                 videoBuffer.getData(),
                                 videoBuffer.getSize(),
                                 mTexMatrix);
                     } 
                else {
                		result = filter.onDraw(videoBuffer.getTexture(), mTexMatrix);
                     }
                
                if(result != 0 ){
                	mVideoBuffers.remove(info.getUserId());
                }
            }
      //  }
        GLES20.glFlush();
        //GLES20.glFinish();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        GlUtil.checkGlError("mixDraw end");
        return mMixerFbo.getTexture();
    }
    

    private byte[] textureToYUV(int type, int textureId, boolean isOes, float[] matrix, int rotation, int mirror,
                                    int texWidth, int texHeight, int outWidht, int outHeight){

        Fbo fbo = null;
        Pbo pbo = null;
        IFilter filter = null;
        if (type == 0){
            if(mNetFirstFbo == null)
                mNetFirstFbo = new Fbo();
            if(mNetFirstPbo == null)
                mNetFirstPbo = new Pbo();
            fbo = mNetFirstFbo;
            pbo = mNetFirstPbo;
        }
        else if (type == 1){
            if(mNetSecondFbo == null)
                mNetSecondFbo = new Fbo();
            if(mNetSecondPbo == null)
                mNetSecondPbo = new Pbo();
            fbo = mNetSecondFbo;
            pbo = mNetSecondPbo;
        }
        else if (type == 2){
            if(mMixerFbo2 == null)
                mMixerFbo2 = new Fbo();
            if(mMixerPbo == null)
                mMixerPbo = new Pbo();
            fbo = mMixerFbo2;
            pbo = mMixerPbo;
        }
       if (isOes)
            filter = FilterManager.getInstance().getCameraFilter(FILTER_TEXTUREOES_TO_YUV, FilterManager.FilterType.TEXTURETOYUVOES);
       else
            filter = FilterManager.getInstance().getCameraFilter(FILTER_TEXTURE_TO_YUV, FilterManager.FilterType.TEXTURETOYUV);

        if(fbo == null || pbo == null || filter == null)
            return null;

        
        int stride = (outWidht+127)/128*128; //128字节对齐
        int width = outWidht/4;
        int height = outHeight*3/2;
        if (!fbo.initFBO(stride, height))
            return null;
        
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, fbo.getFrameBuffer());
        Rect rc = new Rect(0,0, outWidht, outHeight);
        filter.setDrawRect(rc);
        filter.setTextureSize(texWidth, texHeight);
        filter.setFrameBufferSize(outWidht, outHeight);
        filter.makeMatrix(matrix, 180, mirror==1?0:1);
        //filter.makeMatrix(matrix, 0, 0);
        filter.onDraw(textureId, matrix);
        try{
            byte[] buffer = pbo.bindPixelBuffer(width, height);
            if(buffer == null){
                Log.d(TAG, "mixer texture to yuv fail!!");
            }
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            GlUtil.checkGlError("textureToYUV end");
            return buffer;
        }catch (Exception e){
            e.printStackTrace();
            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            return  null;
        }

    }


    private boolean needOut() {

        final int offsetTime = 5;
        boolean ret = false;
        if (!mFpsNeedFilter)
            return true;
        long timeNow = System.currentTimeMillis();
        if (mFpsFilterTimeFirst == 0) {
            mFpsFilterTimeFirst = timeNow;
            mFpsFilterIndexFirst = 0;
        }
        if (timeNow - mFpsFilterTimeFirst + offsetTime >= 1000.f / mVideoFps * mFpsFilterIndexFirst) {
            ret = true;
            mFpsFilterIndexFirst++;
            mFpsFilterIndex++;
        }
        if (timeNow - mFpsFilterTimeFirst + offsetTime >= 1000) {
            if (mFpsFilterIndex <= mVideoFps) {
                mFpsNeedFilter = false;
                Log.d(TAG, "vide mixer fps  --count" + mFpsFilterIndex + " fps:" + mVideoFps);
            }
            mFpsFilterTimeFirst = timeNow;
            mFpsFilterIndexFirst = 0;
            mFpsFilterIndex = 0;
        }
        return ret;
    }
    
    private void onCreateSurfaceTexture(){
    	if(mCameraSurfaceTexture != null){
    		GLES20.glDeleteTextures(1, new int[] {mCameraTextureId}, 0);
    		mCameraSurfaceTexture.release();
    	}
    	mCameraTextureId = GlUtil.createTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
        mCameraSurfaceTexture = new SurfaceTexture(mCameraTextureId);
        if(mFrameAvailableListener != null)
        	mCameraSurfaceTexture.setOnFrameAvailableListener(mFrameAvailableListener); 
        synchronized (mReadyFence) {
        	mCreateSurfaceOk = true;
            mReadyFence.notify();
        }
        Log.d(TAG, "onCreateSurfaceTexture success!");
    }


    private SurfaceTexture.OnFrameAvailableListener onFrameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {

        @Override
        public void onFrameAvailable(SurfaceTexture surfaceTexture) {

//            eglBase.makeCurrent();
//            if (mIVideoMixerCallback != null) {
//                mSurfaceTexture.updateTexImage();
//                mSurfaceTexture.getTransformMatrix(mTexMatrix);
//               // mIVideoMixerCallback.videoFrameMixerCallback(1, mTextureId, mTexMatrix, mMixBackGroundWidth, mMixBackGroundHeight, mLastMixTimestmap);
//            }
        }
    };

    static int scount = 0;

    private void SaveRawPixels(int width, int height) {
        if (scount++ != 20)
            return;
        int size = width * height;
        ByteBuffer buf = ByteBuffer.allocateDirect(size * 4);
        buf.order(ByteOrder.nativeOrder());
        GLES20.glReadPixels(0, 0, width, height, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, buf);
        int data[] = new int[size];
        buf.asIntBuffer().get(data);
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        bitmap.setPixels(data, 0, width, 0, 0, width, height);

        File f = new File("/sdcard/download/", "test.jpg");
        if (f.exists()) {
            f.delete();
        }
        try {
            FileOutputStream out = new FileOutputStream(f);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 90, out);
            out.flush();
            out.close();
            Log.i(TAG, "已经保存");
        } catch (FileNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            throw new RuntimeException("");
        } catch (IOException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }
}
