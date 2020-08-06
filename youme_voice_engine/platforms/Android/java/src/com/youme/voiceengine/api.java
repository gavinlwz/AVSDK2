package com.youme.voiceengine;

import com.youme.mixers.GLESVideoMixer;
import com.youme.mixers.VideoMixerFilterListener;
import com.youme.voiceengine.mgr.LogUtil;

import android.app.Activity;
import android.content.Context;

import android.util.Log;

import java.lang.ref.WeakReference;

public class api {
    public static String TAG = "api";
    public static final int YOUME_RTC_BGM_TO_SPEAKER = 0; // Formal environment.
    public static final int YOUME_RTC_BGM_TO_MIC = 1; // Using test servers.

	public  static WeakReference<Context> mContext = null;
	public static Boolean mInited = false;
	public static String mCachePath;

    public static boolean LoadSO() {
        try {
            System.loadLibrary("youme_voice_engine");
        } catch (Throwable e) {
            e.printStackTrace();
            return false;
        }

        return true;
    }

    //加载文件
    public static boolean Init(Context context) {
        Log.i(TAG, "调用init 函数 开始，目录:");
        if (context == null) {
            Log.e(TAG, "context can not be null");
            return false;
        }
        if (mInited) {
            Log.e(TAG, "Init: Already initialzed");
            if (context instanceof Activity) {
                if (mContext != null) {
                    mContext.clear();
                }
                mContext = new WeakReference<Context>(context);
                CameraMgr.init(context);
                AudioMgr.init(context);
            }
            return true;
        }

        if (mContext != null) {
            mContext.clear();
        }
        mContext = new WeakReference<Context>(context);
        mCachePath = context.getDir("libs", Context.MODE_PRIVATE).getAbsolutePath();
        Log.i(TAG, "调用init 函数 开始，目录:" + mCachePath);

        if (!LoadSO()) {
            Log.e(TAG, "Failed to load so");
            return false;
        }

        try {
            AppPara.initPara(context);
            AudioMgr.init(context);
            CameraMgr.init(context);
        } catch (Throwable e) {
            e.printStackTrace();
            return false;
        }

        Log.i(TAG, "调用init 函数 结束");
        mInited = true;
        return true;
    }

    //反初始化
    public static void Uninit() {
        try {
            AudioMgr.uinit();
            mInited = false;
        } catch (Throwable e) {
            e.printStackTrace();
        }
    }

    //保存logcat 日志
    public static void SaveLogcat(String strPath) {
        LogUtil.SaveLogcat(strPath);
    }

//	public static void SetCallback(YouMeCallBackInterface callBack)
//	{
//		//IYouMeChannelCallback.callBack = callBack;
//		//IYouMeCommonCallback.callBack = callBack;
//		IYouMeEventCallback.callBack = callBack;
//	}

//	public static void setPcmCallbackEnable (YouMeCallBackInterfacePcm callback, int flag, boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel)
//	{
//		Log.d("Api", "setPcmCallbackEnable");
//		IYouMeEventCallback.mCallbackPcm = callback;
//		if (callback == null) {
//			NativeEngine.setPcmCallbackEnable(0 , false, nOutputSampleRate, nOutputChannel);
//		} else {
//			NativeEngine.setPcmCallbackEnable(flag, outputToSpeaker, nOutputSampleRate, nOutputChannel);
//		}
//	}

//    public static void setScreenRotation(int rotation){
//		CameraMgr.setRotation( rotation );
//	}

//	public static void screenRotationChange(){
//		CameraMgr.rotationChange();
//	}

    /**
     * 功能描述: 设置用户自定义数据的回调
     *
     * @param callback:收到其它人自定义数据的回调地址，需要继承IYouMeCustomDataCallback并实现其中的回调函数
     * @return void
     */
//	public static void setRecvCustomDataCallback (YouMeCustomDataCallbackInterface callback)
//	{
//		IYouMeEventCallback.mCustomDataCallback = callback;
//	}

    //设置合流视频数据回调
    public static void setVideoFrameCallback(VideoMgr.VideoFrameCallback callback) {
        VideoMgr.setVideoFrameCallback(callback);
    }

    //设置音频回调
//	public static void setAudioFrameCallback( YouMeAudioCallbackInterface callback  ){
//		IYouMeAudioCallback.callback = callback;
//	}

    public static void SetVideoCallback() {
        IYouMeVideoCallback.mVideoFrameRenderCallback = VideoRenderer.getInstance();
        setVideoCallback();
    }

    /**
     *  功能描述: 设置是否回调视频解码前H264数据，需要在加入房间之前设置
     *  @param  preDecodeCallback 回调方法
     *  @param  needDecodeandRender 是否需要解码并渲染:true 需要;false 不需要
     */
//	public static void setVideoPreDecodeCallbackEnable(YouMeVideoPreDecodeCallbackInterface callback, boolean needDecodeandRender)
//	{
//		IYouMeVideoCallback.mVideoPreDecodeCallback = callback;
//		if (callback == null) {
//			NativeEngine.setVideoPreDecodeCallbackEnable(false , needDecodeandRender);
//		} else {
//			NativeEngine.setVideoPreDecodeCallbackEnable(true, needDecodeandRender);
//		}
//	}

