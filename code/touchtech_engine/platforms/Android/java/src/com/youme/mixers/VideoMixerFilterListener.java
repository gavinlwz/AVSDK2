package com.youme.mixers;

public interface VideoMixerFilterListener {
	
	/**
	 * 自定义渲染
	 * @param texId   纹理ID
	 * @param texWidth   纹理宽度
	 * @param texHeight  纹理高度
	 * @return 返回新的纹理ID，0表示不处理
	 */
    int onDrawTexture(int texId, int texWidth, int texHeight);
    
    /**
	 * 自定义渲染
	 * @param texId   纹理ID
	 * @param texWidth   纹理宽度
	 * @param texHeight  纹理高度
	 * @return 返回新的纹理ID，0表示不处理
	 */
    //int onDrawTextureOES(int texId, int texWidth, int texHeight);
}
