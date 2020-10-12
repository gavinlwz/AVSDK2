package com.youme.mixers;


import java.nio.ByteBuffer;

/**
 * Created by bhb on 2018/1/10.
 */

public interface IVideoMixerCallback {
    public void videoFrameMixerCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);
    public void videoNetFirstCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);
    public void videoNetSecondCallback(int type, int texture, float[] matrix, int width, int height, long timestmap);

    public void videoFrameMixerDataCallback(int type, byte[] data, int width, int height, long timestmap);
    public void videoNetDataFirstCallback(int type, byte[] data,  int width, int height, long timestmap);
    public void videoNetDataSecondCallback(int type, byte[] data,  int width, int height, long timestmap);

    public int  videoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror);
}