    /**
     * 功能描述:   设置是否由外部输入音视频
     *
     * @param bInputModeEnabled: true:外部输入模式，false:SDK内部采集模式
     */
    public static native void setExternalInputMode(boolean bInputModeEnabled);

    /**
     * 功能描述:初始化引擎
     *
     * @param strAPPKey:在申请SDK注册时得到的App    Key，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     * @param strAPPSecret:在申请SDK注册时得到的App Secret，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int initWithAppKey(String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);


    /**
     * 功能描述:反初始化引擎
     *
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int unInit();

    /**
     *  功能描述:设置身份验证的token
     *  @param strToken: 身份验证用token，设置为NULL或者空字符串，清空token值。
     *  @return 无
     */
//    public static native void setToken( String strToken );

    /**
     *  功能描述:设置身份验证的token
     *  @param strToken: 身份验证用token，设置为NULL或者空字符串，清空token值。
     *  @param timeStamp: 用户加入房间的时间，单位s。
     *  @return 无
     */
//	public static native void setTokenV3( String strToken, long timeStamp);

    /**
     *  功能描述:设置是否使用TCP模式来收发数据，针对特殊网络没有UDP端口使用，必须在加入房间之前调用
     *  @param bUseTcp: 是否使用。
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//	public static native int setTCPMode( boolean bUseTcp );

    /**
     * 功能描述: 设置用户自定义Log路径
     *
     * @param filePath Log文件的路径
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setUserLogPath(String filePath);

    /**
     * 功能描述:切换语音输出设备
     * 默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
     *
     * @param bOutputToSpeaker:true——使用扬声器，false——使用听筒
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setOutputToSpeaker(boolean bOutputToSpeaker);

    /**
     * 功能描述:设置扬声器静音
     *
     * @param bOn:true——静音，false——取消静音
     * @return 无
     */
    public static native void setSpeakerMute(boolean bOn);

    /**
     * 功能描述:获取扬声器静音状态
     *
     * @return true——静音，false——没有静音
     */
    public static native boolean getSpeakerMute();

    /**
     * 功能描述:获取麦克风静音状态
     *
     * @return true——静音，false——没有静音
     */
    public static native boolean getMicrophoneMute();

    /**
     * 功能描述:设置麦克风静音
     *
     * @param bOn:true——静音，false——取消静音
     * @return 无
     */
    public static native void setMicrophoneMute(boolean mute);

    /**
     *  功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
     *
     *  @param bAutoSend:true——通知，false——不通知
     *  @return 无
     */
//    public static native void setAutoSendStatus( boolean bAutoSend );


    /**
     * 功能描述:获取音量大小,此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量
     *
     * @return 音量值[0, 100]
     */
    public static native int getVolume();

    /**
     * 功能描述:增加音量，音量数值加1
     *
     * @return 无
     */
    public static native void setVolume(int uiVolume);

    /**
     *  功能描述:是否可使用移动网络
     *
     *  @return true-可以使用，false-禁用
     */
//    public static native boolean getUseMobileNetworkEnabled ();

    /**
     *  功能描述:启用/禁用移动网络
     *
     *  @param bEnabled:true-可以启用，false-禁用，默认禁用
     *
     *  @return 无
     */
//    public static native void setUseMobileNetworkEnabled (boolean bEnabled);

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
//	public static native int setLocalConnectionInfo(String pLocalIP, int iLocalPort, String pRemoteIP, int iRemotePort);

    /**
     *  功能描述:清除本地局域网连接信息，强制server转发
     *
     *
     *  @return 无
     */
//    public static native void clearLocalConnectionInfo();

    /**
     *  功能描述:设置是否切换server通路
     *
     *  @param enable: 设置是否切换server通路标志
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//	public static native int setRouteChangeFlag(boolean enable);

    //---------------------多人语音接口---------------------//

    /**
     * 功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
     *
     * @param strUserID: 用户ID，要保证全局唯一
     * @param strRoomID: 频道ID，要保证全局唯一
     * @param userRole:  用户角色，用于决定讲话/播放背景音乐等权限
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int joinChannelSingleMode(String strUserID, String strRoomID, int userRole, boolean autoRecv);

    /**
     *  功能描述：加入语音频道（多频道模式，可以同时在多个语音频道里面）
     *
     *  @param strUserID: 用户ID，要保证全局唯一
     *  @param strRoomID: 频道ID，要保证全局唯一
     *  @param userRole: 用户角色，用于决定讲话/播放背景音乐等权限
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//    public static native int joinChannelMultiMode(String strUserID, String strRoomID, int userRole);

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
//	public static native int joinChannelSingleModeWithAppKey (String strUserID, String strRoomID, int userRole, String strJoinAppKey, boolean autoRecv);

    /**
     *  功能描述：对指定频道说话
     *
     *  @param strRoomID: 频道ID，要保证全局唯一
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//    public static native int speakToChannel(String strRoomID);


    /**
     *  功能描述:退出多频道模式下的某个语音频道
     *
     *  @param strChannelID:频道ID，要保证全局唯一
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//    public static native int leaveChannelMultiMode (String strChannelID);
    /**
     *  功能描述:退出所有语音频道
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//    public static native int leaveChannelAll ();

    /**
     * 功能描述:查询频道的用户列表
     *
     * @param strChannelID:要查询的频道ID
     * @param maxCount:想要获取的最大数量，-1表示获取全部
     * @param notifyMemChagne:            其他用户进出房间时，是否要收到通知
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int getChannelUserList(String strChannelID, int maxCount, boolean notifyMemChange);

    /**
     * 功能描述:控制其他人的麦克风开关
     *
     * @param strUserID:用户ID，要保证全局唯一
     * @param mute:                  true 静音对方的麦克风，false 取消静音对方麦克风
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setOtherMicMute(String strUserID, boolean status);

    /**
     *  功能描述:控制其他人的扬声器开关
     *
     *  @param strUserID:用户ID，要保证全局唯一
     *  @param mute: true 静音对方的扬声器，false 取消静音对方扬声器
     *
     *  @return 错误码，详见YouMeConstDefine.h定义
     */
//    public static native int setOtherSpeakerMute (String strUserID, boolean status );

