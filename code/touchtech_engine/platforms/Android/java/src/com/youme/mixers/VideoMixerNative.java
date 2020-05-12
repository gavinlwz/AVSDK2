package com.youme.mixers;

import android.os.Build;


/**
 * Created by bhb on 2018/1/15.
 */

public class VideoMixerNative {

    public static native void videoFrameMixerCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);

    public static native void videoNetFirstCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);

    public static native void videoNetSecondCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);

    public static native void videoFrameMixerDataCallback(int type, byte[] data, int width, int height, long timestmap);
    
    public static native void videoNetDataFirstCallback(int type, byte[] data,  int width, int height, long timestmap);
    
    public static native void videoNetDataSecondCallback(int type, byte[] data,  int width, int height, long timestmap);
    
    public static native int  videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror);
    
    public static native void videoMixerUseTextureOES(boolean use);

    public static native boolean isSupportGL3();
    
    public static native void glReadPixels(int x, int y, int width, int height, int format, int type, byte[] array);

    public static native int glMapBufferRange(int width, int height , int stride, int target, int offset, int length, int access, byte[] array);

 
}


