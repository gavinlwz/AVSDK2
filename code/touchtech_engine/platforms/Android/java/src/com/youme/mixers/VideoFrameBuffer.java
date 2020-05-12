package com.youme.mixers;

import android.graphics.SurfaceTexture;

import java.nio.FloatBuffer;

import com.youme.voiceengine.YouMeConst.YOUME_VIDEO_FMT;

/**
 * Created by bhb on 2018/1/10.
 */

public class VideoFrameBuffer {

    public final static float[] samplingMatrix = new float[]{
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    public  final static float[] yuvMatrix = new float[]{
    	    1,  0, 0, 0,
            0, -1, 0, 0,
            0,  0, 1, 0,
            0,  1, 0, 1};
    private int type; //参考YOUME_VIDEO_FMT
    private int texture = 0;
    private int width = 0;
    private int height = 0;
    private long timestmap;
    private int rotation = 0;
    private int mirror = 0;
    private byte[] dataBuffer = null;
    private int size = 0;
    private float[] matrix = new float[16];
    public String userId;
    public VideoFrameBuffer(int type, int texture, float[] matrix, int w, int h, int rotation, int mirror, long ts) {
        this.type = type;
        this.texture = texture;
        if (rotation %180 != 0) {
            this.width = h;
            this.height = w;
        } else {
            this.width = w;
            this.height = h;
        }
        this.timestmap = ts;
        this.rotation = rotation;
        this.mirror = mirror;
        if (matrix != null) {
            System.arraycopy(matrix, 0, this.matrix, 0, this.matrix.length);
        }
    }

    public VideoFrameBuffer(int type, byte[] data, int size, int w, int h, int rotation, int mirror, long ts) {
        if (data.length == 0)
            return;
        this.dataBuffer = new byte[data.length];
        System.arraycopy(data, 0, this.dataBuffer, 0, data.length);

        if (rotation %180 != 0) {
            this.width = h;
            this.height = w;
        } else {
            this.width = w;
            this.height = h;
        }
        this.type = type;
        this.timestmap = ts;
        this.rotation = rotation;
        this.mirror = mirror;
        this.size = size;
        if(type == YOUME_VIDEO_FMT.VIDEO_FMT_NV21 ||
        		type == YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P ||
        		type == YOUME_VIDEO_FMT.VIDEO_FMT_YV12)
            System.arraycopy(yuvMatrix, 0, this.matrix, 0, this.matrix.length);
        else
        	System.arraycopy(samplingMatrix, 0, this.matrix, 0, this.matrix.length);
    }

    public VideoFrameBuffer(int type, int texture, float[] matrix, int w, int h, long ts) {
        this.type = type;
        this.texture = texture;
        this.width = w;
        this.height = h;
        this.timestmap = ts;
        if (matrix != null) {
            System.arraycopy(matrix, 0, this.matrix, 0, this.matrix.length);
        }
    }

    public VideoFrameBuffer(int type, byte[] data, int size, int w, int h, long ts) {
        if (data.length == 0)
            return;
        this.dataBuffer = new byte[size];
        System.arraycopy(data, 0, this.dataBuffer, 0, size);
        this.width = w;
        this.height = h;
        this.timestmap = ts;
        this.size = size;
        this.type = type;
        if(type == YOUME_VIDEO_FMT.VIDEO_FMT_NV21 ||
        		type == YOUME_VIDEO_FMT.VIDEO_FMT_YUV420P ||
        		type == YOUME_VIDEO_FMT.VIDEO_FMT_YV12)
            System.arraycopy(yuvMatrix, 0, this.matrix, 0, this.matrix.length);
        else
        	System.arraycopy(samplingMatrix, 0, this.matrix, 0, this.matrix.length);
       
    }

    public int getType() {
        return type;
    }

    public int getTexture() {
        return texture;
    }

    public byte[] getData() {
        return dataBuffer;
    }

    public int getSize() {
        return size;
    }

    public int getWidth() {
//    	if(rotation %180 == 0)
//    		return width;
//    	else
//           return height;
    	return width;
    }

    public int getHeight() {
//    	if(rotation %180 != 0)
//    		return width;
//    	else
        return height;
    }

    public long getTimestamp() {
        return timestmap;
    }

    public int getRotation() {
        return rotation;
    }

    public int getMirror() {
        return mirror;
    }

    public void getTransformMatrix(float[] matrix) {
        if(this.matrix != null)
           System.arraycopy(this.matrix, 0, matrix, 0, matrix.length);
        else{
           System.arraycopy(samplingMatrix, 0, matrix, 0, matrix.length);
        }
    }
    
    public float[] getMatrix(){
    	return this.matrix;
    }

    public void setTexture(int texture){
        this.texture = texture;
    }
    
    public void setType(int type){
        this.type = type;
    }
    
    public void setRotation(int rotation){
    	this.rotation = rotation;
    }
    
    public void setMirror(int mirror){
    	this.mirror = mirror;
    }
    
    public void setMatrix(final float[] matrix){
    	System.arraycopy(matrix, 0, this.matrix, 0, this.matrix.length);
    }
}
