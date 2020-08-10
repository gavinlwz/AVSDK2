package com.youme.engine;

public interface YouMeVideoCallbackInterface {

	public void FrameRender(int sessionId, int width, int height, int rotationDegree, byte[] yuvframe);
	
	public void FrameRenderGLES(int sessionId, int width, int height, int type, int texture, float[] matrix);
}
