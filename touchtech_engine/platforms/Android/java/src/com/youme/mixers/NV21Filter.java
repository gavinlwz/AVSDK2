package com.youme.mixers;

import java.nio.ByteBuffer;

import android.content.Context;
import android.opengl.GLES20;
import android.util.Log;

import com.youme.gles.GlUtil;

/**
 * Created by bhb on 2018/1/26.
 */

public class NV21Filter extends  CameraFilter {
	    private int mTextureLocUV;
	    private int mTextureY;
	    private int mTextureUV;
	    private int mWidth;
	    private int mHeight;
	    private ByteBuffer mBufferY;
	    private ByteBuffer mBufferUV;
	    
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

	   
	    private static final   String sFragmentShaderFilter= "\n" +
	            "precision mediump float; //指定默认精度\n" +
	            "\n" +
                 "varying vec2 vTextureCoord;\n"+ 
                 "uniform sampler2D uTexture;\n"+
	             "uniform sampler2D uTextureUV;\n"+
	             "void main (void){\n"+
	             "float r, g, b, y, u, v;\n"+
	             "y = texture2D(uTexture, vTextureCoord).r;\n"+
	             "u = texture2D(uTextureUV, vTextureCoord).a - 0.5;\n"+
	             "v = texture2D(uTextureUV, vTextureCoord).r - 0.5;\n"+
	             "r = y + 1.13983*v;\n"+
	             "g = y - 0.39465*u - 0.58060*v;\n"+
	             "b = y + 2.03211*u;\n"+
	             "gl_FragColor = vec4(r, g, b, 1.0);\n"+
                 "}";
    public NV21Filter(int type, boolean useOES) {
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
        mTextureLocUV = GLES20.glGetUniformLocation(mProgramHandle, "uTextureUV");
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
            mBufferY = ByteBuffer.allocateDirect(width*height);
            mBufferUV = ByteBuffer.allocateDirect(width*height/2);
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
     		Log.e("NV21Filter", "drawArrays eror:"+e.getMessage());
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
                GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE, nW, nH, 0, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, null);
                break;

            case 1:
            	nW = nW/2;
                nH = nH/2;
                mTextureUV = texture[0];
                GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
                GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureUV);
                GLES20.glTexImage2D(GLES20.GL_TEXTURE_2D, 0, GLES20.GL_LUMINANCE_ALPHA, nW, nH, 0, GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, null);
                break;

            default:
                break;
        }
      
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
        int uvLen = width * height / 2;
        System.arraycopy(data, 0, mBufferY.array(), 0, yLen);
        System.arraycopy(data, yLen, mBufferUV.array(), 0, uvLen);

        mBufferY.position(0);
        mBufferUV.position(0);
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureY);
        GLES20.glUniform1i(mTextureLoc, 0);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width, height, GLES20.GL_LUMINANCE, GLES20.GL_UNSIGNED_BYTE, mBufferY);

        GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
        GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, mTextureUV);
        GLES20.glUniform1i(mTextureLocUV, 1);
        GLES20.glTexSubImage2D(GLES20.GL_TEXTURE_2D, 0, 0, 0, width/2, height/2, GLES20.GL_LUMINANCE_ALPHA, GLES20.GL_UNSIGNED_BYTE, mBufferUV);

    }

    @Override
    public void releaseProgram() {
        releaseAllTexture();
        super.releaseProgram();
    }
    private void releaseAllTexture(){
        GLES20.glDeleteTextures(2, new int[]{mTextureY,mTextureUV}, 0);
    }

}
