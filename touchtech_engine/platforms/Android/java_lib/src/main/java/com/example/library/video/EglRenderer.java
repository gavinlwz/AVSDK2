package main.java.com.example.library.video;

/**
 * Created by fire on 2017/2/14.
 */


import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import main.java.com.example.library.ThreadUtils;

/**
 * Implements org.webrtc.VideoRenderer.Callbacks by displaying the video stream on an EGL Surface.
 * This class is intended to be used as a helper class for rendering on SurfaceViews and
 * TextureViews.
 */
public class EglRenderer implements VideoBaseRenderer.Callbacks {
    private static final String TAG = "EglRenderer";
    private static final long LOG_INTERVAL_SEC = 10;
    private static final int MAX_SURFACE_CLEAR_COUNT = 3;

    public interface FrameListener { void onFrame(Bitmap frame); }

    private static class FrameListenerAndParams {
        public final FrameListener listener;
        public final float scale;
        public final RendererCommon.GlDrawer drawer;

        public FrameListenerAndParams(
                FrameListener listener, float scale, RendererCommon.GlDrawer drawer) {
            this.listener = listener;
            this.scale = scale;
            this.drawer = drawer;
        }
    }

    private class EglSurfaceCreation implements Runnable {
        private Object surface;

        public synchronized void setSurface(Object surface) {
            this.surface = surface;
        }

        @Override
        public synchronized void run() {
            if (surface != null && eglBase != null && !eglBase.hasSurface()) {
                if (surface instanceof Surface) {
                    eglBase.createSurface((Surface) surface);
                    logD("createSurface");
                } else if (surface instanceof SurfaceTexture) {
                    eglBase.createSurface((SurfaceTexture) surface);
                    logD("createSurface");
                } else {
                    throw new IllegalStateException("Invalid surface: " + surface);
                }
                eglBase.makeCurrent();

                GLES20.glClearColor( 125 /* red */, 125 /* green */, 125 /* blue */, 255 /* alpha */);
                // Necessary for YUV frames with odd width.
                GLES20.glPixelStorei(GLES20.GL_UNPACK_ALIGNMENT, 1);
            }
        }
    }

    private final String name;

    // |renderThreadHandler| is a handler for communicating with |renderThread|, and is synchronized
    // on |handlerLock|.
    private final Object handlerLock = new Object();
    private Handler renderThreadHandler;

    private final ArrayList<FrameListenerAndParams> frameListeners = new ArrayList<FrameListenerAndParams>();

    // Variables for fps reduction.
    private final Object fpsReductionLock = new Object();
    // Time for when next frame should be rendered.
    private long nextFrameTimeNs;
    // Minimum duration between frames when fps reduction is active, or -1 if video is completely
    // paused.
    private long minRenderPeriodNs;

    // EGL and GL resources for drawing YUV/OES textures. After initilization, these are only accessed
    // from the render thread.
    private EglBase eglBase;
    private final RendererCommon.YuvUploader yuvUploader = new RendererCommon.YuvUploader();
    private RendererCommon.GlDrawer drawer;
    // Texture ids for YUV frames. Allocated on first arrival of a YUV frame.
    private int[] yuvTextures = null;

    // Pending frame to render. Serves as a queue with size 1. Synchronized on |frameLock|.
    private final Object frameLock = new Object();
    private VideoBaseRenderer.I420Frame pendingFrame;

    // These variables are synchronized on |layoutLock|.
    private final Object layoutLock = new Object();
    private float layoutAspectRatio;
    // If true, mirrors the video stream horizontally.
    private boolean mirror;

    // These variables are synchronized on |statisticsLock|.
    private final Object statisticsLock = new Object();
    // Total number of video frames received in renderFrame() call.
    private int framesReceived;
    // Number of video frames dropped by renderFrame() because previous frame has not been rendered
    // yet.
    private int framesDropped;
    // Number of rendered video frames.
    private int framesRendered;
    // Start time for counting these statistics, or 0 if we haven't started measuring yet.
    private long statisticsStartTimeNs;
    // Time in ns spent in renderFrameOnRenderThread() function.
    private long renderTimeNs;
    // Time in ns spent by the render thread in the swapBuffers() function.
    private long renderSwapBufferTimeNs;
    
