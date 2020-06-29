package im.youme.video.externalSample;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;

import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.support.annotation.RequiresApi;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;

import com.youme.mixers.GLESVideoMixer;
import com.youme.voiceengine.YouMeConst;
import com.youme.voiceengine.api;
import com.youme.voiceengine.video.EglBase;
import com.youme.voiceengine.video.EglBase14;
import com.youme.voiceengine.video.SurfaceTextureHelper;
import java.lang.ref.WeakReference;

public class ScreenRecorder3 {
    private static final String TAG = "ScreenRecorder";

    public static int SCREEN_RECODER_REQUEST_CODE = 93849;

    private static WeakReference<Context>       mContext = null;
    private static MediaProjectionManager       mMediaProjectionManager = null;
    private static MediaProjection              mMediaProjection = null;
    private static VirtualDisplay               mVirtualDisplay;
    private static MediaProjectionCallback      mMediaProjectionCallback;

    private static final boolean DEBUG = false;

    private static int              mWidth = 1080;
    private static int              mHeight = 1920;
    private static int              mFps = 30;
    private static int              mTimeInterval = 33;     // 默认帧率30，帧间隔为33ms
    private static long             lastCaptureTime = 0;    // 上一帧采集时间，用于抽帧处理

    private static boolean          mIsRecorderStarted = false;
    private static EglBase eglBase;
    private static SurfaceTextureHelper surfaceTextureHelper;
    public static boolean useStandaloneGLESContext = false;

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static void init(Context env) {

        if (env == null) {
            Log.e(TAG,
                    "context can not be null");
            return;
        }
        if (mContext != null) {
            if (env instanceof Activity) {
                mContext.clear();
                mContext = new WeakReference<Context>(env);
            }
            return;
        }
        mContext = new WeakReference<Context>(env);
        mMediaProjectionCallback = new MediaProjectionCallback();
        if(useStandaloneGLESContext) {
            surfaceTextureHelper = SurfaceTextureHelper.create("YoumeCaptureThread", null);
            api.setScreenSharedEGLContext(((EglBase14.Context)(surfaceTextureHelper.sharedEGLBaseContext())).getEGLContext());
        }

    }

    public static void setResolution(int width, int height) {
        mWidth = width;
        mHeight = height;
        Log.d(TAG, "setResolution width:"+ width+" height:"+height);
    }

