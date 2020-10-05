package com.youme.engine;

import java.lang.ref.WeakReference;
import java.util.List;

import java.util.ArrayList;

import com.youme.mixers.GLESVideoMixer;
import com.youme.mixers.VideoMixerHelper;
import com.youme.engine.YouMeConst.YOUME_VIDEO_FMT;
import com.youme.engine.YouMeConst.YouMeVideoMirrorMode;
import com.youme.engine.video.GlUtil;

import android.annotation.SuppressLint;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.CameraInfo;
import android.opengl.GLES11Ext;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.TextureView;
import android.content.Context;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.content.pm.PackageManager;
import android.app.Activity;
import android.Manifest;
import android.view.Surface;
import android.hardware.Camera.Face;
import android.hardware.Camera.FaceDetectionListener;
import android.view.WindowManager;

@SuppressLint("NewApi")
@SuppressWarnings("deprecation")
public class CameraMgr implements Camera.AutoFocusCallback, Camera.ErrorCallback, CommonHandlerListener {
    final static String tag = "YOUME_CameraMgr";

    private final static int DEFAULE_WIDTH = 640;
    private final static int DEFAULE_HEIGHT = 480;
    private final static int DEFAULE_FPS = 15;
    private static int requestPermissionCount = 0;
    private static boolean isStopedByExternalNotify = false;

    private SurfaceView svCamera = null;
    private int mTextureId = 0;
    private SurfaceTexture mSurfaceTexture = null;
    private Camera camera = null;
    Camera.Parameters camPara = null;
    private String camWhiteBalance;
    private String camFocusMode;

    private byte mBuffer[];
    private static boolean isFrontCamera = true;
    private static int orientation = 0;
    private static int orientation2 = 0;
    private static int screenOrientation = 0;
    private static CameraMgr instance = new CameraMgr();
    private static WeakReference<Context> context = null;

    private boolean needLoseFrame = true;
    private boolean droped = false;
    private int frameCount = 0;
    private long totalTime = 0;
    private int mFps = 15;
    private int mFmt = 0;
    private int mWidth = 640;
    private int mHeight = 480;
    private static int mMirrorMode = 0;

    private boolean mAutoFocusLocked = false;
    private boolean mIsSupportAutoFocus = false;
    private boolean mIsSupportAutoFocusContinuousPicture = false;
    public boolean mAudtoFocusFace = false;
    private CameraControllerHandler mHandler;
    private static Camera.AutoFocusCallback mFocusCallback;

    private CameraMgr() {
        mHandler = new CameraControllerHandler(this);
    }

    public static CameraMgr getInstance() {
        return instance;
    }

    public CameraMgr(SurfaceView svCamera) {
        Log.d(tag, "CameraMgr with svCamera");
        this.svCamera = svCamera;
        mHandler = new CameraControllerHandler(this);
    }

    public static void init(Context ctx) {
        if (ctx == null) {
            Log.e(tag, "context can not be null");
            return;
        }
        context = new WeakReference<Context>(ctx);
        Log.d(tag, "camera init Context:" + ctx);
        if (ctx instanceof Activity) {
            setRotation(((Activity) ctx).getWindowManager().getDefaultDisplay().getRotation());
        } else {
            Log.w(tag, "camera init Context is not activity");
            screenOrientation = 0;
        }
    }

    public static void setLocalVideoMirrorMode(int mirror) {
        mMirrorMode = mirror;
    }

    public static void setRotation(int rotation) {
        switch (rotation) {
            case Surface.ROTATION_0:
                screenOrientation = 0;
                break;
            case Surface.ROTATION_90:
                screenOrientation = 90;
                break;
            case Surface.ROTATION_180:
                screenOrientation = 180;
                break;
            case Surface.ROTATION_270:
                screenOrientation = 270;
                break;
        }
    }