    private int renderWidth;
    private int renderHeight;

    private int renderPosition;
    // Used for bitmap capturing.
    private GlTextureFrameBuffer bitmapTextureFramebuffer;

    // Runnable for posting frames to render thread.
    private final Runnable renderFrameRunnable = new Runnable() {
        @Override
        public void run() {
            renderFrameOnRenderThread();
        }
    };

    private final Runnable logStatisticsRunnable = new Runnable() {
        @Override
        public void run() {
//            logStatistics();
            synchronized (handlerLock) {
                if (renderThreadHandler != null) {
                    renderThreadHandler.removeCallbacks(logStatisticsRunnable);
                    renderThreadHandler.postDelayed(
                            logStatisticsRunnable, TimeUnit.SECONDS.toMillis(LOG_INTERVAL_SEC));
                }
            }
        }
    };

    private final EglSurfaceCreation eglSurfaceCreationRunnable = new EglSurfaceCreation();

    /**
     * Standard constructor. The name will be used for the render thread name and included when
     * logging. In order to render something, you must first call init() and createEglSurface.
     */
    public EglRenderer(String name) {
        this.name = name;
    }

    /**
     * Initialize this class, sharing resources with |sharedContext|. The custom |drawer| will be used
     * for drawing frames on the EGLSurface. This class is responsible for calling release() on
     * |drawer|. It is allowed to call init() to reinitialize the renderer after a previous
     * init()/release() cycle.
     */
    public void init(final EglBase.Context sharedContext, final int[] configAttributes,
                     RendererCommon.GlDrawer drawer) {
        synchronized (handlerLock) {
            if (renderThreadHandler != null) {
                throw new IllegalStateException(name + "Already initialized");
            }
            logD("Initializing EglRenderer");
            this.drawer = drawer;

            final HandlerThread renderThread = new HandlerThread(name + "EglRenderer");
            renderThread.start();
            renderThreadHandler = new Handler(renderThread.getLooper());
            // Create EGL context on the newly created render thread. It should be possibly to create the
            // context on this thread and make it current on the render thread, but this causes failure on
            // some Marvel based JB devices. https://bugs.chromium.org/p/webrtc/issues/detail?id=6350.
            ThreadUtils.invokeAtFrontUninterruptibly(renderThreadHandler, new Runnable() {
                @Override
                public void run() {
                    // If sharedContext is null, then texture frames are disabled. This is typically for old
                    // devices that might not be fully spec compliant, so force EGL 1.0 since EGL 1.4 has
                    // caused trouble on some weird devices.
                	 try {
                		 eglBase = EglBase.create(sharedContext, configAttributes);

                     }catch(Exception e)
                     {
                    	 try {
                    		 eglBase = EglBase10.create(null, configAttributes);

                         }catch(Exception e1)
                         {
                    	    e1.printStackTrace();
                    	    eglBase = null;
                         }
                     }
                }
            });
            renderThreadHandler.post(eglSurfaceCreationRunnable);
            final long currentTimeNs = System.nanoTime();
            resetStatistics(currentTimeNs);
            renderThreadHandler.postDelayed(
                    logStatisticsRunnable, TimeUnit.SECONDS.toMillis(LOG_INTERVAL_SEC));
        }
    }

    public void createEglSurface(Surface surface) {
        createEglSurfaceInternal(surface);
    }

    public void createEglSurface(SurfaceTexture surfaceTexture) {
        createEglSurfaceInternal(surfaceTexture);
    }

    private void createEglSurfaceInternal(Object surface) {
        eglSurfaceCreationRunnable.setSurface(surface);
        postToRenderThread(eglSurfaceCreationRunnable);
    }