    /**
     * 功能描述:选择消除其他人的语音
     *
     * @param strUserID:用户ID，要保证全局唯一
     * @param on:                    false屏蔽对方语音，true取消屏蔽
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setListenOtherVoice(String strUserID, boolean on);

    public static native void setServerRegion(int region, String extServerName, boolean bAppend);


    /**
     * 功能描述: 播放背景音乐，并允许选择混合到扬声器输出麦克风输入
     *
     * @param filePath 音乐文件的路径
     * @param bRepeat  是否重复播放
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int playBackgroundMusic(String filePath, boolean bRepeat);

    /**
     * 功能描述: 如果当前正在播放背景音乐的话，暂停播放
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int pauseBackgroundMusic();

    /**
     * 功能描述: 如果当前背景音乐处于暂停状态的话，恢复播放
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int resumeBackgroundMusic();

    /**
     * 功能描述: 如果当前正在播放背景音乐的话，停止播放
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int stopBackgroundMusic();

    /**
     * 功能描述: 设置背景音乐播放的音量
     *
     * @param vol 背景音乐的音量，范围 0-100
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setBackgroundMusicVolume(int vol);

    /**
     *  功能描述: 设置是否用耳机监听自己的声音或背景音乐，当不插耳机或外部输入模式时，这个设置不起作用
     *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setHeadsetMonitorOn(boolean micEnabled);

    /**
     *  功能描述: 设置是否用耳机监听自己的声音或背景音乐，当不插耳机或外部输入模式时，这个设置不起作用
     *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
     *  @param bgmEnabled 是否监听背景音乐 true 监听，false 不监听 默认开启
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setHeadsetMonitorOn(boolean micEnabled, boolean bgmEnabled);

    /**
     *  功能描述: 设置是否开启主播混响模式
     *  @param enabled, true 开启，false 关闭
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setReverbEnabled (boolean enabled);

    /**
     * 功能描述: 暂停通话，释放麦克风等设备资源
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int pauseChannel();

    /**
     * 功能描述: 恢复通话
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int resumeChannel();

    /**
     * 功能描述: 设置是否开启语音检测回调
     *
     * @param enabled, true 开启，false 关闭
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVadCallbackEnabled(boolean enabled);

    /**
     *  功能描述: 设置当前录音的时间戳
     *  @return None
     */
//    public static native void setRecordingTimeMs (int timeMs);

    /**
     *  功能描述: 设置当前播放的时间戳
     *  @return None
     */
//    public static native void setPlayingTimeMs (int timeMs);

    /**
     *  功能描述: 设置是否开启讲话音量回调, 并设置相应的参数
     *  @param maxLevel, 音量最大时对应的级别，最大可设100。根据实际需要设置小于100的值可以减少回调的次数。
     *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
     *                   设 0 表示关闭回调。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setMicLevelCallback(int maxLevel);

    /**
     *  功能描述: 设置是否开启远端语音音量回调, 并设置相应的参数
     *  @param maxLevel, 音量最大时对应的级别，最大可设100。
     *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
     *                   设 0 表示关闭回调。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setFarendVoiceLevelCallback(int maxLevel);

    /**
     *  功能描述: 设置当麦克风静音时，是否释放麦克风设备，在初始化之后、加入房间之前调用
     *  @param enabled,
     *      true 当麦克风静音时，释放麦克风设备，此时允许第三方模块使用麦克风设备录音。在Android上，语音通过媒体音轨，而不是通话音轨输出。
     *      false 不管麦克风是否静音，麦克风设备都会被占用。
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setReleaseMicWhenMute(boolean enabled);

    /**
     * 功能描述: 设置插入耳机时，是否自动退出系统通话模式(禁用手机硬件提供的回声消除等信号前处理)
     * 系统提供的前处理效果包括回声消除、自动增益等，有助于抑制背景音乐等回声噪音，减少系统资源消耗
     * 由于插入耳机可从物理上阻断回声产生，故可设置禁用该效果以保留背景音乐的原生音质效果
     * 默认为false，在初始化之后、加入房间之前调用
     * 注：Windows和macOS不支持该接口
     *
     * @param enabled, true 当插入耳机时，自动禁用系统硬件信号前处理，拔出时还原
     *                 false 插拔耳机不做处理
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
//    public static native int setExitCommModeWhenHeadsetPlugin(boolean enabled);
    public static native String getSdkInfo();

    public static native int createRender(String userId);

    public static native int deleteRender(int renderId);

    public static native int deleteRenderByUserID(String userId);

    private static native int setVideoCallback();
    /**
     *  功能描述:Rest API , 向服务器请求额外数据
     *  @param strCommand: 请求的命令字符串
     *  @param strQueryBody: 请求需要的数据,json格式，内容参考restAPI
     *  @return 0 - 负数表示错误码，正数表示回调requestID，用于标识这个查询
     *          其他 - 具体错误码
     */
//    public static native int   requestRestApi( String strCommand , String strQueryBody  );

