package com.youme.engine.video;

/**
 * Created by fire on 2017/2/14.
 */

import java.nio.ByteBuffer;

/**
 * Java version of VideoSinkInterface.  In addition to allowing clients to
 * define their own rendering behavior (by passing in a Callbacks object), this
 * class also provides a createGui() method for creating a GUI-rendering window
 * on various platforms.
 */
public class VideoBaseRenderer {

    public enum VideoDataType{
        VideoData_I420,
        VideoData_TEXTURE,
        VideoData_TEXTURE_OES,
    }
    private Callbacks callbacks;
    /**
     * Java version of webrtc::VideoFrame. Frames are only constructed from native code and test
     * code.
     */
    public static class I420Frame {
        public final int width;
        public final int height;
        public final int[] yuvStrides;
        public ByteBuffer[] yuvPlanes;
        public final boolean yuvFrame;
        // Matrix that transforms standard coordinates to their proper sampling locations in
        // the texture. This transform compensates for any properties of the video source that
        // cause it to appear different from a normalized texture. This matrix does not take
        // |rotationDegree| into account.
        public float[] samplingMatrix;
        public int textureId;
        // Frame pointer in C++.
        //private long nativeFramePointer;

        // rotationDegree is the degree that the frame must be rotated clockwisely
        // to be rendered correctly.
        public int rotationDegree;;
        public VideoDataType dataType;

        public long timestamp;
        /**
         * Construct a frame of the given dimensions with the specified planar data.
         */
        public I420Frame(int width, int height, int rotationDegree, int[] yuvStrides,
                         ByteBuffer[] yuvPlanes /* , long nativeFramePointer */) {
            this.width = width;
            this.height = height;
            this.yuvStrides = yuvStrides;
            this.yuvPlanes = yuvPlanes;
            this.yuvFrame = true;
            this.rotationDegree = rotationDegree;
            /* this.nativeFramePointer = nativeFramePointer; */
            if (rotationDegree % 90 != 0) {
                throw new IllegalArgumentException("Rotation degree not multiple of 90: " + rotationDegree);
            }
            // The convention in WebRTC is that the first element in a ByteBuffer corresponds to the
            // top-left corner of the image, but in glTexImage2D() the first element corresponds to the
            // bottom-left corner. This discrepancy is corrected by setting a vertical flip as sampling
            // matrix.
            // clang-format off
            this.samplingMatrix = new float[] {
                    1,  0, 0, 0,
                    0, -1, 0, 0,
                    0,  0, 1, 0,
                    0,  1, 0, 1};
            // clang-format on
            this.dataType = VideoDataType.VideoData_I420;
        }

        public I420Frame(int width, int height, int rotationDegree, int textureId, float[] matrix, boolean isOES) {
            this.width = width;
            this.height = height;
            this.yuvStrides = null;
            this.yuvPlanes = null;
            this.textureId = textureId;
            this.yuvFrame = false;
            this.rotationDegree = rotationDegree;
            if (rotationDegree % 90 != 0) {
                throw new IllegalArgumentException("Rotation degree not multiple of 90: " + rotationDegree);
            }
            if(matrix != null)
            {
                this.samplingMatrix = new float[16];
                System.arraycopy(matrix, 0, samplingMatrix, 0, samplingMatrix.length);
            }
            else {
                this.samplingMatrix = new float[]{
                        1, 0, 0, 0,
                        0, 1, 0, 0,
                        0, 0, 1, 0,
                        0, 0, 0, 1};
            }
            this.dataType = isOES ? VideoDataType.VideoData_TEXTURE_OES : VideoDataType.VideoData_TEXTURE;
        }

        public int rotatedWidth() {
            return (rotationDegree % 180 == 0) ? width : height;
        }

        public int rotatedHeight() {
            return (rotationDegree % 180 == 0) ? height : width;
        }

        @Override
        public String toString() {
            return width + "x" + height + ":" + yuvStrides[0] + ":" + yuvStrides[1] + ":" + yuvStrides[2];
        }
    }

    // Helper native function to do a video frame plane copying.
    /***
    public static native void nativeCopyPlane(
            ByteBuffer src, int width, int height, int srcStride, ByteBuffer dst, int dstStride);

     **/

    /** The real meat of VideoSinkInterface. */
    public static interface Callbacks {
        // |frame| might have pending rotation and implementation of Callbacks
        // should handle that by applying rotation during rendering. The callee
        // is responsible for signaling when it is done with |frame| by calling
        // renderFrameDone(frame).
        public void renderFrame(I420Frame frame);
    }

    /**
     * This must be called after every renderFrame() to release the frame.
     */
    public static void renderFrameDone(I420Frame frame) {
        frame.yuvPlanes = null;
        frame.textureId = 0;
        //if (frame.nativeFramePointer != 0) {
            //releaseNativeFrame(frame.nativeFramePointer);
            //frame.nativeFramePointer = 0;
        //}
    }

    //long nativeVideoRenderer;

    public VideoBaseRenderer(Callbacks callbacks) {
        this.callbacks = callbacks;
        //nativeVideoRenderer = nativeWrapVideoRenderer(callbacks);
    }

    public void dispose() {
        //if (nativeVideoRenderer == 0) {
            // Already disposed.
            // return;
        //}

        // freeWrappedVideoRenderer(nativeVideoRenderer);
        // nativeVideoRenderer = 0;
    }

    //private static native long nativeWrapVideoRenderer(Callbacks callbacks);
    //private static native void freeWrappedVideoRenderer(long nativeVideoRenderer);
    //private static native void releaseNativeFrame(long nativeFramePointer);
}