    /**
     * Block until any pending frame is returned and all GL resources released, even if an interrupt
     * occurs. If an interrupt occurs during release(), the interrupt flag will be set. This function
     * should be called before the Activity is destroyed and the EGLContext is still valid. If you
     * don't call this function, the GL resources might leak.
     */
    public void release() {
        logD("Releasing.");
        final CountDownLatch eglCleanupBarrier = new CountDownLatch(1);
        synchronized (handlerLock) {
            if (renderThreadHandler == null) {
                logD("Already released");
                return;
            }
            renderThreadHandler.removeCallbacks(logStatisticsRunnable);
            // Release EGL and GL resources on render thread.
            renderThreadHandler.postAtFrontOfQueue(new Runnable() {
                @Override
                public void run() {
                    if (drawer != null) {
                        drawer.release();
                        drawer = null;
                    }
                    if (yuvTextures != null) {
                        GLES20.glDeleteTextures(3, yuvTextures, 0);
                        yuvTextures = null;
                    }
                    if (bitmapTextureFramebuffer != null) {
                        bitmapTextureFramebuffer.release();
                        bitmapTextureFramebuffer = null;
                    }
                    if (eglBase != null) {
                        logD("eglBase detach and release.");
                        eglBase.detachCurrent();
                        eglBase.release();
                        eglBase = null;
                    }
                    eglCleanupBarrier.countDown();
                }
            });
            final Looper renderLooper = renderThreadHandler.getLooper();
            // TODO(magjed): Replace this post() with renderLooper.quitSafely() when API support >= 18.
            renderThreadHandler.post(new Runnable() {
                @Override
                public void run() {
                    logD("Quitting render thread.");
                    renderLooper.quit();
                }
            });
            // Don't accept any more frames or messages to the render thread.
            renderThreadHandler = null;
        }
        // Make sure the EGL/GL cleanup posted above is executed.
        ThreadUtils.awaitUninterruptibly(eglCleanupBarrier);
        synchronized (frameLock) {
            if (pendingFrame != null) {
                VideoBaseRenderer.renderFrameDone(pendingFrame);
                pendingFrame = null;
            }
        }
        logD("Releasing done.");
    }

    /**
     * Reset the statistics logged in logStatistics().
     */
    private void resetStatistics(long currentTimeNs) {
        synchronized (statisticsLock) {
            statisticsStartTimeNs = currentTimeNs;
            framesReceived = 0;
            framesDropped = 0;
            framesRendered = 0;
            renderTimeNs = 0;
            renderSwapBufferTimeNs = 0;
        }
    }

    public void printStackTrace() {
        synchronized (handlerLock) {
            final Thread renderThread =
                    (renderThreadHandler == null) ? null : renderThreadHandler.getLooper().getThread();
            if (renderThread != null) {
                final StackTraceElement[] renderStackTrace = renderThread.getStackTrace();
                if (renderStackTrace.length > 0) {
                    logD("EglRenderer stack trace:");
                    for (StackTraceElement traceElem : renderStackTrace) {
                        logD(traceElem.toString());
                    }
                }
            }
        }
    }

    /**
     * Set if the video stream should be mirrored or not.
     */
    public void setMirror(final boolean mirror) {
        logD("setMirror: " + mirror);
        synchronized (layoutLock) {
            this.mirror = mirror;
        }
    }

    /**
     * Set layout aspect ratio. This is used to crop frames when rendering to avoid stretched video.
     * Set this to 0 to disable cropping.
     */
    public void setLayoutAspectRatio(float layoutAspectRatio) {
        Log.i(TAG,"setLayoutAspectRatio: " + layoutAspectRatio);
        synchronized (layoutLock) {
            this.layoutAspectRatio = layoutAspectRatio;
        }
       
    }