    //---------------------抢麦接口---------------------//
    /**
     * 功能描述:    抢麦相关设置（抢麦活动发起前调用此接口进行设置）
     * @param pChannelID: 抢麦活动的频道id
     * @param mode: 抢麦模式（1:先到先得模式；2:按权重分配模式）
     * @param maxAllowCount: 允许能抢到麦的最大人数
     * @param maxTalkTime: 允许抢到麦后使用麦的最大时间（秒）
     * @param voteTime: 抢麦仲裁时间（秒），过了X秒后服务器将进行仲裁谁最终获得麦（仅在按权重分配模式下有效）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setGrabMicOption(String pChannelID, int mode, int maxAllowCount, int maxTalkTime, int voteTime);

    /**
     * 功能描述:    发起抢麦活动
     * @param pChannelID: 抢麦活动的频道id
     * @param pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	 public static native int startGrabMicAction(String pChannelID, String pContent);

    /**
     * 功能描述:    停止抢麦活动
     * @param pChannelID: 抢麦活动的频道id
     * @param pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int stopGrabMicAction(String pChannelID, String pContent);

    /**
     * 功能描述:    发起抢麦请求
     * @param pChannelID: 抢麦的频道id
     * @param score: 积分（权重分配模式下有效，游戏根据自己实际情况设置）
     * @param isAutoOpenMic: 抢麦成功后是否自动开启麦克风权限
     * @param pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int requestGrabMic(String pChannelID, int score, boolean isAutoOpenMic, String pContent);

    /**
     * 功能描述:    释放抢到的麦
     * @param pChannelID: 抢麦活动的频道id
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	 public static native int releaseGrabMic(String pChannelID);


    //---------------------连麦接口---------------------//
    /**
     * 功能描述:    连麦相关设置（角色是频道的管理者或者主播时调用此接口进行频道内的连麦设置）
     * @param pChannelID: 连麦的频道id
     * @param waitTimeout: 等待对方响应超时时间（秒）
     * @param maxTalkTime: 最大通话时间（秒）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	 public static native int setInviteMicOption(String pChannelID, int waitTimeout, int maxTalkTime);

    /**
     * 功能描述:    发起与某人的连麦请求（主动呼叫）
     * @param pUserID: 被叫方的用户id
     * @param pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	 public static native int requestInviteMic(String pChannelID, String pUserID, String pContent);

    /**
     * 功能描述:    对连麦请求做出回应（被动应答）
     * @param pUserID: 主叫方的用户id
     * @param isAccept: 是否同意连麦
     * @param pContent: 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int responseInviteMic(String pUserID, boolean isAccept, String pContent);

    /**
     * 功能描述:    停止连麦
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int stopInviteMic();

    /**
     * 功能描述:   向房间用户发送消息
     * @param pChannelID: 房间ID
     * @param pContent: 消息内容-文本串
     * @return - 负数表示错误码，正数表示回调requestID，用于标识这次消息发送
     */
//    public static native int   sendMessage( String  pChannelID, String pContent);

    /**
     * 功能描述:   向房间用户发送消息
     * @param pChannelID: 房间ID
     * @param pContent: 消息内容-文本串
     * @param pUserID: 房间用户ID，如果为空，则表示向房间广播消息
     * @return - 负数表示错误码，正数表示回调requestID，用于标识这次消息发送
     */
//	public static native int   sendMessageToUser( String  pChannelID, String pContent, String pUserID);

    /**
     *  功能描述: 把某人踢出房间
     *  @param  pUserID: 被踢的用户ID
     *  @param  pChannelID: 从哪个房间踢出
     *  @param  lastTime: 踢出后，多长时间内不允许再次进入
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int  kickOtherFromChannel(  String pUserID, String  pChannelID,  int lastTime  );

    /**
     * 功能描述: 设置日志等级
     *
     * @param consoleLevel: 控制台日志等级, 有效值参看YOUME_LOG_LEVEL
     * @param fileLevel:    文件日志等级, 有效值参看YOUME_LOG_LEVEL
     */
    public static native void setLogLevel(int consoleLevel, int fileLevel);