    public static void setFps(int fps) {
        mFps = fps;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static boolean startScreenRecorder() {
        Log.i(TAG, "START Screen Recorder!!");
        if (mIsRecorderStarted) {
            Log.e(TAG, "Recorder already started !");
            return false;
        }
        boolean isApiLevel21 = false;
        try {
            if ((Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) && (mContext != null && mContext.get()!=null)
                    && (mContext.get() instanceof Activity) && (mContext.get().getApplicationInfo().targetSdkVersion >= 21)) {
                isApiLevel21 = true;
                mTimeInterval = 1000/mFps;
                mMediaProjectionManager = (MediaProjectionManager) mContext.get().getSystemService(Context.MEDIA_PROJECTION_SERVICE);
                Intent permissionIntent = mMediaProjectionManager.createScreenCaptureIntent();
                ((Activity) mContext.get()).startActivityForResult(permissionIntent, SCREEN_RECODER_REQUEST_CODE);

            }else {
                Log.e(TAG, "Exception for startScreenRecorder");
            }
        } catch (Throwable e) {
            Log.e(TAG, "Exception for startScreenRecorder");
            e.printStackTrace();
        }
        return isApiLevel21;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static boolean stopScreenRecorder() {
        if(!mIsRecorderStarted){
            Log.i(TAG, "mIsRecorderStarted:Screen Recorder not started!!");
        }
        Log.i(TAG, "STOP Screen Recorder!!");
        if(surfaceTextureHelper != null)
        {
            surfaceTextureHelper.stopListening();
            if(!useStandaloneGLESContext) {
                surfaceTextureHelper.dispose();
                surfaceTextureHelper = null;
            }
        }

        if (mVirtualDisplay != null) {
            mVirtualDisplay.release();
        }
        if(mMediaProjection != null) {
            mMediaProjection.stop();
            mMediaProjection.unregisterCallback(mMediaProjectionCallback);
            mMediaProjection = null;
        }
        mIsRecorderStarted = false;
        return true;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static void orientationChange(int rotation,int width,int height){
        if(mWidth != width || mHeight != height){
            mHeight = height;
            mWidth = width;
            if(mIsRecorderStarted) {
                recreateVirtualDisplay();
            }
        }
    }

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    private static boolean recreateVirtualDisplay(){
        if(mVirtualDisplay != null && surfaceTextureHelper!=null) {
            mVirtualDisplay.release();
            surfaceTextureHelper.getSurfaceTexture().setDefaultBufferSize(mWidth, mHeight);
            Surface surface = new Surface(surfaceTextureHelper.getSurfaceTexture());
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                mVirtualDisplay = mMediaProjection
                        .createVirtualDisplay("youme_screen", mWidth,
                                mHeight, 240,
                                DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC | DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION,
                                surface,
                                null /* Callbacks */, null /* Handler */);
            }
            return true;
        }
        return false;
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private static class MediaProjectionCallback extends MediaProjection.Callback{

        @Override
        public void onStop() {
            Log.d(TAG,"MediaProjectionCallback onStop()");
            super.onStop();

            if(surfaceTextureHelper != null && !useStandaloneGLESContext)
            {
                surfaceTextureHelper.stopListening();
                surfaceTextureHelper.dispose();
                surfaceTextureHelper = null;
            }

            if (mVirtualDisplay != null) {
                mVirtualDisplay.release();
            }
            if(mMediaProjection != null) {
                mMediaProjection.stop();
                mMediaProjection.unregisterCallback(mMediaProjectionCallback);
//                mMediaProjection = null;
            }
        }
    }

    private static long frameCount = 0;
    private static long lastFpsCheckTime = 0;
    private static SurfaceTextureHelper.OnTextureFrameAvailableListener onFrameAvailableListener = new SurfaceTextureHelper.OnTextureFrameAvailableListener() {

        @Override
        public void onTextureFrameAvailable(final int oesTextureId, final float[] transformMatrix, long l) {
            frameCount ++;

            final long currTime = System.currentTimeMillis();
            if(currTime - lastFpsCheckTime > 1000){
                Log.d(TAG,"fps:" + frameCount);
                frameCount = 0;
                lastFpsCheckTime = currTime;
            }

            surfaceTextureHelper.getHandler().post(new Runnable() {
                @Override
                public void run() {
                    api.inputVideoFrameGLESForshare(oesTextureId, transformMatrix, mWidth, mHeight, YouMeConst.YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES, 0, 0, currTime);
                }
            });

            lastCaptureTime = currTime;
            surfaceTextureHelper.returnTextureFrame();
        }

    };

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SCREEN_RECODER_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                if ((Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) && (mContext != null && mContext.get()!=null)
                        && (mContext.get() instanceof Activity) && (mContext.get().getApplicationInfo().targetSdkVersion >= 21)) {
                    mMediaProjection = mMediaProjectionManager.getMediaProjection(resultCode, data);
                    if (mMediaProjection != null) {
                        Log.i(TAG, "Ready to screen recode!!");
                        if(!useStandaloneGLESContext) {
                            if (surfaceTextureHelper != null) {
                                surfaceTextureHelper.stopListening();
                                surfaceTextureHelper.dispose();
                                surfaceTextureHelper = null;
                            }
                            surfaceTextureHelper = SurfaceTextureHelper.create("YoumeCaptureThread", GLESVideoMixer.getInstance().sharedEGLBaseContext());
                        }

                        mMediaProjection.registerCallback(mMediaProjectionCallback , surfaceTextureHelper.getHandler());

                        //DisplayMetrics displayMetrics  = new DisplayMetrics();
                        //((Activity)mContext.get()).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
                        //final int screenDensity = displayMetrics.densityDpi;
                        //Log.d(TAG,"screenDensity dpi:"+screenDensity);
                        surfaceTextureHelper.getHandler().post(new Runnable() {
                            @Override
                            public void run() {
                                surfaceTextureHelper.getSurfaceTexture().setDefaultBufferSize(mWidth, mHeight);
                                Surface surface = new Surface(surfaceTextureHelper.getSurfaceTexture());

                                mVirtualDisplay = mMediaProjection
                                        .createVirtualDisplay("youme_screen", mWidth,
                                                mHeight, 240,
                                                DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC | DisplayManager.VIRTUAL_DISPLAY_FLAG_PRESENTATION,
                                                surface,
                                                null /* Callbacks */, null /* Handler */);

                                surfaceTextureHelper.startListening(onFrameAvailableListener);
                                surfaceTextureHelper.returnTextureFrame();

                                mIsRecorderStarted = true;
                                Log.d(TAG, "Start ScreenRecorder success !!");
                            }
                        });

                    }else {
                        Log.e(TAG, "Start ScreenRecorder failed 1 !!");
                    }
                }else {
                    Log.e(TAG, "Start ScreenRecorder failed 2 !!");
                }
            }else {
                Log.w(TAG, "Start ScreenRecorder failed, user cancel !!");
            }
            return true;
        }else {
            return false;
        } 
    }

}