    /**
     * Limit render framerate.
     *
     * @param fps Limit render framerate to this value, or use Float.POSITIVE_INFINITY to disable fps
     *            reduction.
     */
    public void setFpsReduction(float fps) {
        logD("setFpsReduction: " + fps);
        synchronized (fpsReductionLock) {
            final long previousRenderPeriodNs = minRenderPeriodNs;
            if (fps <= 0) {
                minRenderPeriodNs = Long.MAX_VALUE;
            } else {
                minRenderPeriodNs = (long) (TimeUnit.SECONDS.toNanos(1) / fps);
            }
            if (minRenderPeriodNs != previousRenderPeriodNs) {
                // Fps reduction changed - reset frame time.
                nextFrameTimeNs = System.nanoTime();
            }
        }
    }

    public void disableFpsReduction() {
        setFpsReduction(Float.POSITIVE_INFINITY /* fps */);
    }

    public void pauseVideo() {
        setFpsReduction(0 /* fps */);
    }

    /**
     * Register a callback to be invoked when a new video frame has been received. This version uses
     * the drawer of the EglRenderer that was passed in init.
     *
     * @param listener The callback to be invoked.
     * @param scale    The scale of the Bitmap passed to the callback, or 0 if no Bitmap is
     *                 required.
     */
    public void addFrameListener(final FrameListener listener, final float scale) {
        postToRenderThread(new Runnable() {
            @Override
            public void run() {
                frameListeners.add(new FrameListenerAndParams(listener, scale, drawer));
            }
        });
    }

    /**
     * Register a callback to be invoked when a new video frame has been received.
     *
     * @param listener The callback to be invoked.
     * @param scale    The scale of the Bitmap passed to the callback, or 0 if no Bitmap is
     *                 required.
     * @param drawer   Custom drawer to use for this frame listener.
     */
    public void addFrameListener(
            final FrameListener listener, final float scale, final RendererCommon.GlDrawer drawer) {
        postToRenderThread(new Runnable() {
            @Override
            public void run() {
                frameListeners.add(new FrameListenerAndParams(listener, scale, drawer));
            }
        });
    }

    /**
     * Remove any pending callback that was added with addFrameListener. If the callback is not in
     * the queue, nothing happens. It is ensured that callback won't be called after this method
     * returns.
     *
     * @param runnable The callback to remove.
     */
    public void removeFrameListener(final FrameListener listener) {
        final CountDownLatch latch = new CountDownLatch(1);
        postToRenderThread(new Runnable() {
            @Override
            public void run() {
                latch.countDown();
                final Iterator<FrameListenerAndParams> iter = frameListeners.iterator();
                while (iter.hasNext()) {
                    if (iter.next().listener == listener) {
                        iter.remove();
                    }
                }
            }
        });
        ThreadUtils.awaitUninterruptibly(latch);
    }

    // VideoRenderer.Callbacks interface.
    @Override
    public void renderFrame(VideoBaseRenderer.I420Frame frame) {
        synchronized (statisticsLock) {
            ++framesReceived;
        }
        final boolean dropOldFrame;
        synchronized (handlerLock) {
            if (renderThreadHandler == null) {
                logD("Dropping frame - Not initialized or already released.");
                VideoBaseRenderer.renderFrameDone(frame);
                return;
            }
            // Check if fps reduction is active.
            synchronized (fpsReductionLock) {
                if (minRenderPeriodNs > 0) {
                    final long currentTimeNs = System.nanoTime();
                    if (minRenderPeriodNs == Long.MAX_VALUE || currentTimeNs < nextFrameTimeNs) {
                        logD("Dropping frame - fps reduction is active.");
                        VideoBaseRenderer.renderFrameDone(frame);
                        return;
                    }
                    nextFrameTimeNs += minRenderPeriodNs;
                    // The time for the next frame should always be in the future.
                    nextFrameTimeNs = Math.max(nextFrameTimeNs, currentTimeNs);
                }
            }
            synchronized (frameLock) {
                dropOldFrame = (pendingFrame != null);
                if (dropOldFrame) {
                    VideoBaseRenderer.renderFrameDone(pendingFrame);
                }
                pendingFrame = frame;
                renderThreadHandler.post(renderFrameRunnable);
            }
        }
        if (dropOldFrame) {
            synchronized (statisticsLock) {
                ++framesDropped;
            }
        }
    }

