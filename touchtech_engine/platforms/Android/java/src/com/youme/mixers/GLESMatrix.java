package com.youme.mixers;

import android.graphics.Rect;
import android.opengl.Matrix;

import com.youme.voiceengine.video.RendererCommon;

/**
 * Created by bhb on 2018/1/25.
 */

public class GLESMatrix {

    public static void makeTexMatrix(float[] matrix, int texWidth, int texHeight, int bkWidth, int bkHeight, int rotation, int mirror){
        if (texWidth == 0 || texHeight == 0 || bkWidth == 0 || bkHeight == 0)
            return;
        final float[] layoutMatrix;
        final float[] texMatrix = RendererCommon.rotateTextureMatrix(matrix, rotation);
        final float layoutAspectRatio = bkWidth / (float) bkHeight;
        if (layoutAspectRatio > 0) {
            final float frameAspectRatio = texWidth / (float) texHeight;
            layoutMatrix = RendererCommon.getLayoutMatrix(mirror == 1, frameAspectRatio, layoutAspectRatio);
        } else {
            layoutMatrix = mirror == 1 ? RendererCommon.horizontalFlipMatrix() : RendererCommon.identityMatrix();
        }
        RendererCommon.multiplyMatrices(matrix, texMatrix, layoutMatrix);
    }

    public static void makeMvpMatrix(float[] matrix, int texWidth, int texHeight, int bkWidth, int bkHeight, Rect drawRect){
        if (texWidth == 0 || texHeight == 0 || bkWidth == 0 || bkHeight == 0)
            return;
        final float scalex, scaley, movex, movey;
        scalex = drawRect.width() / (float)bkWidth;
        scaley = drawRect.height() / (float)bkHeight;
        movex = (drawRect.centerX() - bkWidth/2.0f)/(float)bkWidth*2;
        movey = (drawRect.centerY() - bkHeight/2.0f)/(float)bkHeight*2*-1;
        Matrix.setIdentityM(matrix, 0);
        Matrix.translateM(matrix, 0, movex, movey, 0.0f);
        Matrix.scaleM(matrix, 0, scalex, scaley, 0.0f);
    }
}
