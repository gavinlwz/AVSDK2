package com.youme.voiceengine;
import com.youme.mixers.GLESVideoMixer;
import com.youme.mixers.VideoMixerFilterListener;
import com.youme.voiceengine.mgr.YouMeManager;
import android.content.Context;

import android.util.Log;

import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;

public class api
{
	public static final int YOUME_RTC_BGM_TO_SPEAKER  = 0; // Formal environment.
	public static final int YOUME_RTC_BGM_TO_MIC    = 1; // Using test servers.

	public static void SetCallback(YouMeCallBackInterface callBack)
	{
		//IYouMeChannelCallback.callBack = callBack;
		//IYouMeCommonCallback.callBack = callBack;
		IYouMeEventCallback.callBack = callBack;
	}

	public static void setPcmCallbackEnable (YouMeCallBackInterfacePcm callback, int flag, boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel)
	{
		Log.d("Api", "setPcmCallbackEnable");
		IYouMeEventCallback.mCallbackPcm = callback;
		if (callback == null) {
			NativeEngine.setPcmCallbackEnable(0 , false, nOutputSampleRate, nOutputChannel);
		} else {
			NativeEngine.setPcmCallbackEnable(flag, outputToSpeaker, nOutputSampleRate, nOutputChannel);
		}
	}

    public static void setScreenRotation(int rotation){
		CameraMgr.setRotation( rotation );
	}

	public static void screenRotationChange(){
		CameraMgr.rotationChange();
	}

    /**
     *  功能描述: 设置用户自定义数据的回调
     *  @param  callback:收到其它人自定义数据的回调地址，需要继承IYouMeCustomDataCallback并实现其中的回调函数
     *  @return void
     */
	public static void setRecvCustomDataCallback (YouMeCustomDataCallbackInterface callback)
	{
		IYouMeEventCallback.mCustomDataCallback = callback;
	}

    //设置合流视频数据回调
    public static void setVideoFrameCallback( VideoMgr.VideoFrameCallback  callback ){
		VideoMgr.setVideoFrameCallback( callback );
	}

	//设置音频回调
	public static void setAudioFrameCallback( YouMeAudioCallbackInterface callback  ){
		IYouMeAudioCallback.callback = callback;
	}
	
	/* for android */
	public static void SetVideoCallback() 
	{
		IYouMeVideoCallback.mVideoFrameRenderCallback = VideoRenderer.getInstance();
		setVideoCallback();
	}

    /**
     *  功能描述: 设置是否回调视频解码前H264数据，需要在加入房间之前设置
     *  @param  preDecodeCallback 回调方法
     *  @param  needDecodeandRender 是否需要解码并渲染:true 需要;false 不需要
     */
	public static void setVideoPreDecodeCallbackEnable(YouMeVideoPreDecodeCallbackInterface callback, boolean needDecodeandRender) 
	{
		IYouMeVideoCallback.mVideoPreDecodeCallback = callback;
		if (callback == null) {
			NativeEngine.setVideoPreDecodeCallbackEnable(false , needDecodeandRender);
		} else {
			NativeEngine.setVideoPreDecodeCallbackEnable(true, needDecodeandRender);
		}
	}
	
    /**
     * 功能描述:   设置是否由外部输入音视频
     * @param bInputModeEnabled: true:外部输入模式，false:SDK内部采集模式
     */
    public static native void setExternalInputMode( boolean bInputModeEnabled );

    /**
     *  功能描述:初始化引擎
     *
     *  @param strAPPKey:在申请SDK注册时得到的App Key，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     *  @param strAPPSecret:在申请SDK注册时得到的App Secret，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int init (String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);

	public static int initForAndroid( String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName, Context context )
	{
		YouMeManager.mContext = new WeakReference<Context>( context );

		return init( strAPPKey, strAPPSecret, serverRegionId, strExtServerRegionName );
	}

    /**
     *  功能描述:反初始化引擎
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int unInit ();

	public static int unInitForAndroid(){
		int ret = unInit();
		YouMeManager.mContext = null;

		return ret;
	}

    /**
     *  功能描述:设置身份验证的token
     *  @param strToken: 身份验证用token，设置为NULL或者空字符串，清空token值。
     *  @return 无
     */
    public static native void setToken( String strToken );
	
	/**
     *  功能描述:设置身份验证的token
     *  @param strToken: 身份验证用token，设置为NULL或者空字符串，清空token值。
	 *  @param timeStamp: 用户加入房间的时间，单位s。
     *  @return 无
     */
	public static native void setTokenV3( String strToken, long timeStamp);

	/**
	 *  功能描述:设置是否使用TCP模式来收发数据，针对特殊网络没有UDP端口使用，必须在加入房间之前调用
	 *  @param bUseTcp: 是否使用。
     *  @return 错误码，详见YouMeConstDefine.h定义
	 */
	public static native int setTCPMode( boolean bUseTcp );