    /**
     * Release EGL surface. This function will block until the EGL surface is released.
     */
    public void releaseEglSurface(final Runnable completionCallback) {
        // Ensure that the render thread is no longer touching the Surface before returning from this
        // function.
        eglSurfaceCreationRunnable.setSurface(null /* surface */);
        synchronized (handlerLock) {
            if (renderThreadHandler != null) {
                renderThreadHandler.removeCallbacks(eglSurfaceCreationRunnable);
                renderThreadHandler.postAtFrontOfQueue(new Runnable() {
                    @Override
                    public void run() {
                        if (eglBase != null) {
                            eglBase.detachCurrent();
                            eglBase.releaseSurface();
                        }
                        completionCallback.run();
                    }
                });
                return;
            }
        }
        completionCallback.run();
    }

    /**
     * Private helper function to post tasks safely.
     */
    private void postToRenderThread(Runnable runnable) {
        synchronized (handlerLock) {
            if (renderThreadHandler != null) {
                renderThreadHandler.post(runnable);
            }
        }
    }

    private void clearSurfaceOnRenderThread() {
        if (eglBase != null && eglBase.hasSurface()) {
            logD("clearSurface");
//            GLES20.glClearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
            GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
            eglBase.swapBuffers();
        }
    }

    public void setBackgroundColor(final int r, final  int g , final  int b , final  int alpha ){

        postToRenderThread( new Runnable() {
            @Override
            public void run() {
                if (eglBase != null && eglBase.hasSurface()) {
                    GLES20.glClearColor( (float)r/255 /* red */, (float)g/255 /* green */, (float)b/255 /* blue */, (float)alpha/255 /* alpha */);
                    clearSurfaceOnRenderThread();
                }
            }
        });
    }

    /**
     * Post a task to clear the TextureView to a transparent uniform color.
     */
    public void clearImage() {
        synchronized (handlerLock) {
            if (renderThreadHandler == null) {
                return;
            }
            renderThreadHandler.postAtFrontOfQueue(new Runnable() {
                @Override
                public void run() {
                    clearSurfaceOnRenderThread();
                }
            });
        }
    }

