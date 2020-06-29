package com.youme.mixers;

import android.graphics.Rect;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.util.Log;

import com.youme.gles.GlUtil;
import com.youme.voiceengine.YouMeConst.YOUME_VIDEO_FMT;

/**
 * Created by bhb on 2018/1/25.
 */

public class Fbo {

    private int mWidth = 0;
    private int mHeight = 0;
    private int mFrameBuffer = 0;
    private int mRenderBuffer = 0;
    private int mFrameBufferTexture = 0;
    private IFilter mFilter;
    public Fbo(){

    }

    public int getTextureTarget() {
        return GLES20.GL_TEXTURE_2D;
    }

    public int getTexture(){
        return mFrameBufferTexture;
    }

    public int getFrameBuffer(){
        return mFrameBuffer;
    }

    public int getRenderBuffer(){
        return mRenderBuffer;
    }

    public boolean initFBO(int width, int height) {
        if (width == 0 || height == 0)
            return false;

        if (width == mWidth && height == mHeight)
            return true;
        uninitFBO();
        int [] texturebuffers = new int[1];
        int [] framebuffers = new int[1];
        int [] renderbuffers = new int[1];
        ///////////////// create FrameBufferTextures
        GLES20.glGenTextures(1, texturebuffers, 0);
        GlUtil.checkGlError("glGenTextures");

        mFrameBufferTexture = texturebuffers[0];
        GLES20.glBindTexture(getTextureTarget(), mFrameBufferTexture);
        GlUtil.checkGlError("glBindTexture " + mFrameBufferTexture);

        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_MAG_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_MIN_FILTER,
                GLES20.GL_LINEAR);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_WRAP_S,
                GLES20.GL_CLAMP_TO_EDGE);
        GLES20.glTexParameterf(getTextureTarget(), GLES20.GL_TEXTURE_WRAP_T,
                GLES20.GL_CLAMP_TO_EDGE);

        if (getTextureTarget() == GLES11Ext.GL_TEXTURE_EXTERNAL_OES) {
            GLES11Ext.glEGLImageTargetTexture2DOES(getTextureTarget(), null);
        } else {
            GLES20.glTexImage2D(/*GLES20.GL_TEXTURE_2D*/ getTextureTarget(), 0, GLES20.GL_RGBA, width, height, 0,
                    GLES20.GL_RGBA, GLES20.GL_UNSIGNED_BYTE, null);
        }

        GlUtil.checkGlError("glTexImage2D");

        ////////////////////////// create FrameBuffer
        GLES20.glGenFramebuffers(1, framebuffers, 0);
        GlUtil.checkGlError("glGenFramebuffers");
        mFrameBuffer = framebuffers[0];
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER,mFrameBuffer);
        GlUtil.checkGlError("glBindFramebuffer " + mFrameBuffer);

        ////////////////////////// create DepthBuffer
        GLES20.glGenRenderbuffers(1, renderbuffers, 0);
        GlUtil.checkGlError("glRenderbuffers");
        mRenderBuffer = renderbuffers[0];
        GLES20.glBindRenderbuffer(GLES20.GL_RENDERBUFFER, mRenderBuffer);
        GlUtil.checkGlError("glBindRenderbuffer");

        GLES20.glRenderbufferStorage(GLES20.GL_RENDERBUFFER, GLES20.GL_DEPTH_COMPONENT16, width,
                height);
        GlUtil.checkGlError("glRenderbufferStorage");
        /////////////

        GLES20.glFramebufferRenderbuffer(GLES20.GL_FRAMEBUFFER, GLES20.GL_DEPTH_ATTACHMENT,
                GLES20.GL_RENDERBUFFER, mRenderBuffer);
        GlUtil.checkGlError("glFramebufferRenderbuffer");

        GLES20.glFramebufferTexture2D(GLES20.GL_FRAMEBUFFER, GLES20.GL_COLOR_ATTACHMENT0,
                    /*GLES20.GL_TEXTURE_2D*/ getTextureTarget(), mFrameBufferTexture, 0);

        GlUtil.checkGlError("glFramebufferTexture2D");

        int status = GLES20.glCheckFramebufferStatus(GLES20.GL_FRAMEBUFFER);
        if (status != GLES20.GL_FRAMEBUFFER_COMPLETE) {
           // throw new RuntimeException("Framebuffer not complete, status=" + status);
            Log.d("fbo", "Framebuffer not complete, status=" + status);
            return false;
        }

        // Switch back to the default framebuffer.
        GLES20.glBindTexture(getTextureTarget(), 0);
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        GlUtil.checkGlError("prepareFramebuffer done");
        mWidth  = width;
        mHeight = height;
        return  true;
    }

    public void uninitFBO() {
        if(mFrameBufferTexture != 0){
            GLES20.glDeleteTextures(1, new int[]{mFrameBufferTexture}, 0);
            mFrameBufferTexture = 0;
        }
        if(mFrameBuffer != 0) {
            GLES20.glDeleteFramebuffers(1, new int[]{mFrameBuffer}, 0);
            mFrameBuffer = 0;
        }
        if(mRenderBuffer != 0) {
            GLES20.glDeleteRenderbuffers(1, new int[]{mRenderBuffer}, 0);
            mRenderBuffer = 0;
        }
        mWidth = 0;
        mHeight = 0;
    }

  
    public int drawTexture(int texId, int width, int height, int rotation, int mirror){
    	
    	GlUtil.checkGlError("drawTexture start");
    	if(!initFBO(width, height))
    		return 0;
    	
    	float[] matrix = new float[16];
    	System.arraycopy(VideoFrameBuffer.samplingMatrix, 0, matrix, 0, matrix.length);
    	if(mFilter == null)
    		mFilter = new CameraFilter(0, false);
    	 GLES20.glViewport(0, 0, width, height);
         GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
         GLES20.glClear(GLES20.GL_COLOR_BUFFER_BIT);
         GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, getFrameBuffer());
         mFilter.setDrawRect(new Rect(0, 0, width, height));
         mFilter.setTextureSize(width, height);
         mFilter.setFrameBufferSize(width, height);
         mFilter.makeMatrix(matrix, rotation, mirror);
         mFilter.onDraw(texId, matrix);

        //GLES20.glFlush();
        GLES20.glBindFramebuffer(GLES20.GL_FRAMEBUFFER, 0);
        
        try{
        	GlUtil.checkGlError("drawTexture end");
     	}catch (Exception e){
     		e.printStackTrace();
     		Log.e("FBO", "drawTexture eror:"+e.getMessage());
     	};
        return getTexture();
    	
    }

}
