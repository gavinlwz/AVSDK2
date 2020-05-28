package main.java.com.example.library.video;

public interface VideoFrameCallbackInterface {
    /**
     * 回调远端视频数据
     *
     * @param userId    用户id
     * @param data      一帧视频数据
     * @param len       视频数据的大小
     * @param width     宽
     * @param height    高
     * @param fmt       数据类型，参考YOUME_VIDEO_FMT
     * @param timestamp 时间戳
     * @return void
     */
    void onVideoFrameCallback(String userId, byte[] data, int len, int width, int height, int fmt, long timestamp);

    /**
     * 回调的合流后的视频数据
     *
     * @param data      一帧视频数据
     * @param len       视频数据的大小
     * @param width     宽
     * @param height    高
     * @param fmt       数据类型，参考YOUME_VIDEO_FMT
     * @param timestamp 时间戳
     * @return void
     */
    void onVideoFrameMixed(byte[] data, int len, int width, int height, int fmt, long timestamp);

    /**
     * 远端视频数据回调
     *
     * @param userId    用户id
     * @param type      数据类型，参考 YOUME_VIDEO_FMT
     * @param texture   纹理id
     * @param matrix    纹理坐标矩阵
     * @param width     纹理宽度
     * @param height    纹理高度
     * @param timestamp 时间戳
     * @return void
     */
    void onVideoFrameCallbackGLES(String userId, int type, int texture, float[] matrix, int width, int height, long timestamp);

    /**
     * 合流后数据回调
     *
     * @param userId    用户id
     * @param texture   纹理id
     * @param matrix    纹理坐标矩阵
     * @param width     纹理宽度
     * @param height    纹理高度
     * @param timestamp 时间戳
     * @return void
     */
    void onVideoFrameMixedGLES(int type, int texture, float[] matrix, int width, int height, long timestamp);

    /**
     * 自定义滤镜回调
     *
     * @param userId   用户id
     * @param texture  纹理id
     * @param width    纹理宽度
     * @param height   纹理高度
     * @param rotation 图像需要旋转度数（0-360），目前为0
     * @param mirror   图像水平镜像，1需要镜像，目前为0
     * @return void
     */
    int onVideoRenderFilterCallback(int texture, int width, int height, int rotation, int mirror);
}