    /**
     * Renders and releases |pendingFrame|.
     */
    private void renderFrameOnRenderThread() {
        // Fetch and render |pendingFrame|.
        int viewWidth, viewHieght;
        final VideoBaseRenderer.I420Frame frame;
        synchronized (frameLock) {
            if (pendingFrame == null) {
                return;
            }
            frame = pendingFrame;
            pendingFrame = null;
        }
        if (eglBase == null || !eglBase.hasSurface()) {
            logD("Dropping frame - No surface");
            VideoBaseRenderer.renderFrameDone(frame);
            return;
        }
        if(renderWidth != 0 &&  renderHeight != 0){
        	viewWidth = renderWidth;
        	viewHieght = renderHeight;
        }else{
        	viewWidth = eglBase.surfaceWidth();
        	viewHieght = eglBase.surfaceHeight();
        }
       
       //if(eglBase.surfaceWidth() != renderWidth || eglBase.surfaceHeight() != renderHeight)
       // 	Log.i(TAG,"surface render  viewWidth:" + renderWidth + " viewHieght:"+ "renderHeight" + 
       //         " surfaceWidth:"+ eglBase.surfaceWidth() + " surfaceHeight:"+eglBase.surfaceHeight());

        final long startTimeNs = System.nanoTime();
        final float[] texMatrix =
                RendererCommon.rotateTextureMatrix(frame.samplingMatrix, frame.rotationDegree);
        float[] drawMatrix;

        // After a surface size change, the EGLSurface might still have a buffer of the old size in the
        // pipeline. Querying the EGLSurface will show if the underlying buffer dimensions haven't yet
        // changed. Such a buffer will be rendered incorrectly, so flush it with a black frame.
        final int drawnFrameWidth;
        final int drawnFrameHeight;
        synchronized (layoutLock) {
            final float[] layoutMatrix;
            if (layoutAspectRatio > 0) {
                final float frameAspectRatio = frame.rotatedWidth() / (float) frame.rotatedHeight();
                layoutMatrix = RendererCommon.getLayoutMatrix(mirror, frameAspectRatio, layoutAspectRatio);
                if (frameAspectRatio > layoutAspectRatio) {
                    drawnFrameWidth = (int) (frame.rotatedHeight() * layoutAspectRatio);
                    drawnFrameHeight = frame.rotatedHeight();
                } else {
                    drawnFrameWidth = frame.rotatedWidth();
                    drawnFrameHeight = (int) (frame.rotatedWidth() / layoutAspectRatio);
                }
            } else {
                layoutMatrix =
                        mirror ? RendererCommon.horizontalFlipMatrix() : RendererCommon.identityMatrix();
                drawnFrameWidth = frame.rotatedWidth();
                drawnFrameHeight = frame.rotatedHeight();
            }
            drawMatrix = RendererCommon.multiplyMatrices(texMatrix, layoutMatrix);
        }

        GLES20.glClearColor(0 /* red */, 0 /* green */, 0 /* blue */, 0 /* alpha */);
        GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
        if (frame.dataType == VideoBaseRenderer.VideoDataType.VideoData_I420) {
            // Make sure YUV textures are allocated.
            if (yuvTextures == null) {
                yuvTextures = new int[3];
                for (int i = 0; i < 3; i++) {
                    yuvTextures[i] = GlUtil.generateTexture(GLES20.GL_TEXTURE_2D);
                }
            }

            yuvUploader.uploadYuvData(
                    yuvTextures, frame.width, frame.height, frame.yuvStrides, frame.yuvPlanes);
            //m3 noto 变形
            // drawer.drawYuv(yuvTextures, drawMatrix, drawnFrameWidth, drawnFrameHeight, 0, 0,
            //         eglBase.surfaceWidth(), eglBase.surfaceHeight());
            drawer.drawYuv(yuvTextures, drawMatrix, drawnFrameWidth, drawnFrameHeight, 0, 0,
            		viewWidth, viewHieght);
        } else if (frame.dataType == VideoBaseRenderer.VideoDataType.VideoData_TEXTURE) {
            drawer.drawRgb(frame.textureId, drawMatrix, drawnFrameWidth, drawnFrameHeight, 0, 0,
            		viewWidth, viewHieght);
        } else if (frame.dataType == VideoBaseRenderer.VideoDataType.VideoData_TEXTURE_OES) {
            drawer.drawOes(frame.textureId, drawMatrix, drawnFrameWidth, drawnFrameHeight, 0, 0,
            		viewWidth, viewHieght);
        }
        final long swapBuffersStartTimeNs = System.nanoTime();
        eglBase.swapBuffers();

        final long currentTimeNs = System.nanoTime();
        synchronized (statisticsLock) {
            ++framesRendered;
            renderTimeNs += (currentTimeNs - startTimeNs);
            renderSwapBufferTimeNs += (currentTimeNs - swapBuffersStartTimeNs);
        }

        notifyCallbacks(frame, texMatrix);
        VideoBaseRenderer.renderFrameDone(frame);
    }

