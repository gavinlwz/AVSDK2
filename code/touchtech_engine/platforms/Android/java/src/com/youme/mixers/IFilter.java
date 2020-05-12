package com.youme.mixers;

import android.graphics.Rect;

import java.nio.FloatBuffer;

public interface IFilter {
    int getTextureTarget();

    int getFilterType();

    void setTextureSize(int width, int height);

    void setFrameBufferSize(int width, int height);

    void setDrawRect(Rect rect);

//    void onDraw(float[] mvpMatrix, FloatBuffer vertexBuffer, int firstVertex, int vertexCount,
//                int coordsPerVertex, int vertexStride, float[] texMatrix, FloatBuffer texBuffer,
//                int textureId, int texStride);

    int onDraw(int textureId, float[] texMatrix);

    int onDraw(int width, int height, byte[] data, int size, float[] texMatrix);

    void releaseProgram();

    void makeMatrix(float[] samplingMatrix, int rotation, int mirror);

    void setBeautyLevel(float level);
}