    /**
     * 功能描述: 设置外部输入模式的语音采样率
     *
     * @param inputSampleRate:         输入语音采样率
     * @param mixedCallbackSampleRate: mix后输出语音采样率
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setExternalInputSampleRate(int inputSampleRate, int mixedCallbackSampleRate);

    /**
     * 功能描述: 设置语音传输质量
     *
     * @param quality: 0: low, 1: high
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native void setAudioQuality(int quality);

    /**
     *  功能描述: 设置视频接收平滑开关
     *  @param enable  0:关闭平滑，1:打开平滑
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int  setVideoSmooth( int mode );

    /**
     *  功能描述: 设置视频大小流接收调整模式 
     *  @param mode 模式 0:自动调整，1:手动调整
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int  setVideoNetAdjustmode( int mode );

    /**
     * 功能描述: 设置视频网络传输过程的分辨率, 第一路高分辨率
     *
     * @param width:宽
     * @param height:高
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoNetResolution(int width, int height);

    /**
     * 功能描述: 设置视频网络传输过程的分辨率 第二路低分辨率，默认不发送
     *
     * @param width:宽
     * @param height:高
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoNetResolutionForSecond(int width, int height);

    /**
     * 功能描述: 设置共享视频网络传输过程的分辨率
     *
     * @param width:宽
     * @param height:高
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoNetResolutionForShare(int width, int height);

    /**
     * 功能描述: 设置视频数据上行的码率的上下限。
     *
     * @param maxBitrate: 最大码率，单位kbit/s.  0无效
     * @param minBitrate: 最小码率，单位kbit/s.  0无效
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native void setVideoCodeBitrate(int maxBitrate, int minBitrate);


    /**
     * 功能描述: 设置视频数据上行的码率的上下限，第二路
     *
     * @param maxBitrate: 最大码率，单位kbit/s.  0无效
     * @param minBitrate: 最小码率，单位kbit/s.  0无效
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native void setVideoCodeBitrateForSecond(int maxBitrate, int minBitrate);


    /**
     * 功能描述: 设置共享视频数据上行的码率的上下限
     *
     * @param maxBitrate: 最大码率，单位kbps.  0：使用默认值
     * @param minBitrate: 最小码率，单位kbps.  0：使用默认值
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native void setVideoCodeBitrateForShare(int maxBitrate, int minBitrate);

    /**
     * 功能描述: 获取视频数据上行的当前码率。
     *
     * @return 视频数据上行的当前码率
     */
    public static native int getCurrentVideoCodeBitrate();

    /**
     * 功能描述: 设置视频编码是否采用VBR动态码率方式
     *
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native void setVBR(boolean useVBR);


    /**
     * 功能描述: 设置小流视频编码是否采用VBR动态码率方式
     *
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native void setVBRForSecond(boolean useVBR);

    /**
     * 功能描述: 设置共享流视频编码是否采用VBR动态码率方式
     *
     * @return None
     * @warning:需要在进房间之前设置
     */
    public static native int setVBRForShare(boolean useVBR);

    /**
     * 功能描述: 设置视频数据是否同意开启硬编硬解
     *
     * @param bEnable: true:开启，false:不开启
     * @return None
     * @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
     * @warning:需要在进房间之前设置
     */
    public static native void setVideoHardwareCodeEnable(boolean bEnable);

    /**
     * 功能描述: 获取视频数据是否同意开启硬编硬解
     *
     * @return true:开启，false:不开启， 默认为true;
     * @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
     */
    public static native boolean getVideoHardwareCodeEnable();

    /**
     * 功能描述: 获取是否使用GL进行视频前处理
     *
     * @return true:开启，false:不开启， 默认为true;
     * @note: 无。
     */
    public static native boolean getUseGL();

    /**
     * 功能描述: 设置无视频帧渲染的等待超时时间
     *
     * @param timeout:单位毫秒
     */
    public static native void setVideoNoFrameTimeout(int timeout);

    /**
     * 功能描述: 设置音视频统计数据时间间隔
     *
     * @param interval:时间间隔
     */
    public static native void setAVStatisticInterval(int interval);

//    public static native int openVideoEncoder(String pFilePath);

    /**
     *  功能描述: 设置本地视频渲染回调的分辨率
     *  @param width:宽
     *  @param height:高
     *  @return 0 - 成功
     *          其他 - 具体错误码
     */
//    public static native int setVideoLocalResolution(int width, int height);

    /**
     * 功能描述:   是否启动前置摄像头
     *
     * @return 0 - 成功
     * 其他 - 具体错误码
     */
    public static native int setCaptureFrontCameraEnable(boolean enable);

    /**
     *  功能描述: 设置视频帧率
     *  @param fps:帧率（1-60），默认15帧
     */
//    public static native int setVideoPreviewFps(int fps);

    /**
     * 功能描述: 设置视频帧率
     *
     * @param fps:帧率（1-60），默认15帧
     */
    public static native int setVideoFps(int fps);

    /**
     * 功能描述: 设置小流帧率 (如果大于大流帧率，以大流帧率为准)
     *
     * @param fps:帧率（1-30），默认15帧
     */
    public static native int setVideoFpsForSecond(int fps);

    /**
     * 功能描述: 设置共享视频帧率
     *
     * @param fps:帧率（1-30），默认15帧
     */
    public static native int setVideoFpsForShare(int fps);

    /**
     * 功能描述:切换身份(仅支持单频道模式，进入房间以后设置)
     *
     * @param userRole: 用户身份
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int setUserRole(int userRole);


    /**
     * 功能描述:获取身份(仅支持单频道模式)
     *
     * @return 身份定义，详见YouMeConstDefine.h定义
     */
    public static native int getUserRole();


    /**
     * 功能描述:查询是否在某个语音频道内
     *
     * @param strChannelID:要查询的频道ID
     * @return true——在频道内，false——没有在频道内
     */
    public static native boolean isInChannel(String strChannelID);