    public static void rotationChange() {
        WindowManager wm = (WindowManager) context.get().getSystemService(Context.WINDOW_SERVICE);
        int angle = wm.getDefaultDisplay().getRotation();
        int width = wm.getDefaultDisplay().getWidth();
        int height = wm.getDefaultDisplay().getHeight();

        int rotation = 0;
        switch (angle) {
            case Surface.ROTATION_0:
                rotation = 0;
                screenOrientation = 0;
                Log.d(tag, "Rotation_0");
                break;
            case Surface.ROTATION_90:
                rotation = 1;
                screenOrientation = 90;
                Log.d(tag, "ROTATION_90");
                break;
            case Surface.ROTATION_180:
                rotation = 2;
                screenOrientation = 180;
                Log.d(tag, "ROTATION_180");
                break;
            case Surface.ROTATION_270:
                rotation = 3;
                screenOrientation = 270;
                Log.d(tag, "ROTATION_270");
                break;
            default:
                Log.d(tag, "Default Rotation!");
                break;
        }

        int cameraNum = Camera.getNumberOfCameras();
        CameraInfo cameraInfo = new CameraInfo();

        for (int i = 0; i < cameraNum; i++) {
            Camera.getCameraInfo(i, cameraInfo);
            if ((isFrontCamera) && (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT)) {
                orientation2 = (360 - cameraInfo.orientation + 360 - screenOrientation) % 360;
                orientation = (360 - orientation2) % 360;  // compensate the mirror
                Log.d(tag, "i:" + i + " cameraInfo.orientation:" + cameraInfo.orientation + " screenOrientation:" + screenOrientation);
                break;
            } else if ((!isFrontCamera) && (cameraInfo.facing == CameraInfo.CAMERA_FACING_BACK)) {
                orientation = (cameraInfo.orientation + 360 - screenOrientation) % 360;
                orientation2 = orientation;//(360 - orientation) % 360;
                Log.d(tag, "i:" + i + " cameraInfo.orientation:" + cameraInfo.orientation + " screenOrientation:" + screenOrientation);
                break;
            }
        }

        if (rotation != 0 && rotation != 2) {
            if (width > height) {
                NativeEngine.switchResolutionForLandscape();
            } else {
                NativeEngine.resetResolutionForPortrait();
            }
        } else {
            if (width > height) {
                NativeEngine.switchResolutionForLandscape();
            } else {
                NativeEngine.resetResolutionForPortrait();
            }
        }

    }

    static boolean isLandscape(int orientation) {
        return orientation == 90 || orientation == 270;
    }