    /**
     *  功能描述: 设置用户自定义Log路径
     *  @param filePath Log文件的路径
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setUserLogPath (String filePath);

    /**
     *  功能描述:切换语音输出设备
     *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
     *
     *  @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setOutputToSpeaker (boolean bOutputToSpeaker);

    /**
     *  功能描述:设置扬声器静音
     *
     *  @param bOn:true——静音，false——取消静音
     *  @return 无
     */
    public static native void setSpeakerMute (boolean bOn);

    /**
     *  功能描述:获取扬声器静音状态
     *
     *  @return true——静音，false——没有静音
     */
    public static native boolean getSpeakerMute ();

    /**
     *  功能描述:获取麦克风静音状态
     *
     *  @return true——静音，false——没有静音
     */
    public static native boolean getMicrophoneMute ();

    /**
     *  功能描述:设置麦克风静音
     *
     *  @param bOn:true——静音，false——取消静音
     *  @return 无
     */
    public static native void setMicrophoneMute (boolean mute);
    
    /**
     *  功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
     *
     *  @param bAutoSend:true——通知，false——不通知
     *  @return 无
     */
    public static native void setAutoSendStatus( boolean bAutoSend );
    

    /**
     *  功能描述:获取音量大小,此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量
     *
     *  @return 音量值[0,100]
     */
    public static native int getVolume ();

    /**
     *  功能描述:增加音量，音量数值加1
     *
     *  @return 无
     */
    public static native void setVolume (int uiVolume);

    /**
     *  功能描述:是否可使用移动网络
     *
     *  @return true-可以使用，false-禁用
     */
    public static native boolean getUseMobileNetworkEnabled ();

    /**
     *  功能描述:启用/禁用移动网络
     *
     *  @param bEnabled:true-可以启用，false-禁用，默认禁用
     *
     *  @return 无
     */
    public static native void setUseMobileNetworkEnabled (boolean bEnabled);

	/**
     *  功能描述:设置本地连接信息，用于p2p传输，本接口在join房间之前调用
     *
     *  @param pLocalIP:本端ip
     *  @param iLocalPort:本端数据端口
     *  @param pRemoteIP:远端ip
     *  @param iRemotePort:远端数据端口
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
	public static native int setLocalConnectionInfo(String pLocalIP, int iLocalPort, String pRemoteIP, int iRemotePort);

    /**
     *  功能描述:清除本地局域网连接信息，强制server转发
     *
     *
     *  @return 无
     */
    public static native void clearLocalConnectionInfo();
	
	/**
     *  功能描述:设置是否切换server通路
     *
     *  @param enable: 设置是否切换server通路标志
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
	public static native int setRouteChangeFlag(boolean enable);
	
    //---------------------多人语音接口---------------------//

    /**
     *  功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
     *
     *  @param strUserID: 用户ID，要保证全局唯一
     *  @param strRoomID: 频道ID，要保证全局唯一
     *  @param userRole: 用户角色，用于决定讲话/播放背景音乐等权限
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int joinChannelSingleMode (String strUserID, String strRoomID, int userRole, boolean autoRecv);

    /**
     *  功能描述：加入语音频道（多频道模式，可以同时在多个语音频道里面）
     *
     *  @param strUserID: 用户ID，要保证全局唯一
     *  @param strRoomID: 频道ID，要保证全局唯一
     *  @param userRole: 用户角色，用于决定讲话/播放背景音乐等权限
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int joinChannelMultiMode(String strUserID, String strRoomID, int userRole);

	/**
	 *  功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
	 *
	 *  @param strUserID: 用户ID，要保证全局唯一
	 *  @param strRoomID: 频道ID，要保证全局唯一
	 *  @param userRole: 用户角色，用于决定讲话/播放背景音乐等权限
	 *  @param strJoinAppKey: 加入房间用额外的appkey
	 *
	 *  @return 错误码，详见YouMeConstDefine.h定义
	 */
	public static native int joinChannelSingleModeWithAppKey (String strUserID, String strRoomID, int userRole, String strJoinAppKey, boolean autoRecv);
    