    /**
     * 功能描述:查询是否在语音频道内
     *
     * @return true——在频道内，false——没有在频道内
     */
    public static native boolean isJoined();

    /**
     *  功能描述:查询是否在语音频道内
     *
     *  @return true——在频道内，false——没有在频道内
     */
//	public static boolean isInChannel(){
//		return api.isJoined();
//	}


    /**
     * 功能描述:背景音乐是否在播放
     *
     * @return true——正在播放，false——没有播放
     */
    public static native boolean isBackgroundMusicPlaying();


    /**
     * 功能描述:判断是否初始化完成
     *
     * @return true——完成，false——还未完成
     */
    public static native boolean isInited();

    /**
     * 功能描述: 查询多个用户视频信息（支持分辨率）
     *
     * @param userArray: 用户ID列表
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int queryUsersVideoInfo(String[] userArray);

    /**
     * 功能描述: 设置多个用户视频信息（支持分辨率）
     *
     * @param userArray:       用户ID列表
     * @param resolutionArray: 用户对应分辨率列表
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setUsersVideoInfo(String[] userArray, int[] resolutionArray);

    ///摄像头相关

    /**
     * 功能描述:   启动摄像头采集
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static int startCapturer() {
        return NativeEngine.startCapture();
    }

    /***
     * 停止采集
     */
    public static void stopCapturer() {
        NativeEngine.stopCapture();
    }

    /***
     * 切换前后摄像头
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
    public static int switchCamera() {
        return NativeEngine.switchCamera();
    }


    ///外部采集输入

    /**
     * 功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面, 单声道(待废弃)
     *
     * @param data      指向PCM数据的缓冲区
     * @param len       音频数据的大小
     * @param timestamp 时间戳
     * @return 成功/失败
     */
    public static boolean inputAudioFrame(byte[] data, int len, long timestamp) {
        return NativeEngine.inputAudioFrame(data, len, timestamp, 1, false);
    }

    /**
     *  功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面
     *  @param data 指向PCM数据的缓冲区
     *  @param len  音频数据的大小
     *  @param timestamp 时间戳
     *    @param channelnum  声道数，1:单声道，2:双声道，其它非法
     *  @param binterleaved 音频数据打包格式（仅对双声道有效）
     *  @return 成功/失败
     */
//	public static boolean inputAudioFrameEx(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved)
//	{
//		long realLen = data.length;
//		if(realLen < 2 || len > realLen)
//		{
//			Log.e("YOUME", "inputAudioFrameEx data length error input len:"+ len +" real len:"+realLen );
//			return false;
//		}
//
//		return NativeEngine.inputAudioFrame( data, len , timestamp, channelnum, binterleaved );
//	}

    /**
     *  功能描述: 将多路音频数据流混合到麦克风或者扬声器的音轨里面。
     *  @param streamId  音频数据的流id
     *  @param data 指向PCM数据的缓冲区
     *  @param len  音频数据的大小
     *  @param frameInfo 音频数据的格式信息
     *  @param timestamp 时间戳
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static boolean inputAudioFrameForMix(int streamId, byte[] data, int len, YMAudioFrameInfo frameInfo, long timestamp)
//	{
//		return NativeEngine.inputAudioFrameForMix(streamId, data, len , frameInfo, timestamp );
//	}

    /**
     * 功能描述: 输入用户自定义数据，发送到房间内指定成员或广播全部成员
     *
     * @param data:自定义数据
     * @param len:数据长度，不能大于1024
     * @param timestamp:时间戳
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static int inputCustomData(byte[] data, int len, long timestamp) {
        return NativeEngine.inputCustomData(data, len, timestamp);
    }

    /**
     *  功能描述: 输入用户自定义数据，发送到房间内指定成员或广播全部成员
     *  @param  data:自定义数据
     *  @param  len:数据长度，不能大于1024
     *  @param  timestamp:时间戳
     *  @param  userId:接收端用户
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static int inputCustomDataToUser(byte[] data,int len,long timestamp, String userId)
//	{
//		return NativeEngine.inputCustomDataToUser(data, len, timestamp, userId);
//	}


    /**
     * 功能描述: 视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)
     *
     * @param data      视频帧数据
     * @param len       视频数据大小
     * @param width     视频图像宽
     * @param height    视频图像高
     * @param fmt       视频格式
     * @param rotation  视频角度
     * @param mirror    镜像
     * @param timestamp 时间戳
     * @return 成功/失败
     */
    public static boolean inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp) {
        return NativeEngine.inputVideoFrame(data, len, width, height, fmt, rotation, mirror, timestamp);
    }

//	public static boolean inputVideoFrameByteBuffer(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp)
//	{
//		return  NativeEngine.inputVideoFrameByteBuffer( data, len , width, height, fmt, rotation, mirror, timestamp );
//	}

    /**
     * 功能描述: 视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)
     * @param data 视频帧数据
     * @param len 视频数据大小
     * @param width 视频图像宽
     * @param height 视频图像高
     * @param fmt 视频格式
     * @param rotation 视频角度
     * @param mirror 镜像
     * @param timestamp 时间戳
     * @param streamID 大小流
     * @return 成功/失败
     */
