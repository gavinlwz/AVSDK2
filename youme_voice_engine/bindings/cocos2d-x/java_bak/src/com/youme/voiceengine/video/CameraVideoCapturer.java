package com.youme.voiceengine.video;

/**
 * Created by fire on 2017/2/14.
 */

import android.util.Log;

/**
 * Base interface for camera1 and camera2 implementations. Extends VideoCapturer with a
 * switchCamera() function. Also provides subinterfaces for handling camera events, and a helper
 * class for detecting camera freezes.
 */
public interface CameraVideoCapturer extends VideoBaseCapturer {
    /**
     * Camera events handler - can be used to be notifed about camera events. The callbacks are
     * executed from an arbitrary thread.
     */
    public interface CameraEventsHandler {
        // Camera error handler - invoked when camera can not be opened
        // or any camera exception happens on camera thread.
        void onCameraError(String errorDescription);

        // Called when camera is disconnected.
        void onCameraDisconnected();

        // Invoked when camera stops receiving frames.
        void onCameraFreezed(String errorDescription);

        // Callback invoked when camera is opening.
        void onCameraOpening(String cameraName);

        // Callback invoked when first camera frame is available after camera is started.
        void onFirstFrameAvailable();

        // Callback invoked when camera is closed.
        void onCameraClosed();
    }

    /**
     * Camera switch handler - one of these functions are invoked with the result of switchCamera().
     * The callback may be called on an arbitrary thread.
     */
    public interface CameraSwitchHandler {
        // Invoked on success. |isFrontCamera| is true if the new camera is front facing.
        void onCameraSwitchDone(boolean isFrontCamera);

        // Invoked on failure, e.g. camera is stopped or only one camera available.
        void onCameraSwitchError(String errorDescription);
    }

    /**
     * Switch camera to the next valid camera id. This can only be called while the camera is running.
     * This function can be called from any thread.
     */
    void switchCamera(CameraSwitchHandler switchEventsHandler);

    /**
     * Helper class to log framerate and detect if the camera freezes. It will run periodic callbacks
     * on the SurfaceTextureHelper thread passed in the ctor, and should only be operated from that
     * thread.
     */
    public static class CameraStatistics {
        private final static String TAG = "CameraStatistics";
        private final static int CAMERA_OBSERVER_PERIOD_MS = 2000;
        private final static int CAMERA_FREEZE_REPORT_TIMOUT_MS = 4000;

        private final SurfaceTextureHelper surfaceTextureHelper;
        private final CameraEventsHandler eventsHandler;
        private int frameCount;
        private int freezePeriodCount;
        // Camera observer - monitors camera framerate. Observer is executed on camera thread.
        private final Runnable cameraObserver = new Runnable() {
            @Override
            public void run() {
                final int cameraFps = Math.round(frameCount * 1000.0f / CAMERA_OBSERVER_PERIOD_MS);
                Log.d(TAG, "Camera fps: " + cameraFps + ".");
                if (frameCount == 0) {
                    ++freezePeriodCount;
                    if (CAMERA_OBSERVER_PERIOD_MS * freezePeriodCount >= CAMERA_FREEZE_REPORT_TIMOUT_MS
                            && eventsHandler != null) {
                        Log.e(TAG, "Camera freezed.");
                        if (surfaceTextureHelper.isTextureInUse()) {
                            // This can only happen if we are capturing to textures.
                            eventsHandler.onCameraFreezed("Camera failure. Client must return video buffers.");
                        } else {
                            eventsHandler.onCameraFreezed("Camera failure.");
                        }
                        return;
                    }
                } else {
                    freezePeriodCount = 0;
                }
                frameCount = 0;
                surfaceTextureHelper.getHandler().postDelayed(this, CAMERA_OBSERVER_PERIOD_MS);
            }
        };

        public CameraStatistics(
                SurfaceTextureHelper surfaceTextureHelper, CameraEventsHandler eventsHandler) {
            if (surfaceTextureHelper == null) {
                throw new IllegalArgumentException("SurfaceTextureHelper is null");
            }
            this.surfaceTextureHelper = surfaceTextureHelper;
            this.eventsHandler = eventsHandler;
            this.frameCount = 0;
            this.freezePeriodCount = 0;
            surfaceTextureHelper.getHandler().postDelayed(cameraObserver, CAMERA_OBSERVER_PERIOD_MS);
        }

        private void checkThread() {
            if (Thread.currentThread() != surfaceTextureHelper.getHandler().getLooper().getThread()) {
                throw new IllegalStateException("Wrong thread");
            }
        }

        public void addFrame() {
            checkThread();
            ++frameCount;
        }

        public void release() {
            surfaceTextureHelper.getHandler().removeCallbacks(cameraObserver);
        }
    }
}