    public int openCamera(boolean isFront) {
        if (null != camera) {
            closeCamera();
        }

        int cameraId = 0;
        int cameraNum = Camera.getNumberOfCameras();
        CameraInfo cameraInfo = new CameraInfo();

        for (int i = 0; i < cameraNum; i++) {
            Camera.getCameraInfo(i, cameraInfo);
            if ((isFront) && (cameraInfo.facing == CameraInfo.CAMERA_FACING_FRONT)) {
                cameraId = i;
                orientation2 = (360 - cameraInfo.orientation + 360 - screenOrientation) % 360;
                orientation = (360 - orientation2) % 360;  // compensate the mirror
                Log.d(tag, "i:" + i + "orientation:" + orientation + " orientation2:" + orientation2 + " screenOrientation:" + screenOrientation);
                break;
            } else if ((!isFront) && (cameraInfo.facing == CameraInfo.CAMERA_FACING_BACK)) {
                cameraId = i;
                orientation = (cameraInfo.orientation + 360 - screenOrientation) % 360;
                orientation2 = orientation;//(360 - orientation) % 360;
                Log.d(tag, "ii:" + i + "orientation:" + orientation + " orientation2:" + orientation2 + " screenOrientation:" + screenOrientation);
                break;
            }
        }

        try {
            camera = Camera.open(cameraId);
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
            camera = null;
            return -1;
        }

        camera.setErrorCallback(new android.hardware.Camera.ErrorCallback() {
            @Override
            public void onError(int error, android.hardware.Camera camera) {
                String errorMessage;
                if (error == android.hardware.Camera.CAMERA_ERROR_SERVER_DIED) {
                    errorMessage = "Camera server died!";
                } else {
                    errorMessage = "Camera error: " + error;
                }

                Log.e(tag, "camera error:" + errorMessage);
                NativeEngine.stopCapture();
            }
        });

        dumpCameraInfo(camera, cameraId);

        try {
            camPara = camera.getParameters();
        } catch (Exception e) {
            e.printStackTrace();
            camera = null;
            return -2;
        }

        try {
            Camera.Size size = getCloselyPreSize(VideoMgr.getWidth(), VideoMgr.getHeight(), camPara.getSupportedPreviewSizes(), false);
            if (size == null) {
                Log.d(tag, "could not getCloselyPreSize, use default: width = " + DEFAULE_WIDTH + ", height = " + DEFAULE_HEIGHT);
                camPara.setPreviewSize(DEFAULE_WIDTH, DEFAULE_HEIGHT);
                mWidth = DEFAULE_WIDTH;
                mHeight = DEFAULE_HEIGHT;
            } else {
                Log.d(tag, "width = " + size.width + ", height = " + size.height + "; w = " + VideoMgr.getWidth() + ", h = " + VideoMgr.getHeight() + ", fps = " + VideoMgr.getFps());
                camPara.setPreviewSize(size.width, size.height);
                mWidth = size.width;
                mHeight = size.height;
            }
        } catch (Exception e) {
            e.printStackTrace();
            camera = null;
            return -2;
        }

        try {
            mFmt = YOUME_VIDEO_FMT.VIDEO_FMT_NV21;
            camPara.setPreviewFormat(ImageFormat.NV21);
        } catch (Exception e) {
            e.printStackTrace();
            Log.e(tag, "nv21 failed,try yv12");
            mFmt = YOUME_VIDEO_FMT.VIDEO_FMT_YV12;
            try {
                camPara.setPreviewFormat(ImageFormat.YV12);
            } catch (Exception e2) {
                e2.printStackTrace();
                return -2;
            }
        }

        ///先设置一下，有些机器上设置帧率会失败，所以其他参数先设置吧
        try {
            camera.setParameters(camPara);
        } catch (Exception e) {
            e.printStackTrace();
        }

        try {
            //设置自动对焦
            List<String> focusModes = camPara.getSupportedFocusModes();
            if (focusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
                mIsSupportAutoFocusContinuousPicture = true;
                camPara.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);// 自动连续对焦
            } else if (focusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                mIsSupportAutoFocus = true;
                camPara.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);// 自动对焦
            } else {
                mIsSupportAutoFocusContinuousPicture = false;
                mIsSupportAutoFocus = false;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        camPara.setPreviewFpsRange(VideoMgr.getFps() * 1000, VideoMgr.getFps() * 1000);
        Log.i(tag, "minfps = " + (VideoMgr.getFps() * 1000) + " maxfps = " + (VideoMgr.getFps() * 1000));

        camWhiteBalance = camPara.getWhiteBalance();
        camFocusMode = camPara.getFocusMode();
        Log.d(tag, "white balance = " + camWhiteBalance + ", focus mode = " + camFocusMode);

        try {
            camera.setParameters(camPara);
        } catch (Exception e) {
            needLoseFrame = true;
            setCatpturePreViewFps();
            e.printStackTrace();
        }

        mFps = VideoMgr.getFps();
        int mFrameWidth = camPara.getPreviewSize().width;
        int mFrameHeigth = camPara.getPreviewSize().height;
        int frameSize = mFrameWidth * mFrameHeigth;
        frameSize = frameSize * ImageFormat.getBitsPerPixel(camPara.getPreviewFormat()) / 8;
        boolean useEGL = api.getUseGL();
        if (!useEGL) {
            mBuffer = new byte[frameSize];
            camera.addCallbackBuffer(mBuffer);
            camera.setPreviewCallbackWithBuffer(youmePreviewCallback);
        }

        if ((Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) && (null == svCamera)) {
            GLESVideoMixer.SurfaceContext surfaceContext = VideoMixerHelper.getCameraSurfaceContext();
            if (surfaceContext != null) {
                mTextureId = surfaceContext.textureId;
                mSurfaceTexture = surfaceContext.surfaceTexture;
                if (useEGL) mSurfaceTexture.setOnFrameAvailableListener(onFrameAvailableListener);
            } else {
                mTextureId = GlUtil.generateTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES);
                mSurfaceTexture = new SurfaceTexture(mTextureId);
            }

            try {
                camera.setPreviewTexture(mSurfaceTexture);
                camera.startPreview();
                if (mIsSupportAutoFocusContinuousPicture) {
                    camera.cancelAutoFocus();
                }
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                camera.release();
                camera = null;
                return -1;
            }
        } else {
            try {
                if (null == svCamera) {
                    camera.setPreviewDisplay(null);
                } else {
                    camera.setPreviewDisplay(svCamera.getHolder());
                }
                camera.startPreview();
                if (mIsSupportAutoFocusContinuousPicture) {
                    camera.cancelAutoFocus();
                }
            } catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                camera.release();
                camera = null;
                return -1;
            }
        }
        totalTime = 0;
        frameCount = 0;
        isFrontCamera = isFront;
        return 0;
    }

    public void setCatpturePreViewFps() {
        try {
            if (camera == null) {
                return;
            }
            List<int[]> fpsRangeList = camPara.getSupportedPreviewFpsRange();
            Camera.Parameters p = camera.getParameters();
            p.setPreviewFpsRange(fpsRangeList.get(0)[0], fpsRangeList.get(0)[1]);
            Log.i(tag, "fix minfps = " + fpsRangeList.get(0)[0] + " maxfps = " + fpsRangeList.get(0)[1]);
            camera.setParameters(p);
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public int closeCamera() {
        Log.e(tag, "closeCamera");
        if (camera != null) {
            try {
                camera.setPreviewCallback(null);
                camera.stopPreview();
                camera.release();
                camera = null;
            } catch (Throwable e) {
                camera = null;
                e.printStackTrace();
            }
        }

        return 0;
    }

    public void setSvCamera(SurfaceView svCamera) {
        this.svCamera = svCamera;
    }

    private SurfaceTexture.OnFrameAvailableListener onFrameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {
        @Override
        public void onFrameAvailable(SurfaceTexture surfaceTexture) {
            int mirror = mMirrorMode;
            if (isFrontCamera) {
                if (YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_AUTO == mirror) { //自适应模式下，近端/远端都镜像
                    mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR;
                } else if (YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR == mirror) {
                    mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED;  // 前置摄像头预览默认为镜像模式，关闭镜像则是再镜像一次
                }
            } else {
                mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_DISABLED;
            }
            api.inputVideoFrameGLES(mTextureId, null, mWidth, mHeight, YOUME_VIDEO_FMT.VIDEO_FMT_TEXTURE_OES, orientation2, mirror, System.currentTimeMillis());
        }
    };

    Camera.PreviewCallback youmePreviewCallback = new Camera.PreviewCallback() {
        @Override
        public void onPreviewFrame(byte[] data, Camera camera) {
            //times = System.currentTimeMillis();
            //Log.d(tag,"onPreviewFrame");
            pushVideoFrame(data);
            if (camera != null) {
                camera.addCallbackBuffer(data);
            }
        }
    };

    private void pushVideoFrame(byte[] data) {
        int mirror = mMirrorMode;
        if (isFrontCamera) {
            if (YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_AUTO == mirror) { //自适应模式下，近端/远端都镜像
                mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_NEAR;
            } else if (YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_FAR == mirror) {
                mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_ENABLED;  // 前置摄像头预览默认为镜像模式，关闭镜像则是再镜像一次
            }
        } else {
            mirror = YouMeVideoMirrorMode.YOUME_VIDEO_MIRROR_MODE_DISABLED;
        }
        if (needLoseFrame) {
            long newTime = System.currentTimeMillis();
            if ((newTime - totalTime) / (1000.0f / mFps) >= frameCount && frameCount < mFps) {
                frameCount++;
                if (data != null)
                    api.inputVideoFrame(data, data.length, mWidth, mHeight, mFmt, orientation, mirror, System.currentTimeMillis());
            } else {
                droped = true;
            }
            if (newTime - totalTime >= 1000) {
                totalTime = newTime;
                if (frameCount > 5) {
                    needLoseFrame = droped;
                }
                frameCount = 0;
            }
        } else {
            if (data != null)
                api.inputVideoFrame(data, data.length, mWidth, mHeight, mFmt, orientation, mirror, System.currentTimeMillis());
        }
    }

    public static int startCapture() {
        Log.i(tag, "start capture is called");
        isFrontCamera = VideoMgr.isFrontCamera();
        return CameraMgr.getInstance().openCamera(isFrontCamera);
    }

    public static int stopCapture() {
        int ret = 0;
        Log.i(tag, "stop capture is called.");
        CameraMgr.getInstance().closeCamera();

        return ret;
    }

    public static int switchCamera() {
        Log.i(tag, "switchCamera is called");
        int ret = 0;
        isFrontCamera = !isFrontCamera;
        CameraMgr.getInstance().closeCamera();
        ret = CameraMgr.getInstance().openCamera(isFrontCamera);
        if (ret == 0 && CameraMgr.getInstance().mAudtoFocusFace)
            CameraMgr.getInstance().setCameraMgrAutoFocusFaceModeEnabled(true);
        return ret;
    }

    public static boolean isCameraZoomSupported() {
        return CameraMgr.getInstance().isCameraMgrZoomSupported();
    }

    public static float setCameraZoomFactor(float zoomFactor) {
        return CameraMgr.getInstance().setCameraMgrZoomFactor(zoomFactor);
    }

    public static boolean isCameraFocusPositionInPreviewSupported() {
        return CameraMgr.getInstance().isCameraMgrFocusPositionInPreviewSupported();
    }

    public static boolean setCameraFocusPositionInPreview(float x, float y) {
        return CameraMgr.getInstance().setCameraMgrFocusPositionInPreview(x, y);
    }

    public static boolean isCameraTorchSupported() {
        return CameraMgr.getInstance().isCameraMgrTorchSupported();
    }

    public static boolean setCameraTorchOn(boolean isOn) {
        return CameraMgr.getInstance().setCameraMgrTorchOn(isOn);
    }

    public static boolean isCameraAutoFocusFaceModeSupported() {
        return CameraMgr.getInstance().isCameraMgrAutoFocusFaceModeSupported();
    }

    public static boolean setCameraAutoFocusFaceModeEnabled(boolean enable) {
        return CameraMgr.getInstance().setCameraMgrAutoFocusFaceModeEnabled(enable);
    }

    public static void setCameraAutoFocusCallBack(Camera.AutoFocusCallback cb) {
        mFocusCallback = cb;
    }

    public boolean isCameraMgrZoomSupported() {
        if (camera == null)
            return false;
        try {
            Camera.Parameters parameters = camera.getParameters();
            if (parameters.isZoomSupported())
                return true;
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(tag, "camera not supported zoom!");
        return false;
    }

    public float setCameraMgrZoomFactor(float zoomFactor) {
        if (camera == null)
            return 1.0f;
        try {
            Camera.Parameters parameters = camera.getParameters();
            if (!parameters.isZoomSupported())
                return 1.0f;
            float ratio = 100.f;
            List<Integer> list = parameters.getZoomRatios();
            if (list != null && list.size() > 0 && parameters.getMaxZoom() != 0)
                ratio = (list.get(0) + list.get(list.size() - 1)) / parameters.getMaxZoom();
            int zoom = Float.valueOf(zoomFactor * ratio).intValue();
            if (zoom >= ratio && zoom <= parameters.getMaxZoom()) {
                parameters.setZoom(zoom);
                camera.setParameters(parameters);
                return zoom / ratio;
            } else {
                Log.i(tag, "camera set zoom out max :" + parameters.getMaxZoom() / ratio);
                return parameters.getZoom() / ratio;
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return 1.0f;
    }

    public boolean isCameraMgrFocusPositionInPreviewSupported() {
        if (mIsSupportAutoFocus || mIsSupportAutoFocusContinuousPicture)
            return true;
        Log.i(tag, "camera not supported focus positoin!");
        return false;
    }

    public boolean setCameraMgrFocusPositionInPreview(float x, float y) {
        if (!mIsSupportAutoFocus && !mIsSupportAutoFocusContinuousPicture || camera == null) {
            return false;
        }

        float focusX = x;
        float focusY = y;
        if (orientation == 90) {
            float tmp = focusX;
            focusY = 1.f - focusX;
            focusX = (tmp - 1.f) * -1;
        } else if (orientation == 270) {
            float tmp = focusX;
            focusY = focusX;
            focusX = tmp;
        }

        try {
            mAutoFocusLocked = true;
            camera.cancelAutoFocus();
            Camera.Parameters parameters = camera.getParameters();
            parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
            if (parameters.getMaxNumFocusAreas() > 0) {
                Rect focusRect = calculateTapArea(focusX, focusY, 1f, 0);
                List<Camera.Area> focusAreas = new ArrayList<Camera.Area>();
                focusAreas.add(new Camera.Area(focusRect, 1000));
                parameters.setFocusAreas(focusAreas);
            }

            if (parameters.getMaxNumMeteringAreas() > 0) {
                Rect meteringRect = calculateTapArea(focusX, focusY, 1.5f, 0);
                List<Camera.Area> meteringAreas = new ArrayList<Camera.Area>();
                meteringAreas.add(new Camera.Area(meteringRect, 1000));
                parameters.setMeteringAreas(meteringAreas);
            }

            camera.setParameters(parameters);
            camera.autoFocus(this);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            mAutoFocusLocked = false;
        }
        return false;
    }

    public boolean isCameraMgrTorchSupported() {
        if (camera == null)
            return false;
        try {
            Camera.Parameters parameters = camera.getParameters();
            List<String> features = parameters.getSupportedFlashModes();//判断是否支持闪光灯
            if (features.contains(Camera.Parameters.FLASH_MODE_ON))
                return true;
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(tag, "camera not supported torch!");
        return false;
    }

    public boolean setCameraMgrTorchOn(boolean isOn) {
        if (camera == null)
            return false;
        try {
            Camera.Parameters parameters = camera.getParameters();
            if (parameters.getSupportedFocusModes().contains(android.hardware.Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
                if (isOn)
                    parameters.setFlashMode(Camera.Parameters.FLASH_MODE_TORCH);
                else
                    parameters.setFlashMode(Camera.Parameters.FLASH_MODE_OFF);
            }
            camera.setParameters(parameters);
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    public boolean isCameraMgrAutoFocusFaceModeSupported() {
        if (camera == null || (!mIsSupportAutoFocus && !mIsSupportAutoFocusContinuousPicture)) {
            Log.i(tag, "camera not supported auto face focus!");
            return false;
        }
        try {
            Camera.Parameters parameters = camera.getParameters();
            if (parameters.getMaxNumDetectedFaces() > 0)
                return true;
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.i(tag, "camera not supported auto face focus!");

        return false;
    }

    public boolean setCameraMgrAutoFocusFaceModeEnabled(boolean enable) {
        if (camera == null || (!mIsSupportAutoFocus && !mIsSupportAutoFocusContinuousPicture))
            return false;

        try {
            Camera.Parameters parameters = camera.getParameters();
            if (parameters.getMaxNumDetectedFaces() > 0) {
                if (enable) {
                    camera.setFaceDetectionListener(mFaceDetectionListener); //添加人脸识别监听
                    camera.startFaceDetection();
                    mAudtoFocusFace = true;
                } else {
                    camera.stopFaceDetection();
                    mAudtoFocusFace = false;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        return false;
    }


    private static final int RESET_TOUCH_FOCUS = 301;
    private static final int RESET_TOUCH_FOCUS_DELAY = 3000;

    private static class CameraControllerHandler extends Handler {

        private CommonHandlerListener listener;

        public CameraControllerHandler(CommonHandlerListener listener) {
            super(Looper.getMainLooper());
            this.listener = listener;
        }

        @Override
        public void handleMessage(Message msg) {
            listener.handleMessage(msg);
        }
    }

    @Override
    public void handleMessage(Message msg) {
        switch (msg.what) {
            case RESET_TOUCH_FOCUS: {
                if (camera == null || mAutoFocusLocked) {
                    return;
                }
                mHandler.removeMessages(RESET_TOUCH_FOCUS);
                try {
                    if (mIsSupportAutoFocusContinuousPicture) {
                        Camera.Parameters cp = camera.getParameters();
                        if (cp != null) {
                            cp.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
                            camera.setParameters(cp);
                        }
                    }
                    camera.cancelAutoFocus();
                } catch (Exception e) {
                    e.printStackTrace();
                }

                break;
            }
        }
    }


    private FaceDetectionListener mFaceDetectionListener = new FaceDetectionListener() {
        @Override
        public void onFaceDetection(Face[] faces, Camera camera) {
//        	for(int i = 0; i < faces.length; i++){
//        		Log.i("====人脸识别====","" + faces[i].rect.left + " x " + + faces[i].rect.top);
//        	}

            if (faces.length > 0) {
                int index = faces.length / 2;
                float centerX = (faces[index].rect.exactCenterX() + 1000) / 2000.f;
                float centerY = (faces[index].rect.exactCenterY() + 1000) / 2000.f;
                setCameraMgrFocusPositionInPreview(centerX, centerY);
            }

        }
    };

    @Override
    public void onAutoFocus(boolean success, Camera camera) {
        mHandler.sendEmptyMessageDelayed(RESET_TOUCH_FOCUS, RESET_TOUCH_FOCUS_DELAY);
        mAutoFocusLocked = false;
        if (mFocusCallback != null)
            mFocusCallback.onAutoFocus(success, camera);
    }

    //ErrorCallback
    @Override
    public void onError(int error, Camera camera) {

    }

    private static class PermissionCheckThread extends Thread {
        @Override
        public void run() {
            try {
                Log.i(tag, "PermissionCheck starts...");
                Context mContext = context.get();
                while (!Thread.interrupted()) {
                    requestPermissionCount++;
                    Thread.sleep(3000);
                    Log.i(tag, "PermissionCheck starts...running");
                    if ((mContext != null) && (mContext instanceof Activity)) {
                        int cameraPermission = ContextCompat.checkSelfPermission((Activity) mContext, Manifest.permission.CAMERA);
                        if (cameraPermission == PackageManager.PERMISSION_GRANTED) {
                            // Once the permission is granted, reset the microphone to take effect.
                            NativeEngine.resetCamera();
                            break;
                        }
                    } else {
                        Log.i(tag, "PermissionCheck mContext not Activity");
                    }
                    if (requestPermissionCount > 3) {
                        //最多检查5分钟吧
                        requestPermissionCount = 0;
                        break;
                    }
                }
            } catch (InterruptedException e) {
                Log.i(tag, "PermissionCheck interrupted");
            } catch (Throwable e) {
                Log.e(tag, "PermissionCheck caught a throwable:" + e.getMessage());

            }
            Log.i(tag, "PermissionCheck exit");
        }
    }

    private static PermissionCheckThread mPermissionCheckThread = null;

    public static boolean startRequestPermissionForApi23() {
        boolean isApiLevel23 = false;
        if (isStopedByExternalNotify) {
            return isApiLevel23;
        }
        try {
            Context mContext = context.get();//AppPara.getContext();
            if ((Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) && (mContext != null)
                    && (mContext instanceof Activity) && (mContext.getApplicationInfo().targetSdkVersion >= 23)) {

                isApiLevel23 = true;
                // TODO 第一次在ZUI、vivo 6.0系统上可能采集到的帧率很低
                int cameraPermission = ContextCompat.checkSelfPermission((Activity) mContext, Manifest.permission.CAMERA);
                if (cameraPermission != PackageManager.PERMISSION_GRANTED) {
                    Log.w(tag, "Request for camera permission");
                    ActivityCompat.requestPermissions((Activity) context.get(), new String[]{Manifest.permission.CAMERA}, 9);
                    // Start a thread to check if the permission is granted. Once it's granted, reset the microphone to apply it.
                    if (mPermissionCheckThread != null) {
                        mPermissionCheckThread.interrupt();
                        mPermissionCheckThread.join(2000);
                    }
                    mPermissionCheckThread = new PermissionCheckThread();
                    if (mPermissionCheckThread != null) {
                        mPermissionCheckThread.start();
                    }
                } else {
                    Log.i(tag, "Already got camera permission");
                }
            }
        } catch (Throwable e) {
            Log.e(tag, "Exception for startRequirePermiForApi23");
            e.printStackTrace();
        }

        return isApiLevel23;
    }

    public static void stopRequestPermissionForApi23() {
        try {
            if (mPermissionCheckThread != null) {
                mPermissionCheckThread.interrupt();
                mPermissionCheckThread.join(2000);
                mPermissionCheckThread = null;
            }
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    public static void OnReqeustPermissionResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        // 检查返回授权结果不为空
        if (grantResults.length > 0) {
            // 判断授权结果
            for (int i = 0; i < grantResults.length; i++) {
                if (permissions[i].equals(Manifest.permission.CAMERA)) {
                    isStopedByExternalNotify = true;
                    stopRequestPermissionForApi23();
                    int result = grantResults[i];
                    if (result == PackageManager.PERMISSION_GRANTED) {
                        Log.i(tag, "OnReqeustPermissionResult Already got record permission");
                        //允许
                        if (api.mInited) NativeEngine.resetCamera();
                    } else {
                        Log.i(tag, "OnReqeustPermissionResult user not granted permission");
                        //拒绝
                        if (api.mInited) NativeEngine.resetCamera();
                    }
                }
            }
        }
    }

    class YMSurfaceHolderCallback implements SurfaceHolder.Callback {
        @Override
        public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            // TODO Auto-generated method stub
            Log.e(tag, "surfaceChanged is called.");
        }

        @Override
        public void surfaceCreated(SurfaceHolder holder) {
            // TODO Auto-generated method stub
            Log.e(tag, "surfaceCreated is called.");
        }

        @Override
        public void surfaceDestroyed(SurfaceHolder holder) {
            // TODO Auto-generated method stub
            Log.e(tag, "surfaceDestroyed is called.");
        }
    }

    /**
     * 通过对比得到与宽高比最接近的尺寸（如果有相同尺寸，优先选择）
     *
     * @param surfaceWidth  需要被进行对比的原宽
     * @param surfaceHeight 需要被进行对比的原高
     * @param preSizeList   需要对比的预览尺寸列表
     * @return 得到与原宽高比例最接近的尺寸
     */
    protected Camera.Size getCloselyPreSize(int surfaceWidth, int surfaceHeight, List<Camera.Size> preSizeList, boolean mIsPortrait) {
        int ReqTmpWidth;
        int ReqTmpHeight;
        // 当屏幕为垂直的时候需要把宽高值进行调换，保证宽大于高
        switch (orientation) {
            case 90:
            case 270:
                ReqTmpWidth = surfaceHeight;
                ReqTmpHeight = surfaceWidth;
                break;
            default:
                ReqTmpWidth = surfaceWidth;
                ReqTmpHeight = surfaceHeight;
                break;
        }

        //先查找preview中是否存在与surfaceview相同宽高的尺寸
        float wRatio = 1.0f;
        float hRatio = 1.0f;
        List<Camera.Size> tempList = new ArrayList<Camera.Size>();
        for (Camera.Size size : preSizeList) {
            Log.i(tag, "supported size: w " + size.width + " h " + size.height);
            wRatio = (((float) size.width) / ReqTmpWidth);
            hRatio = (((float) size.height) / ReqTmpHeight);
            if ((wRatio >= 1.0) && (hRatio >= 1.0)) {
                tempList.add(size);
            }
        }

        int pixelCount = 0;
        Camera.Size retSize = null;
        for (Camera.Size size : tempList) {
            if (0 == pixelCount) {
                pixelCount = size.width * size.height;
                retSize = size;
            } else {
                if ((size.width * size.height) < pixelCount) {
                    pixelCount = size.width * size.height;
                    retSize = size;
                }
            }
        }

        return retSize;
    }

    /**
     * 摄像头信息
     */
    private void dumpCameraInfo(Camera camera, int cameraId) {
        Log.d(tag, "============ Camera Info (" + cameraId + ")============ ");

        int cameraNum = Camera.getNumberOfCameras();
        CameraInfo cameraInfo = new CameraInfo();
        for (int i = 0; i < cameraNum; i++) {
            Camera.getCameraInfo(i, cameraInfo);
            Log.d(tag, "facing : " + cameraInfo.facing + "orientation : " + cameraInfo.orientation);
            Log.d(tag, cameraInfo.toString());
        }

        try {
            Camera.Parameters p = camera.getParameters();

            Log.d(tag, "============ support format ============ ");
            for (Integer f : p.getSupportedPreviewFormats()) {
                Log.d(tag, "support format " + f);
            }

            Log.d(tag, "============ support FpsRange ============ ");
            for (int[] range : p.getSupportedPreviewFpsRange()) {
                Log.d(tag, "support range " + range[0] + " - " + range[1]);
            }

            Log.d(tag, "============ Preview Size ============ ");
            for (Camera.Size size : p.getSupportedPreviewSizes()) {
                Log.d(tag, "support size " + size.width + " x " + size.height);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public static Rect calculateTapArea(float x, float y, float coefficient, int orientation) {
        if (orientation == 90 || orientation == 270) {
            float tmp = x;
            x = y;
            y = tmp;
        }

        float focusAreaSize = 200;
        int areaSize = Float.valueOf(focusAreaSize * coefficient).intValue();
        int centerX = (int) (x * 2000 - 1000);
        int centerY = (int) (y * 2000 - 1000);

        int left = clamp(centerX - areaSize / 2, -1000, 1000);
        int right = clamp(left + areaSize, -1000, 1000);
        int top = clamp(centerY - areaSize / 2, -1000, 1000);
        int bottom = clamp(top + areaSize, -1000, 1000);

        return new Rect(left, top, right, bottom);
    }

    private static int clamp(int x, int min, int max) {
        if (x > max) {
            return max;
        }
        if (x < min) {
            return min;
        }
        return x;
    }
}