//	public  static boolean inputVideoFrameEncrypt(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp, int streamID)
//	{
//		return  NativeEngine.inputVideoFrameEncrypt( data, len , width, height, fmt, rotation, mirror, timestamp, streamID );
//	}

    /**
     * 功能描述: 共享视频数据输入，不需要sdk来进行预览，和摄像头视频流一起发送出去
     *
     * @param data      视频帧数据
     * @param len       视频数据大小
     * @param width     视频图像宽
     * @param height    视频图像高
     * @param fmt       视频格式
     * @param rotation  视频角度
     * @param mirror    镜像
     * @param timestamp 时间戳
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static boolean inputVideoFrameForShare(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp) {
        return NativeEngine.inputVideoFrameForShare(data, len, width, height, fmt, rotation, mirror, timestamp);
    }

//	public static boolean inputVideoFrameByteBufferForShare(ByteBuffer data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp)
//	{
//		return  NativeEngine.inputVideoFrameByteBufferForShare( data, len , width, height, fmt, rotation, mirror, timestamp );
//	}

    /**
     * 功能描述: 视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)
     *
     * @param texture   纹理ID
     * @param matrix    纹理矩阵坐标
     * @param width     视频图像宽
     * @param height    视频图像高
     * @param fmt       视频格式
     * @param rotation  视频角度
     * @param mirror    镜像
     * @param timestamp 时间戳
     * @return 成功/失败
     */
    public static boolean inputVideoFrameGLES(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp) {
        return NativeEngine.inputVideoFrameGLES(texture, matrix, width, height, fmt, rotation, mirror, timestamp);
    }

    /**
     * 功能描述: 共享视频数据输入(不需要sdk来进行预览，和摄像头视频流一起发送出去)
     *
     * @param texture   纹理ID
     * @param matrix    纹理矩阵坐标
     * @param width     视频图像宽
     * @param height    视频图像高
     * @param fmt       视频格式
     * @param rotation  视频角度
     * @param mirror    镜像
     * @param timestamp 时间戳
     * @return 成功/失败
     */
    public static boolean inputVideoFrameGLESForshare(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp) {
        return NativeEngine.inputVideoFrameGLESForShare(texture, matrix, width, height, fmt, rotation, mirror, timestamp);
    }

    /**
     * 功能描述: 停止视频数据输入(在inputVideoFrame之后调用，房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP事件)
     */
    public static void stopInputVideoFrame() {
        NativeEngine.stopInputVideoFrame();
    }

    public static void stopInputVideoFrameForShare() {
        NativeEngine.stopInputVideoFrameForShare();
    }

//	public static void setMixVideoSize(int width, int height)
//	{
//		NativeEngine.setMixVideoSize( width, height );
//	}

//	public static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height)
//	{
//		NativeEngine.addMixOverlayVideo( userId, x, y, z ,width, height );
//	}
//	public static void removeMixOverlayVideo(String userId)
//	{
//		NativeEngine.removeMixOverlayVideo( userId );
//	}

//	public static void removeMixAllOverlayVideo()
//	{
//		NativeEngine.removeMixAllOverlayVideo( );
//	}

//	public static void maskVideoByUserId(String userId, boolean mask)
//	{
//		NativeEngine.maskVideoByUserId( userId, mask );
//	}

    ///美颜相关
    public static void openBeautify(boolean open) {
        NativeEngine.openBeautify(open);
    }

    /***
     * 设置美颜, 0.0 - 1.0
     */
    public static void setBeautyLevel(float level) {
        NativeEngine.setBeautyLevel(level);
    }

    /**
     *  功能描述: 调用后同步完成麦克风释放，只是为了方便使用 IM 的录音接口时，临时切换麦克风使用权。
     *  @return bool - 成功
     */
//	public static boolean releaseMicSync(){
//		AudioRecorder.OnAudioRecorderTmp(0);
//		return true;
//	}

    /**
     *  功能描述: 调用后恢复麦克风到释放前的状态，只是为了方便使用 IM 的录音接口时，临时切换麦克风使用权。
     *  @return bool - true 成功
     */
//	public static boolean resumeMicSync(){
//		AudioRecorder.OnAudioRecorderTmp(1);
//		return true;
//	}


    /**
     *  功能描述: 设置共享EGLContext；
     *  eglContext: 共享egl context；
     *  @return void
     */
//	public static void setSharedEGLContext(Object eglContext){
//         GLESVideoMixer.setShareEGLContext(eglContext);
//	}

    /**
     *  功能描述: 获取共享EGLContext；
     *  @return Object
     */
//	public static Object sharedEGLContext(){
//        return GLESVideoMixer.getInstance().sharedContext();
//		//return NativeEngine.sharedEGLContext();
//	}

    /**
     *  功能描述: 获取硬件处理方式，camera需要的SurfaceTexture；
     *  @return Object
     */
//    public static GLESVideoMixer.SurfaceContext getCameraSurfaceTexture(){
//	        return GLESVideoMixer.getInstance().getCameraSurfaceContext();
//	 }

    /**
     *  功能描述: 设置视频混流完成回调YUV格式数据
     *  @return boolean - true 成功
     */
