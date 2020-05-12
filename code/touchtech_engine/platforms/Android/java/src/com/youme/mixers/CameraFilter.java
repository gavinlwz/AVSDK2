package com.youme.mixers;

import android.content.Context;
import android.graphics.Rect;
import android.nfc.Tag;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.Matrix;
import android.util.Log;

import java.io.IOException;
import java.nio.FloatBuffer;

import com.youme.gles.Drawable2d;
import com.youme.gles.GlUtil;
import com.youme.voiceengine.video.RendererCommon;


public class CameraFilter extends AbstractFilter implements IFilter {
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

    private static final String sFragmentShaderFilterOES = "#extension GL_OES_EGL_image_external : require\n" +
            "precision mediump float; //指定默认精度\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "uniform samplerExternalOES uTexture;\n" +
            "\n" +
            "void main() {\n" +
            "    gl_FragColor = texture2D(uTexture, vTextureCoord);\n" +
            "}";

    private static final String sFragmentShaderFilter = "\n"+
            "precision mediump float; //指定默认精度\n" +
            "\n" +
            "varying vec2 vTextureCoord;\n" +
            "uniform sampler2D uTexture;\n" +
            "\n" +
            "void main() {\n" +
            "    gl_FragColor = texture2D(uTexture, vTextureCoord);\n" +
            "}";
    protected int mProgramHandle;
    protected int maPositionLoc;
    protected int muMVPMatrixLoc;
    protected int muTexMatrixLoc;
    protected int maTextureCoordLoc;
    protected int mTextureLoc;
    protected Drawable2d mDrawable2d;

    protected int mTextureWidth, mTextureHeight;
    protected int mFrameBufferWidth, mFrameBufferHeight;
    protected Rect mDrawRect = new Rect();
    protected boolean mMatrixChange = false;
    public final float[] mIdentityMatrix = new float[16];
    protected boolean mUseOES = false;
    protected int mType;
    public CameraFilter(int type, boolean useOES) {
        mUseOES = useOES;
        mProgramHandle = createProgram(null);
        if (mProgramHandle == 0) {
            throw new RuntimeException("Unable to create program");
        }
        mDrawable2d = new Drawable2d();
        Matrix.setIdentityM(mIdentityMatrix, 0);
        getGLSLValues();
        mType = type;
    }

    @Override
    public int getFilterType(){
       return mType;
    }

    @Override
    public int getTextureTarget() {
        return  mUseOES ?  GLES11Ext.GL_TEXTURE_EXTERNAL_OES :GLES20.GL_TEXTURE_2D;
    }

    @Override
    public void setTextureSize(int width, int height) {
        if (mTextureWidth == width && mTextureHeight == height) {
            return;
        }
        mTextureWidth = width;
        mTextureHeight = height;
        mMatrixChange = true;
    }

    @Override
    public void setFrameBufferSize(int width, int height) {
        if (mFrameBufferWidth == width && mFrameBufferHeight == height) {
            return;
        }
        mFrameBufferWidth = width;
        mFrameBufferHeight = height;
        mMatrixChange = true;
    }

    @Override
    public void setDrawRect(Rect rect) {
        if (mDrawRect.left == rect.left &&
                mDrawRect.right == rect.right &&
                mDrawRect.top == rect.top &&
                mDrawRect.bottom == rect.bottom) {
            return;
        }
        mDrawRect.set(rect);
        mMatrixChange = true;
    }

    @Override
    protected int createProgram(Context applicationContext) {

        return GlUtil.createProgram(sVertexShaderFilter, mUseOES?sFragmentShaderFilterOES:sFragmentShaderFilter);
    }

    @Override
    protected void getGLSLValues() {
        mTextureLoc = GLES20.glGetUniformLocation(mProgramHandle, "uTexture");
        maPositionLoc = GLES20.glGetAttribLocation(mProgramHandle, "aPosition");
        muMVPMatrixLoc = GLES20.glGetUniformLocation(mProgramHandle, "uMVPMatrix");
        muTexMatrixLoc = GLES20.glGetUniformLocation(mProgramHandle, "uTexMatrix");
        maTextureCoordLoc = GLES20.glGetAttribLocation(mProgramHandle, "aTextureCoord");
    }

