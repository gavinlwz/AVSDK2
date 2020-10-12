package com.youme.mixers;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.opengl.GLES30;
import android.os.Build;
import android.util.Log;

import com.youme.gles.GlUtil;

import java.nio.IntBuffer;

/**
 * Created by bhb on 2018/3/15.
 */
@SuppressLint("NewApi") // This class is an implementation detail of the Java PeerConnection API.
@TargetApi(18)
public class Pbo {
    private final static  String TAG = "PBO";
    private final int pboAlign = 128;   //8字节对齐
    private final int mPixelStride = 4;  //rgba
    private IntBuffer mPboIds;
    private int mRowStride;
    private int mPboSize;
    private int mPboIndex = 0;
    private int mPboNewIndex = 1;
    private int mInputWidth;
    private int mInputHeight;
    private byte[][] mOutputBuffer;

    private boolean mInitPbo = false;


    private static boolean loadGlES3 = false;
    public static void loadGLESLibrary(){
    	 if(!loadGlES3){
    	   System.loadLibrary("GLESv3");
    	   loadGlES3 = true;
    	 }
    }

    public Pbo(){

    }

    public static boolean isSupportPBO() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2)
            return VideoMixerNative.isSupportGL3();
        return  false;
    }
    public static void checkGlError(String op) {
        int error = GLES30.glGetError();
        if (error != GLES30.GL_NO_ERROR) {
            String msg = op + ": glError 0x" + Integer.toHexString(error);
            Log.e(TAG, msg);
            throw new RuntimeException(msg);
        }
    }
    public  void initPBO(int width, int height){

        GlUtil.checkGlError("initPBO start");
        mRowStride = (width*mPixelStride  + (pboAlign - 1)) & ~(pboAlign - 1);
        mPboSize = mRowStride*height;
        mPboIds = IntBuffer.allocate(2);
        mOutputBuffer = new byte[2][width*height*4];
        if(!isSupportPBO())
            return;
        Pbo.loadGLESLibrary();
        GLES30.glGenBuffers(2, mPboIds);
        GlUtil.checkGlError("glGenBuffers");
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPboIds.get(0));
        checkGlError("glBindBuffer");
        GLES30.glBufferData(GLES30.GL_PIXEL_PACK_BUFFER, mPboSize, null,GLES30.GL_STATIC_READ);
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPboIds.get(1));
        GLES30.glBufferData(GLES30.GL_PIXEL_PACK_BUFFER, mPboSize, null,GLES30.GL_STATIC_READ);
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
        GlUtil.checkGlError("initPBO end");
        mInitPbo = true;
        Log.d(TAG,"initPBO pbo1:"+mPboIds.get(0)+" pbo2:"+mPboIds.get(1)+" mRowStride:"+mRowStride );
    }

    public void unInitPBO(){
        if(mPboIds != null){
            GLES30.glDeleteBuffers(2,  mPboIds);
            mInputWidth = 0;
            mInputHeight = 0;
            mPboIds = null;
        }
        if(mOutputBuffer != null)
            mOutputBuffer = null;
        mInitPbo = false;
    }

    public byte[] bindPixelBuffer(int width, int height) {

        boolean initRecord = false;
        try {
            if (width != mInputWidth || height != mInputHeight) {
                mInputWidth = width;
                mInputHeight = height;
                mPboIndex = 0;
                mPboNewIndex = 1;
                unInitPBO();
                initPBO(width, height);
                initRecord = true;
            }
        }catch (Exception e){
            e.printStackTrace();
        }
        if (mInitPbo) {
            //long tm1 = System.currentTimeMillis();
            GlUtil.checkGlError("bindPixelBuffer start");
            GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPboIds.get(mPboIndex));
            VideoMixerNative.glReadPixels(0, 0, mRowStride / mPixelStride, mInputHeight, GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, null);
            //long tm2 = System.currentTimeMillis();
            GlUtil.checkGlError("bindPixelBuffer glReadPixels");
            //第一帧没有数据跳出
            if (initRecord) {
                unbindPixelBuffer();
                return null;
            }
            GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, mPboIds.get(mPboNewIndex));
            byte[] buff = mOutputBuffer[mPboNewIndex];
            int result = VideoMixerNative.glMapBufferRange(mInputWidth, mInputHeight, mRowStride, GLES30.GL_PIXEL_PACK_BUFFER, 0, mPboSize, GLES30.GL_MAP_READ_BIT, buff);
            if (result != 0) {
                Log.w("pbo", "opengl glMapBufferRange fail result:"+result);
                return null;
            }
            //long tm3 = System.currentTimeMillis();
            GLES30.glUnmapBuffer(GLES30.GL_PIXEL_PACK_BUFFER);
            unbindPixelBuffer();
            //long tm4 = System.currentTimeMillis();
            //Log.d(TAG, "bindPixelBuffer tm1:" + (tm2 - tm1) + " tm2:" + (tm3 - tm2) + " tm3:" +
            //    (tm4 - tm3));
            return buff;
        }else{
            //long tm1 = System.currentTimeMillis();
        	VideoMixerNative.glReadPixels(0, 0, width, height, GLES30.GL_RGBA, GLES30.GL_UNSIGNED_BYTE, mOutputBuffer[0]);
            //long tm2 = System.currentTimeMillis();
            //Log.d(TAG, "bindPixelBuffer tm1:" + (tm2 - tm1));
            return mOutputBuffer[0];
        }


    }

    //解绑pbo
    private void unbindPixelBuffer() {
        //解除绑定PBO
        GLES30.glBindBuffer(GLES30.GL_PIXEL_PACK_BUFFER, 0);
        //交换索引
        mPboIndex = (mPboIndex + 1) % 2;
        mPboNewIndex = (mPboNewIndex + 1) % 2;
    }

}
