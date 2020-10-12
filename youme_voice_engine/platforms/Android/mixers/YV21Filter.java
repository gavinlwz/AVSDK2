package com.youme.mixers;

import android.content.Context;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.util.Log;

import com.youme.gles.GlUtil;

import java.nio.ByteBuffer;

/**
 * Created by bhb on 2018/1/11.
 */

public class YV21Filter extends CameraFilter {

    private int mTextureLocU;
    private int mTextureLocV;
    private int mTextureY;
    private int mTextureU;
    private int mTextureV;
    private int mWidth;
    private int mHeight;
    private ByteBuffer mBufferY;
    private ByteBuffer mBufferU;
    private ByteBuffer mBufferV;
    private static final   String sVertexShaderFilter = "uniform mat4 uMVPMatrix;  // MVP 的变换矩阵（整体变形）\n" +
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

    private static final   String sFragmentShaderFilter = "\n" +
            "precision mediump float; //指定默认精度\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "uniform sampler2D uTexture;\n" +
            "uniform sampler2D uTextureU;\n" +
            "uniform sampler2D uTextureV;\n" +
            "\n" +
            "void main() {\n" +
            "mediump vec3 yuv;\n"+
            "lowp vec3 rgb;\n"+
            "yuv.x = texture2D(uTexture, vTextureCoord).r;\n"+
            "yuv.y = texture2D(uTextureU, vTextureCoord).r - 0.5;\n"+
            "yuv.z = texture2D(uTextureV, vTextureCoord).r - 0.5;\n"+
            "rgb = mat3(1, 1, 1, 0, -0.39465, 2.03211, 1.13983, -0.58060, 0) * yuv;\n"+
            "gl_FragColor = vec4(rgb, 1);\n"+
            "}";

    public YV21Filter(int type, boolean useOES) {
        super(type, useOES);
    }

    @Override public int getTextureTarget() {
        return GLES20.GL_TEXTURE_2D;
    }

    @Override
    protected int createProgram(Context applicationContext) {
        return GlUtil.createProgram(sVertexShaderFilter, sFragmentShaderFilter);
    }


    @Override
    protected void getGLSLValues() {
        super.getGLSLValues();
        mTextureLocU = GLES20.glGetUniformLocation(mProgramHandle, "uTextureU");
        mTextureLocV = GLES20.glGetUniformLocation(mProgramHandle, "uTextureV");
    }

    @Override
    public int onDraw(int width, int height, byte[] data, int size, float[] texMatrix){

        GlUtil.checkGlError("draw start");
        if (data == null || width*height*3/2 != size){
            return -1;
        }
        try{
        if (width != mWidth || height != mHeight){
            releaseAllTexture();
            CreateTexture(0, width, height);
            CreateTexture(1, width, height);
            CreateTexture(2, width, height);
            mBufferY = ByteBuffer.allocateDirect(width*height);
            mBufferU = ByteBuffer.allocateDirect(width*height/4);
            mBufferV = ByteBuffer.allocateDirect(width*height/4);
            mWidth = width;
            mHeight = height;
        }
        useProgram();

        uploadData(width, height, data, size);

        bindGLSLValues(mIdentityMatrix, mDrawable2d.getVertexArray(), mDrawable2d.getCoordsPerVertex(), mDrawable2d.getVertexStride(),
                texMatrix, mDrawable2d.getTexCoordArray(),
                mDrawable2d.getTexCoordStride());

        drawArrays(0, mDrawable2d.getVertexCount());

        unbindGLSLValues();

        unbindTexture();

        disuseProgram();
     	}catch (Exception e){
     		e.printStackTrace();
     		Log.e("YV21Filter", "drawArrays eror:"+e.getMessage());
     		return -1;
     	};
        return 0;
    }

    protected void CreateTexture(int nType, int nW, int nH)
    {
        int[] texture = new int[1];
        GLES20.glGenTextures(1, texture, 0);
        switch (nType)
        {
            case 0:
                mTextureY = texture[0];
                GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureY);
                break;

            case 1:
                mTextureU = texture[0];
                GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureU);
                nW = nW/2;
                nH = nH/2;
                break;

            case 2:
                mTextureV = texture[0];
                GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureV);
                nW = nW/2;
                nH = nH/2;
                break;

            default:
                break;
        }
        GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, nW, nH, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, null);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);
    }

    protected void uploadData(int width, int height, byte[] data, int size){

        int yLen = width * height;
        int uLen = width * height / 4;
        int vLen = width * height / 4;

        System.arraycopy(data, 0, mBufferY.array(), 0, yLen);
        System.arraycopy(data, yLen, mBufferU.array(), 0, uLen);
        System.arraycopy(data, (yLen + uLen), mBufferV.array(), 0, vLen);
        mBufferY.position(0);
        mBufferU.position(0);
        mBufferV.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureY);
        GLES20.glUniform1i(mTextureLoc, 0);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width, height, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mBufferY);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureU);
        GLES20.glUniform1i(mTextureLocU, 1);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mBufferU);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE2);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureV);
        GLES20.glUniform1i(mTextureLocV, 2);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mBufferV);

    }

    @Override
    public void releaseProgram() {
        releaseAllTexture();
        super.releaseProgram();
    }
    private void releaseAllTexture(){
        GLES20.glDeleteTextures(3, new int[]{mTextureY,mTextureU,mTextureV}, 0);
    }
}
