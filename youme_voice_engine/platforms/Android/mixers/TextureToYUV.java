package com.youme.mixers;

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.util.Log;

import com.youme.gles.GlUtil;

import java.nio.FloatBuffer;

/**
 * Created by bhb on 2018/3/16.
 */

public class TextureToYUV extends CameraFilter {

    private static final String sVertexShaderFilter = "uniform mat4 uMVPMatrix;  // MVP 的变换矩阵（整体变形）\n" +
            "uniform mat4 uTexMatrix;  // Texture 的变换矩阵 （只对texture变形）\n" +
            "\n" +
            "attribute vec4 aPosition;\n" +
            "attribute vec4 aTextureCoord;\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "\n" +
            "void main() {\n" +
            "    gl_Position = uMVPMatrix * aPosition;\n" +
            "    vTextureCoord = (uTexMatrix*aTextureCoord).xy;\n" +
            "}";
    private static final String sFragmentShaderFilterOES =
            "#extension GL_OES_EGL_image_external : require\n"
                    + "precision mediump float;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "\n"
                    + "uniform samplerExternalOES uTexture;\n"
                    + "uniform vec2 xUnit;\n"
                    + "uniform vec4 coeffs;\n"
                    + "\n"
                    + "void main() {\n"
                    + "  gl_FragColor.r = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord - 1.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.g = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord - 0.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.b = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord + 0.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.a = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord + 1.5 * xUnit).rgb);\n"
                    + "}\n";
    private static final String sFragmentShaderFilter = ""
                    + "precision mediump float;\n"
                    + "varying vec2 vTextureCoord;\n"
                    + "\n"
                    + "uniform sampler2D uTexture;\n"
                    + "uniform vec2 xUnit;\n"
                    + "uniform vec4 coeffs;\n"
                    + "\n"
                    + "void main() {\n"
                    + "  gl_FragColor.r = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord - 1.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.g = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord - 0.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.b = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord + 0.5 * xUnit).rgb);\n"
                    + "  gl_FragColor.a = coeffs.a + dot(coeffs.rgb,\n"
                    + "      texture2D(uTexture, vTextureCoord + 1.5 * xUnit).rgb);\n"
                    + "}\n";



    private int xUnitLoc;
    private int coeffsLoc;

    public TextureToYUV(int type, boolean useOES) {
        super(type, useOES);
    }

    @Override public int getTextureTarget() {
        return  mUseOES ?  GLES11Ext.GL_TEXTURE_EXTERNAL_OES :GLES20.GL_TEXTURE_2D;
    }

    @Override
    protected int createProgram(Context applicationContext) {
        return GlUtil.createProgram(sVertexShaderFilter, mUseOES ? sFragmentShaderFilterOES: sFragmentShaderFilter);
    }


    @Override
    protected void getGLSLValues() {
        super.getGLSLValues();

        xUnitLoc = GLES20.glGetUniformLocation(mProgramHandle, "xUnit");
        coeffsLoc = GLES20.glGetUniformLocation(mProgramHandle, "coeffs");

    }
    @Override
    public int onDraw(int textureId, float[] texMatrix) {
        GlUtil.checkGlError("draw start");
        try{
        useProgram();
        bindTexture(textureId);
        bindGLSLValues(mIdentityMatrix, mDrawable2d.getVertexArray(), mDrawable2d.getCoordsPerVertex(), mDrawable2d.getVertexStride(),
                texMatrix, mDrawable2d.getTexCoordArray(),
                mDrawable2d.getTexCoordStride());
        //drawArrays(0, mDrawable2d.getVertexCount());
        unbindGLSLValues();
        unbindTexture();
        disuseProgram();
        }catch (Exception e){
    		e.printStackTrace();
    		Log.e("CameraFilter", "onDraw eror:"+e.getMessage());
    		return -1;
    	}
        return 0;
    }
    @Override
    protected void bindGLSLValues(float[] mvpMatrix, FloatBuffer vertexBuffer, int coordsPerVertex,
                                  int vertexStride, float[] texMatrix, FloatBuffer texBuffer, int texStride) {
        super.bindGLSLValues(mvpMatrix, vertexBuffer, coordsPerVertex, vertexStride, texMatrix,
                texBuffer, texStride);

        int width = mFrameBufferWidth;
        int height = mFrameBufferHeight;
        int y_width = (width+3) / 4;
        int y_height = (height+3) / 4;
        int uv_width = (width+7) / 8;
        int uv_height = (height+1)/2;
        int stride = ((width + 7) / 8) * 8;
        // Draw Y
        GLES20.glViewport(0, 0, y_width, height);
        GLES20.glUniform2f(xUnitLoc,
                texMatrix[0] / width,
                texMatrix[1] / width);
        GLES20.glUniform4f(coeffsLoc, 0.299f, 0.587f, 0.114f, 0.0f);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        // Draw U
        GLES20.glViewport(0, height, uv_width, uv_height);
        GLES20.glUniform2f(xUnitLoc,
                2.0f * texMatrix[0] / width,
                2.0f * texMatrix[1] / width);
        GLES20.glUniform4f(coeffsLoc, -0.169f, -0.331f, 0.499f, 0.5f);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        // Draw V
        GLES20.glViewport(stride/8, height, uv_width, uv_height);
        GLES20.glUniform4f(coeffsLoc, 0.499f, -0.418f, -0.0813f, 0.5f);
        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        // Draw U
//        GLES20.glViewport(0, height, y_width, y_height);
//        GLES20.glUniform2f(xUnitLoc,
//                 2.0f*texMatrix[0] / width,
//                 2.0f*texMatrix[1] / width);
//        GLES20.glUniform4f(coeffsLoc, -0.169f, -0.331f, 0.499f, 0.5f);
//        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
//
//        // Draw V
//        GLES20.glViewport(0, height + y_height, y_width, y_height);
//        GLES20.glUniform4f(coeffsLoc, 0.499f, -0.418f, -0.0813f, 0.5f);
//        GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

        try{
        	 GlUtil.checkGlError("draw bindGLSLValues");
      	}catch (Exception e){
      		e.printStackTrace();
      		Log.e("CameraFilter", "drawArrays eror:"+e.getMessage());
      	}
       
    }

    @Override
    public void releaseProgram() {

        super.releaseProgram();
    }

}