//    public static boolean setVideoMixerRawCallbackEnabled(boolean enabled){
//    	return GLESVideoMixer.setMixerYUVCallBack(enabled);
//    }

    /**
     *  功能描述: 设置视频自定义处理监听
     *  @return void
     */
//    public static void setVideoMixerFilterListener(VideoMixerFilterListener listener){
//    	GLESVideoMixer.setFilterListener(listener);
//	}

    /**
     *  功能描述: 打开/关闭 外部扩展滤镜回调
     *  @param  enabled:true表示打开回调，false表示关闭回调
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native int setExternalFilterEnabled(boolean enabled);

    /**
     * 功能描述: 设置视频数据回调方式，硬编解码默认回调opengl纹理方式，使用该方法可以回调yuv格式
     *
     * @param enabled:true 打开，false 关闭，默认关闭
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoFrameRawCbEnabled(boolean enabled);

    /**
     * 功能描述: 设置视频数据回调方式，硬编解码默认回调opengl纹理方式，使用该方法可以将解码数据回调yuv格式
     *
     * @param enabled:true 打开，false 关闭，默认关闭
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoDecodeRawCbEnabled(boolean enabled);

    /**
     *  功能描述: 获取摄像头是否支持变焦
     *  @return boolean - true 支持，false 不支持
     */
//    public static native boolean isCameraZoomSupported();
    /**
     *  功能描述: 设置摄像头变焦
     *  @param  zoomFactor: 1.0 表示原图像大小， 大于1.0 表示放大，默认1.0f
     *  @return 设置后当前缩放比例
     */
//    public static native float setCameraZoomFactor(float zoomFactor);
    /**
     *  功能描述: 获取摄像头是否支持自定义坐标对焦
     *  @return boolean - true 支持，false 不支持
     */
//    public static native boolean isCameraFocusPositionInPreviewSupported();
    /**
     *  功能描述: 设置摄像头自定义对焦坐标,左上角坐标（0.0,1.0）,右下角（1.0,0.0）,默认（0.5,0.5）
     *  @param   x: 横坐标, 0.0f-1.0f
     *  @param   x: 纵坐标, 0.0f-1.0f
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native boolean setCameraFocusPositionInPreview( float x , float y );
    /**
     *  功能描述: 获取摄像头是否支持闪光灯
     *  @return boolean - true 支持，false 不支持
     */
//    public static native boolean isCameraTorchSupported();
    /**
     *  功能描述: 设置摄像头打开关闭闪光灯
     *  @param   isOn: true 打开，false 关闭
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native boolean setCameraTorchOn(boolean isOn );
    /**
     *  功能描述: 获取摄像头是否自动人脸对焦
     *  @return boolean - true 支持，false 不支持
     */
//    public static native boolean isCameraAutoFocusFaceModeSupported();
    /**
     *  功能描述: 设置摄像头打开关闭人脸对焦
     *  @param   enable: true 打开，false 关闭
     *  @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//    public static native boolean setCameraAutoFocusFaceModeEnabled(boolean enabled);

    /**
     * 开启服务器推流，推送自己的数据
     * @param pushUrl:推流地址，可以用restApi获取
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int startPush( String pushUrl );
    /**
     * 关闭服务器推流，对应startPush
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int stopPush();

    /**
     * 开启服务器混流推流，将多个人的画面混到一起，对混流画面就行推流。每个房间只允许一个混流推流。
     * 开启服务器混流推流后，可以通过addPushMixUser/removePushMixUser改变推流的画面。
     * @param pushUrl:推流地址，可以用restApi获取
     * @param width: 推流的画面的宽
     * @param height: 推流的画面的高
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int setPushMix( String pushUrl , int width, int height );
    /**
     * 关闭服务器混流推流，对应setPushMix
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int clearPushMix();

    /**
     * 在服务器混流推流中，加入一个user的数据
     * @param userId: 需要加入的用户ID
     * @param x: 用户画面在混流画面中的位置，x
     * @param y: 用户画面在混流画面中的位置，y
     * @param z: 用户画面在混流画面中的位置，z,代表前后关系
     * @param width: 用户画面在混流画面中的宽
     * @param height: 用户画面在混流画面中的高
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int addPushMixUser( String userId, int x, int y, int z, int width, int height );
    /**
     * 删除混流推流中的一个用户
     * @param userId: 需要删除的用户ID
     * @return YOUME_SUCCESS - 成功
     *          其他 - 具体错误码
     */
//	public static native int removePushMixUser( String userId );

//	public static String m_strCallbackName;

    /**
     * 功能描述: 横竖屏变化时，通知sdk把分辨率切换到横屏适配模式
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int switchResolutionForLandscape();

    /**
     * 功能描述: 设置视频预览镜像模式，目前仅对前置摄像头生效
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native void setlocalVideoPreviewMirror(boolean enable);

    /**
     * 功能描述: 横竖屏变化时，通知sdk把分辨率切换到竖屏适配模式
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int resetResolutionForPortrait();

    /*
     * 功能：国际语言文本翻译
     * @param text：内容
     * @param destLangCode：目标语言编码
     * @param srcLangCode：源语言编码
     * @return <=0,表示错误码，>0表示本次翻译的requestId
     */
//	public static native int translateText(  String text, int destLangCode, int srcLangCode);

    public static native void setScreenSharedEGLContext(Object gles);
}
