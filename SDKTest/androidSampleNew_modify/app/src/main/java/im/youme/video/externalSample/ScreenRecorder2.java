package im.youme.video.externalSample;

import android.Manifest;
import android.app.Activity;
import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.graphics.ImageFormat;
import android.graphics.PixelFormat;
import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.Image;
import android.media.ImageReader;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Environment;
import android.support.annotation.RequiresApi;
import android.support.v4.content.ContextCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Surface;

import com.youme.voiceengine.YouMeConst;
import com.youme.voiceengine.api;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.text.SimpleDateFormat;
import java.util.Date;

public class ScreenRecorder2 {
    private static final String TAG = "ScreenRecorder";

    public static int SCREEN_RECODER_REQUEST_CODE = 93847;

    private static WeakReference<Context>       mContext = null;
    private static MediaProjectionManager       mMediaProjectionManager = null;
    private static MediaProjection              mMediaProjection = null;
    private static VirtualDisplay               mVirtualDisplay;
    private static MediaProjectionCallback      mMediaProjectionCallback;

    private static final boolean DEBUG = false;
    private static FileOutputStream mFos = null;
    private static int           mCounter = 1;

    private static ImageReader.OnImageAvailableListener imageListener = null;
    private static ImageReader      mImageReader = null;

    private static int              mWidth = 1080;
    private static int              mHeight = 1920;
    private static int              mFps = 30;
    private static int              mTimeInterval = 33;     // 默认帧率30，帧间隔为33ms
    private static long             lastCaptureTime = 0;    // 上一帧采集时间，用于抽帧处理

    private static Thread           mRecorderThread;
    private static boolean          mIsRecorderStarted = false;
    private static boolean          mIsLoopExit = false;
    private static String           mScreenRecorderName;

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
    }

    public static void setResolution(int width, int height) {
        mWidth = width;
        mHeight = height;
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

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    private static class MediaProjectionCallback extends MediaProjection.Callback{

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

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public static boolean onActivityResult(int requestCode, int resultCode, Intent data) {
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
                        mImageReader = ImageReader.newInstance(mWidth, mHeight, PixelFormat.RGBA_8888, 1);

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
                        Log.d(TAG, "Start ScreenRecorder success !!");
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

    @RequiresApi(api = Build.VERSION_CODES.KITKAT)
    private static class ImageListener implements ImageReader.OnImageAvailableListener {
        @Override
        public void onImageAvailable(ImageReader reader) {
            Log.d("fish","in");
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
            if ((currTime - lastCaptureTime) >= (1000/mFps)) {
                Log.d("fish","in2");
                Image image = reader.acquireLatestImage();
                //reader.getSurface();
                Log.d("fish","in3");

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
//                    int srclength = bytes_src.length;
//                    int dstlength = bytes_dst.length;
                    int srcIndex = 0;
                    int dstIndex = 0;
                    for (int i = 0; i < imageHeight; i++) {
                        System.arraycopy(bytes_src, srcIndex, bytes_dst, dstIndex, imageWidth * 4);
                        srcIndex += rowStride;
                        dstIndex += imageWidth * 4;
                    }
                    Log.d("Screen","w:"+imageWidth+" h:"+imageHeight+"lenght:"+bytes_dst.length);
                    Log.d("fish","in4");
                    api.inputVideoFrameForShare(bytes_dst, bytes_dst.length, imageWidth, imageHeight, YouMeConst.YOUME_VIDEO_FMT.VIDEO_FMT_ABGR32, 0, YouMeConst.YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_AUTO, currTime);
                    Log.d("fish","in5");
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