package com.youme.voiceengine;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public class ScreenRecorderARGB implements ScreenRecorderInterface {
    private static final String TAG = "ScreenRecorderARGB";
    public static int SCREEN_RECODER_REQUEST_CODE = 33;
    private  WeakReference<Context>       mContext = null;
    private  MediaProjectionManager       mMediaProjectionManager = null;
    private  MediaProjection              mMediaProjection = null;
    private  VirtualDisplay               mVirtualDisplay;
    private  MediaProjectionCallback      mMediaProjectionCallback;

    private  final boolean DEBUG = false;
    private  FileOutputStream mFos = null;
    private  int           mCounter = 1;

    private  ImageReader.OnImageAvailableListener imageListener = null;
    private  ImageReader      mImageReader = null;

    private  int              mWidth = 1080;
    private  int              mHeight = 1920;
    private  int              mFps = 30;
    private  int              mTimeInterval = 33;     // 默认帧率30，帧间隔为33ms
    private  long             lastCaptureTime = 0;    // 上一帧采集时间，用于抽帧处理

    private  Thread           mRecorderThread;
    private  boolean          mIsRecorderStarted = false;
    private  boolean          mIsLoopExit = false;
    private  String           mScreenRecorderName;

    public  void init(Context env) {
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
    }

    public void setResolution(int width, int height) {
        mWidth = width;
        mHeight = height;
    }

    public void setFps(int fps) {
        mFps = fps;
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public boolean startScreenRecorder() {
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

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public boolean stopScreenRecorder() {
        Log.i(TAG, "STOP Screen Recorder!!");
        if(mImageReader != null) {
            mImageReader.close();
            mImageReader = null;
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

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private class MediaProjectionCallback extends MediaProjection.Callback{

        @TargetApi(Build.VERSION_CODES.LOLLIPOP)
        @Override
        public void onStop() {
            Log.d(TAG,"MediaProjectionCallback onStop()");
            super.onStop();

            if(mImageReader != null) {
                mImageReader.close();
                mImageReader = null;
            }
            if (mVirtualDisplay != null) {
                mVirtualDisplay.release();
            }
            if(mMediaProjection != null) {
                mMediaProjection.stop();
                mMediaProjection.unregisterCallback(mMediaProjectionCallback);
                mMediaProjection = null;
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    public boolean onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == SCREEN_RECODER_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                if ((Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) && (mContext != null && mContext.get()!=null)
                        && (mContext.get() instanceof Activity) && (mContext.get().getApplicationInfo().targetSdkVersion >= 21)) {
                    mMediaProjection = mMediaProjectionManager.getMediaProjection(resultCode, data);
                    if (mMediaProjection != null) {
                        Log.i(TAG, "Ready to screen recode!!");

                        if(mImageReader != null) {
                            mImageReader.close();
                            mImageReader = null;
                        }
                        mImageReader = ImageReader.newInstance(mWidth, mHeight, PixelFormat.RGBA_8888, 2);

                        mMediaProjection.registerCallback(mMediaProjectionCallback , null);
                        DisplayMetrics displayMetrics  = new DisplayMetrics();
                        ((Activity)mContext.get()).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
                        int screenDensity = displayMetrics.densityDpi;
                        mVirtualDisplay = mMediaProjection
                                .createVirtualDisplay("screen", mWidth,
                                        mHeight, screenDensity,
                                        DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                                        mImageReader.getSurface(),
                                        null /* Callbacks */, null /* Handler */);

                        mIsLoopExit = false;

                        imageListener = new ImageListener();
                        mImageReader.setOnImageAvailableListener(imageListener, null);

                        mIsRecorderStarted = true;
                        Log.d(TAG, "Start ScreenRecorderInterface success !!");
                    }else {
                        Log.e(TAG, "Start ScreenRecorderInterface failed 1 !!");
                    }
                }else {
                    Log.e(TAG, "Start ScreenRecorderInterface failed 2 !!");
                }
            }else {
                Log.w(TAG, "Start ScreenRecorderInterface failed, user cancel !!");
            }
            return true;
        }else {
            return false;
        } 
    }

    @Override
    public void orientationChange(int rotation, int width, int height) {
        if(mWidth != width || mHeight != height){
            mHeight = height;
            mWidth = width;
            if(mIsRecorderStarted) {
                recreateVirtualDisplay();
            }
        }
    }

    @TargetApi(Build.VERSION_CODES.LOLLIPOP)
    private boolean recreateVirtualDisplay(){
        if(mVirtualDisplay != null && mImageReader!=null) {
            mVirtualDisplay.release();
            if(mImageReader != null) {
                mImageReader.close();
                mImageReader = null;
            }
            mImageReader = ImageReader.newInstance(mWidth, mHeight, PixelFormat.RGBA_8888, 2);

            DisplayMetrics displayMetrics  = new DisplayMetrics();
            ((Activity)mContext.get()).getWindowManager().getDefaultDisplay().getMetrics(displayMetrics);
            int screenDensity = displayMetrics.densityDpi;
            mVirtualDisplay = mMediaProjection
                    .createVirtualDisplay("screen", mWidth,
                            mHeight, screenDensity,
                            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
                            mImageReader.getSurface(),
                            null /* Callbacks */, null /* Handler */);
            imageListener = new ImageListener();
            mImageReader.setOnImageAvailableListener(imageListener, null);
            return true;
        }
        return false;
    }

    private long frameCount = 0;
    private long lastFpsCheckTime = 0;

    @TargetApi(Build.VERSION_CODES.KITKAT)
    private class ImageListener implements ImageReader.OnImageAvailableListener {
        @Override
        public void onImageAvailable(ImageReader reader) {
//            Process.setThreadPriority(Process.THREAD_PRIORITY_URGENT_AUDIO);
            if (DEBUG) {
                if (mFos == null) {
                    mScreenRecorderName = String.format(((Activity)mContext.get()).getExternalFilesDir("").toString() + "test_%d.yuv", mCounter++);
                    File file = new File(mScreenRecorderName);
                    try {
                        if (file.exists()) {
                            file.delete();
                        }
                        mFos = new FileOutputStream(file); //建立一个可以存取字节的文件
                    } catch (Exception e) {
                        e.printStackTrace();
                    }
                }
            }

            long currTime = System.currentTimeMillis();
            if ((currTime - lastCaptureTime) > (1000.0f/mFps)) {
                frameCount ++;

                if(currTime - lastFpsCheckTime > 1000){
                    Log.d(TAG,"fps:" + frameCount +" mFps:"+mFps);
                    lastFpsCheckTime = currTime;
                    frameCount = 0;
                }

                Image image = reader.acquireLatestImage();

                if (image != null) {
                    int imageWidth = image.getWidth();
                    int imageHeight = image.getHeight();
                    Image.Plane plane = image.getPlanes()[0];
                    //int planeLength = image.getPlanes().length;
                    //int pixelStride = plane.getPixelStride();
                    int rowStride = plane.getRowStride();

                    ByteBuffer buffer = plane.getBuffer();
                    byte[] bytes_src = new byte[buffer.capacity()];
                    buffer.get(bytes_src);
                    byte[] bytes_dst = new byte[imageWidth * imageHeight * 4];
                    //int srclength = bytes_src.length;
                    //int dstlength = bytes_dst.length;
                    int srcIndex = 0;
                    int dstIndex = 0;
                    for (int i = 0; i < imageHeight; i++) {
                        System.arraycopy(bytes_src, srcIndex, bytes_dst, dstIndex, imageWidth * 4);
                        srcIndex += rowStride;
                        dstIndex += imageWidth * 4;
                    }
                    api.inputVideoFrameForShare(bytes_dst, bytes_dst.length, imageWidth, imageHeight, YouMeConst.YOUME_VIDEO_FMT.VIDEO_FMT_ABGR32, 0, YouMeConst.YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_AUTO, currTime);
                    if (DEBUG) {
                        if (mFos != null) {
                            try {
                                mFos.write(bytes_dst);
                            } catch (IOException e) {
                                e.printStackTrace();
                            }
                        }
                    }
                    lastCaptureTime = currTime;
                    image.close();
                }
            }else {
                Image image = reader.acquireLatestImage();
                if (image != null)
                    image.close();
            }
        }
    }
}