    /*
    @Override public void onDraw(float[] mvpMatrix, FloatBuffer vertexBuffer, int firstVertex,
            int vertexCount, int coordsPerVertex, int vertexStride, float[] texMatrix,
            FloatBuffer texBuffer, int textureId, int texStride) {

        GlUtil.checkGlError("draw start");

        useProgram();

        bindTexture(textureId);

        //runningOnDraw();

        bindGLSLValues(mvpMatrix, vertexBuffer, coordsPerVertex, vertexStride, texMatrix, texBuffer,
                texStride);

        drawArrays(firstVertex, vertexCount);

        unbindGLSLValues();

        unbindTexture();

        disuseProgram();
    }
    */
    @Override
    public int onDraw(int textureId, float[] texMatrix) {
        GlUtil.checkGlError("draw start");
        try{
        useProgram();
        bindTexture(textureId);
        bindGLSLValues(mIdentityMatrix, mDrawable2d.getVertexArray(), mDrawable2d.getCoordsPerVertex(), mDrawable2d.getVertexStride(),
                texMatrix, mDrawable2d.getTexCoordArray(),
                mDrawable2d.getTexCoordStride());
        drawArrays(0, mDrawable2d.getVertexCount());
        unbindGLSLValues();
        unbindTexture();
        disuseProgram();
        }catch (Exception e){
    		e.printStackTrace();
    		Log.e("CameraFilter", "drawArrays eror:"+e.getMessage());
    		return -1;
    		//throw new RuntimeException("drawArrays error:"+ e.getMessage());
    	}
        return 0;
    }

    @Override
    public int onDraw(int width, int height, byte[] data, int size, float[] texMatrix){
        return 0;
    }

    @Override
    protected void useProgram() {
        GLES20.glUseProgram(mProgramHandle);
    }

    @Override
    protected void bindTexture(int textureId) {
        GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
        GLES20.glBindTexture(getTextureTarget(), textureId);
        GLES20.glUniform1i(mTextureLoc, 0);
    }

    @Override
    protected void bindGLSLValues(float[] mvpMatrix, FloatBuffer vertexBuffer, int coordsPerVertex,
                                  int vertexStride, float[] texMatrix, FloatBuffer texBuffer, int texStride) {

        GLES20.glUniformMatrix4fv(muMVPMatrixLoc, 1, false, mvpMatrix, 0);
        GLES20.glUniformMatrix4fv(muTexMatrixLoc, 1, false, texMatrix, 0);
        GLES20.glEnableVertexAttribArray(maPositionLoc);
        GLES20.glVertexAttribPointer(maPositionLoc, coordsPerVertex, GLES20.GL_FLOAT, false,
                vertexStride, vertexBuffer);
        GLES20.glEnableVertexAttribArray(maTextureCoordLoc);
        GLES20.glVertexAttribPointer(maTextureCoordLoc, 2, GLES20.GL_FLOAT, false, texStride,
                texBuffer);
    }

    @Override
    protected void drawArrays(int firstVertex, int vertexCount) {
        //GLES20.glClearColor(0f, 0f, 0f, 1f);
        //GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
    	
          GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, firstVertex, vertexCount);
          GlUtil.checkGlError("drawArrays");
    	

    }

    @Override
    protected void unbindGLSLValues() {
        GLES20.glDisableVertexAttribArray(maPositionLoc);
        GLES20.glDisableVertexAttribArray(maTextureCoordLoc);
    }

    @Override
    protected void unbindTexture() {
        GLES20.glBindTexture(getTextureTarget(), 0);
    }

    @Override
    protected void disuseProgram() {
        GLES20.glUseProgram(0);
    }

    @Override
    public void releaseProgram() {
        GLES20.glDeleteProgram(mProgramHandle);
        mProgramHandle = -1;
    }

    @Override
    public void makeMatrix(float[] samplingMatrix, int rotation, int mirror) {

         GLESMatrix.makeTexMatrix(samplingMatrix, mTextureWidth, mTextureHeight,
                 mFrameBufferWidth, mFrameBufferHeight,
                 rotation,mirror);

         Matrix.setIdentityM(mIdentityMatrix, 0);
         GLESMatrix.makeMvpMatrix(mIdentityMatrix, mTextureWidth, mTextureHeight,
                 mFrameBufferWidth, mFrameBufferHeight,
                 mDrawRect);

    }

    @Override
    public void setBeautyLevel(float level){

    }

}