    /**
     *  功能描述：对指定频道说话
     *
     *  @param strRoomID: 频道ID，要保证全局唯一
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int speakToChannel(String strRoomID);
    
    
    /**
     *  功能描述:退出多频道模式下的某个语音频道
     *
     *  @param strChannelID:频道ID，要保证全局唯一
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int leaveChannelMultiMode (String strChannelID);
    /**
     *  功能描述:退出所有语音频道
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int leaveChannelAll ();
	public static  boolean isJoined() {
		return NativeEngine.isJoined();
	}

    public static native int  getChannelUserList( String strChannelID, int maxCount, boolean notifyMemChange );

    /**
     *  功能描述:控制其他人的麦克风开关
     *
     *  @param strUserID:用户ID，要保证全局唯一
     *  @param mute: true 静音对方的麦克风，false 取消静音对方麦克风
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setOtherMicMute (String strUserID, boolean status );

    /**
     *  功能描述:控制其他人的扬声器开关
     *
     *  @param strUserID:用户ID，要保证全局唯一
     *  @param mute: true 静音对方的扬声器，false 取消静音对方扬声器
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setOtherSpeakerMute (String strUserID, boolean status );

    /**
     *  功能描述:选择消除其他人的语音
     *
     *  @param strUserID:用户ID，要保证全局唯一
     *  @param on: false屏蔽对方语音，true取消屏蔽
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setListenOtherVoice (String strUserID, boolean on );
    
    public static native void setServerRegion (int region, String extServerName, boolean bAppend);
    

    /**
     *  功能描述: 播放背景音乐，并允许选择混合到扬声器输出麦克风输入
     *  @param filePath 音乐文件的路径
     *  @param bRepeat 是否重复播放
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int playBackgroundMusic (String filePath, boolean bRepeat);
   
    /**
     *  功能描述: 如果当前正在播放背景音乐的话，暂停播放
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int pauseBackgroundMusic ();

    /**
     *  功能描述: 如果当前背景音乐处于暂停状态的话，恢复播放
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int resumeBackgroundMusic ();

    /**
     *  功能描述: 如果当前正在播放背景音乐的话，停止播放
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int stopBackgroundMusic ();
    
    /**
     *  功能描述: 设置背景音乐播放的音量
     *  @param vol 背景音乐的音量，范围 0-100
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setBackgroundMusicVolume (int vol);
    
    /**
     *  功能描述: 设置是否用耳机监听自己的声音或背景音乐，当不插耳机或外部输入模式时，这个设置不起作用
     *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setHeadsetMonitorOn(boolean micEnabled);
    
    /**
     *  功能描述: 设置是否用耳机监听自己的声音或背景音乐，当不插耳机或外部输入模式时，这个设置不起作用
     *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
     *  @param bgmEnabled 是否监听背景音乐 true 监听，false 不监听 默认开启
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setHeadsetMonitorOn(boolean micEnabled, boolean bgmEnabled);
    
    /**
     *  功能描述: 设置是否开启主播混响模式
     *  @param enabled, true 开启，false 关闭
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setReverbEnabled (boolean enabled);

    /**
     *  功能描述: 暂停通话，释放麦克风等设备资源
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int pauseChannel();
    
    /**
     *  功能描述: 恢复通话
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int resumeChannel();
    
    /**
     *  功能描述: 设置是否开启语音检测回调
     *  @param enabled, true 开启，false 关闭
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setVadCallbackEnabled (boolean enabled);
    
    /**
     *  功能描述: 设置当前录音的时间戳
     *  @return  None
     */
    public static native void setRecordingTimeMs (int timeMs);
    
    /**
     *  功能描述: 设置当前播放的时间戳
     *  @return  None
     */
    public static native void setPlayingTimeMs (int timeMs);
    
    /**
     *  功能描述: 设置是否开启讲话音量回调, 并设置相应的参数
     *  @param maxLevel, 音量最大时对应的级别，最大可设100。根据实际需要设置小于100的值可以减少回调的次数。
     *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
     *                   设 0 表示关闭回调。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setMicLevelCallback(int maxLevel);
    
    /**
     *  功能描述: 设置是否开启远端语音音量回调, 并设置相应的参数
     *  @param maxLevel, 音量最大时对应的级别，最大可设100。
     *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
     *                   设 0 表示关闭回调。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setFarendVoiceLevelCallback(int maxLevel);
    
    /**
     *  功能描述: 设置当麦克风静音时，是否释放麦克风设备，在初始化之后、加入房间之前调用
     *  @param enabled,
     *      true 当麦克风静音时，释放麦克风设备，此时允许第三方模块使用麦克风设备录音。在Android上，语音通过媒体音轨，而不是通话音轨输出。
     *      false 不管麦克风是否静音，麦克风设备都会被占用。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setReleaseMicWhenMute(boolean enabled);
    
    /**
     *  功能描述: 设置插入耳机时，是否自动退出系统通话模式(禁用手机硬件提供的回声消除等信号前处理)
     *          系统提供的前处理效果包括回声消除、自动增益等，有助于抑制背景音乐等回声噪音，减少系统资源消耗
     *          由于插入耳机可从物理上阻断回声产生，故可设置禁用该效果以保留背景音乐的原生音质效果
     *          默认为false，在初始化之后、加入房间之前调用
 	 *          注：Windows和macOS不支持该接口
     *  @param enabled,
     *      true 当插入耳机时，自动禁用系统硬件信号前处理，拔出时还原
     *      false 插拔耳机不做处理
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static native int setExitCommModeWhenHeadsetPlugin(boolean enabled);

    public static native String getSdkInfo();
    public static native int createRender(String userId);
    public static native int deleteRender(int renderId);
	public static native int deleteRenderByUserID(String userId);
    
    private static native int setVideoCallback();



	/**
	 *  功能描述: 设置日志等级
     *  @param consoleLevel: 控制台日志等级, 有效值参看YOUME_LOG_LEVEL
     *  @param fileLevel: 文件日志等级, 有效值参看YOUME_LOG_LEVEL
	 */
	public static void  setLogLevel(  int consoleLevel, int fileLevel ) {
		NativeEngine.setLogLevel(consoleLevel, fileLevel);
	}
}