    private void notifyCallbacks(VideoBaseRenderer.I420Frame frame, float[] texMatrix) {
        // Make temporary copy of callback list to avoid ConcurrentModificationException, in case
        // callbacks call addFramelistener or removeFrameListener.
        final ArrayList<FrameListenerAndParams> tmpList;
        if (frameListeners.isEmpty())
            return;
        tmpList = new ArrayList<FrameListenerAndParams>(frameListeners);
        frameListeners.clear();

        final float[] bitmapMatrix = RendererCommon.multiplyMatrices(
                RendererCommon.multiplyMatrices(texMatrix,
                        mirror ? RendererCommon.horizontalFlipMatrix() : RendererCommon.identityMatrix()),
                RendererCommon.verticalFlipMatrix());

        for (FrameListenerAndParams listenerAndParams : tmpList) {
            final int scaledWidth = (int) (listenerAndParams.scale * frame.rotatedWidth());
            final int scaledHeight = (int) (listenerAndParams.scale * frame.rotatedHeight());

            if (scaledWidth == 0 || scaledHeight == 0) {
                listenerAndParams.listener.onFrame(null);
                continue;
            }

            if (bitmapTextureFramebuffer == null) {
                bitmapTextureFramebuffer = new GlTextureFrameBuffer(GLES20.GL_RGBA);
            }
            bitmapTextureFramebuffer.setSize(scaledWidth, scaledHeight);

            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, bitmapTextureFramebuffer.getFrameBufferId());
            GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
                    GLES20.GL_TEXTURE_2D, bitmapTextureFramebuffer.getTextureId(), 0);

            if (frame.yuvFrame) {
                listenerAndParams.drawer.drawYuv(yuvTextures, bitmapMatrix, frame.rotatedWidth(),
                        frame.rotatedHeight(), 0 /* viewportX */, 0 /* viewportY */, scaledWidth, scaledHeight);
            } else {
                listenerAndParams.drawer.drawRgb(frame.textureId, bitmapMatrix, frame.rotatedWidth(),
                        frame.rotatedHeight(), 0 /* viewportX */, 0 /* viewportY */, scaledWidth, scaledHeight);
            }

            final ByteBuffer bitmapBuffer = ByteBuffer.allocateDirect(scaledWidth * scaledHeight * 4);
            GLES20.glViewport(0, 0, scaledWidth, scaledHeight);
            GLES20.glReadPixels(
                    0, 0, scaledWidth, scaledHeight, GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, bitmapBuffer);

            GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
            GlUtil.checkNoGLES2Error("EglRenderer.notifyCallbacks");

            final Bitmap bitmap = Bitmap.createBitmap(scaledWidth, scaledHeight, Bitmap.Config.ARGB_8888);
            bitmap.copyPixelsFromBuffer(bitmapBuffer);
            listenerAndParams.listener.onFrame(bitmap);
        }
    }

    private String averageTimeAsString(long sumTimeNs, int count) {
        return (count <= 0) ? "NA" : TimeUnit.NANOSECONDS.toMicros(sumTimeNs / count) + " μs";
    }

    private void logStatistics() {
        final long currentTimeNs = System.nanoTime();
        synchronized (statisticsLock) {
            final long elapsedTimeNs = currentTimeNs - statisticsStartTimeNs;
            if (elapsedTimeNs <= 0) {
                return;
            }
            final float renderFps = framesRendered * TimeUnit.SECONDS.toNanos(1) / (float) elapsedTimeNs;
            logD("Duration: " + TimeUnit.NANOSECONDS.toMillis(elapsedTimeNs) + " ms."
                    + " Frames received: " + framesReceived + "."
                    + " Dropped: " + framesDropped + "."
                    + " Rendered: " + framesRendered + "."
                    + " Render fps: " + String.format("%.1f", renderFps) + "."
                    + " Average render time: " + averageTimeAsString(renderTimeNs, framesRendered) + "."
                    + " Average swapBuffer time: "
                    + averageTimeAsString(renderSwapBufferTimeNs, framesRendered) + ".");
            resetStatistics(currentTimeNs);
        }
    }
    
    public void setRenderSize(int w, int h)
    {
    	this.renderWidth = w;
    	this.renderHeight = h;
    }
    
    public void setRenderPosition(int position)
    {
    	this.renderPosition = position;
    	if(drawer != null)
    	{
    		drawer.drawPosition(position);
    	}
    }
    

    private void logD(String string) {
        Log.d(TAG, name + string);
    }
    
}

