package com.youme.voiceengine;

public class YouMeEngineAudioMixerUtils 
{

	/**
	 *  功能描述: 设置AudioTrack的采样率
	 *  @param sampleRate AudioTrack的采样率
	 *  @return 错误码，详见YouMeConstDefine.h定义
	 */
	public static native int setAudioMixerTrackSamplerate(int sampleRate);

	/**
 	 *  功能描述: 设置AudioTrack的音量
 	 *  @param volume AudioTrack的音量，参数范围 0-500
 	 *  @return 错误码，详见YouMeConstDefine.h定义
 	 */
	public static native int setAudioMixerTrackVolume(int volume);

	/**
 	 *  功能描述: 设置inputAudioToMix的音量
 	 *  @param volume inputAudioToMix的音量，参数范围 0-500
 	 *  @return 错误码，详见YouMeConstDefine.h定义
 	 */
	public static native int setAudioMixerInputVolume(int volume);

	/**
	 *  功能描述: 提供混音主音轨，混音后的数据被写到data里
	 *  @param data 指向PCM数据的缓冲区
	 *  @param nSizeInByte 缓冲区中数据的大小，以Byte为单位
	 *  @param nChannelNum 声道数量
	 *  @param nSampleRateHz 采样率, 已Hz为单位
	 *  @param nBytesPerSample 一个声道里面每个采样的字节数，典型情况下，如果数据是整型，该值为2，如果是浮点数，该值为4
	 *  @param bFloat, true - 数据是浮点数， false - 数据是整型
	 *  @param timestamp 时间戳
	 *  @return true - 成功
	 *          false -失败
	 */
	public static native boolean pushAudioMixerTrack(byte[] data, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, boolean bFloat, long timestamp);
	
	/**
	 *  功能描述: 输入数据流，混音到主音轨里
	 *  @param indexId 混音数据流的索引值，不同索引的流将会混在一起
	 *  @param data 指向PCM数据的缓冲区
	 *  @param nSizeInByte 缓冲区中数据的大小，以Byte为单位
	 *  @param nChannelNum 声道数量
	 *  @param nSampleRateHz 采样率, 已Hz为单位
	 *  @param nBytesPerSample 一个声道里面每个采样的字节数，典型情况下，如果数据是整型，该值为2，如果是浮点数，该值为4
	 *  @param bFloat, true - 数据是浮点数， false - 数据是整型
	 *  @param timestamp 时间戳
	 *  @return true - 成功
	 *          false -失败
	 */
	public static native boolean inputAudioToMix(String indexId, byte[] data, int nSizeInByte, int nChannelNUm, int nSampleRate, int nBytesPerSample, boolean bFloat, long timestamp);
}

