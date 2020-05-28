package main.java.com.example.library;

import android.content.Context;
import android.hardware.Camera;
import android.util.Log;

import main.java.com.example.library.audio.AudioMgr;
import main.java.com.example.library.video.CameraMgr;
import main.java.com.example.library.video.VideoFrameCallbackInterface;
import main.java.com.example.library.video.VideoMgr;

public class api {
    public static String YOU_ME_LIB_NAME_STRING = "youme_voice_engine";
    private static final String TAG = "api";

    public static final int YOUME_RTC_BGM_TO_SPEAKER = 0; // Formal environment.
    public static final int YOUME_RTC_BGM_TO_MIC = 1; // Using test servers.

	public static Boolean mInited = false;


    private static boolean LoadSO() {
        try {
            System.loadLibrary(YOU_ME_LIB_NAME_STRING);
        } catch (Throwable e) {
            e.printStackTrace();
            return false;
        }
        return true;
    }

    //加载文件
    public static boolean Init(Context context) {
		Log.d(TAG, "Init: begin");
        if (context == null) {
            Log.e(TAG, "context can not be null");
            return false;
        }

        if (mInited) {
            Log.e(TAG, "Init: Already initialzed");
            return true;
        }

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

        mInited = true;
		Log.d(TAG, "Init: end");
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

    public static void SetCallback(IYouMeCallBack callBack) {
        //IYouMeChannelCallback.callBack = callBack;
        //IYouMeCommonCallback.callBack = callBack;
        YouMeEventCallback.callBack = callBack;
    }

//    public static void setPcmCallbackEnable(YouMeCallBackInterfacePcm callback, int flag, boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel) {
//        Log.d("Api", "setPcmCallbackEnable");
//        YouMeEventCallback.mCallbackPcm = callback;
//        if (callback == null) {
//            NativeEngine.setPcmCallbackEnable(0, false, nOutputSampleRate, nOutputChannel);
//        } else {
//            NativeEngine.setPcmCallbackEnable(flag, outputToSpeaker, nOutputSampleRate, nOutputChannel);
//        }
//    }

//
//    public static void setScreenRotation(int rotation) {
//        CameraMgr.setRotation(rotation);
//    }
//    public static void screenRotationChange() {
//        CameraMgr.rotationChange();
//    }

    /**
     * 功能描述: 设置用户自定义数据的回调
     *
     * @param callback:收到其它人自定义数据的回调地址，需要继承IYouMeCustomDataCallback并实现其中的回调函数
     * @return void
     */
//    public static void setRecvCustomDataCallback(YouMeCustomDataCallbackInterface callback) {
//        IYouMeEventCallback.mCustomDataCallback = callback;
//    }

    //设置合流视频数据回调
    public static void setVideoFrameCallback(VideoFrameCallbackInterface callback) {
        VideoMgr.setVideoFrameCallback(callback);
    }

    //设置音频回调
//    public static void setAudioFrameCallback(AudioFrameCallbackInterface callback) {
//        IYouMeAudioCallback.callback = callback;
//    }

//    public static void SetVideoCallback() {
//        IYouMeVideoCallback.mVideoFrameRenderCallback = VideoRenderer.getInstance();
//        setVideoCallback();
//    }

    /**
     * 功能描述: 设置是否回调视频解码前H264数据，需要在加入房间之前设置
     *
     * @param preDecodeCallback   回调方法
     * @param needDecodeandRender 是否需要解码并渲染:true 需要;false 不需要
     */
//    public static void setVideoPreDecodeCallbackEnable(YouMeVideoPreDecodeCallbackInterface callback, boolean needDecodeandRender) {
//        IYouMeVideoCallback.mVideoPreDecodeCallback = callback;
//        if (callback == null) {
//            NativeEngine.setVideoPreDecodeCallbackEnable(false, needDecodeandRender);
//        } else {
//            NativeEngine.setVideoPreDecodeCallbackEnable(true, needDecodeandRender);
//        }
//    }

    /**
     * 功能描述:   设置是否由外部输入音视频
     *
     * @param bInputModeEnabled: true:外部输入模式，false:SDK内部采集模式
     */
//    public static native void setExternalInputMode(boolean bInputModeEnabled);

    /**
     * 功能描述:初始化引擎
     *
     * @param strAPPKey:在申请SDK注册时得到的App    Key，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     * @param strAPPSecret:在申请SDK注册时得到的App Secret，也可凭账号密码到http://gmx.dev.net/createApp.html查询
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static void initWithAppkey(String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName) {
        NativeEngine.initWithAppkey(strAPPKey, strAPPSecret, serverRegionId, strExtServerRegionName);
    }

    /**
     * 功能描述:反初始化引擎
     *
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int unInit();

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

    public static native void setMicrophoneMute(boolean mute);

    /**
     * 功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
     *
     * @param bAutoSend:true——通知，false——不通知
     * @return 无
     */
    public static native void setAutoSendStatus(boolean bAutoSend);


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
     * 功能描述:是否可使用移动网络
     *
     * @return true-可以使用，false-禁用
     */
    public static native boolean getUseMobileNetworkEnabled();


    //---------------------多人语音接口---------------------//

    /**
     * 功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
     *
     * @param strUserID: 用户ID，要保证全局唯一
     * @param strRoomID: 频道ID，要保证全局唯一
     * @param userRole:  用户角色，用于决定讲话/播放背景音乐等权限
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int joinChannel(String strUserID, String strRoomID, int userRole, boolean autoRecv);

    /**
     * 功能描述:退出多频道模式下的某个语音频道
     *
     * @param strChannelID:频道ID，要保证全局唯一
     * @return 错误码，详见YouMeConstDefine.h定义
     */
    public static native int leaveChannel(String strChannelID);


    public static native int getChannelUserList(String strChannelID, int maxCount, boolean notifyMemChange);


    public static native int setOtherMicMute(String strUserID, boolean status);

    public static native String getSdkInfo();

    public static native int createRender(String userId);

    public static native int deleteRender(int renderId);

    public static native int deleteRenderByUserID(String userId);

    private static native int setVideoCallback();

    public static native int setVideoNetResolution(int width, int height);


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
     * 功能描述: 设置本地视频渲染回调的分辨率
     *
     * @param width:宽
     * @param height:高
     * @return 0 - 成功
     * 其他 - 具体错误码
     */
    public static native int setVideoLocalResolution(int width, int height);

    /**
     * 功能描述:   是否启动前置摄像头
     *
     * @return 0 - 成功
     * 其他 - 具体错误码
     */
    public static native int setCaptureFrontCameraEnable(boolean enable);

    /**
     * 功能描述: 设置视频帧率
     *
     * @param fps:帧率（1-60），默认15帧
     */
    public static native int setVideoFps(int fps);


    /**
     * 功能描述:判断是否初始化完成
     *
     * @return true——完成，false——还未完成
     */
    public static native boolean isInited();

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
    public static void inputAudioFrame(byte[] data, int len, long timestamp) {
        NativeEngine.inputAudioFrame(data, len, timestamp, 1, false);
    }

    /**
     * 功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面
     *
     * @param data         指向PCM数据的缓冲区
     * @param len          音频数据的大小
     * @param timestamp    时间戳
     * @param channelnum   声道数，1:单声道，2:双声道，其它非法
     * @param binterleaved 音频数据打包格式（仅对双声道有效）
     * @return 成功/失败
     */
    public static void inputAudioFrameEx(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved) {
        long realLen = data.length;
        if (realLen < 2 || len > realLen) {
            Log.e("YOUME", "inputAudioFrameEx data length error input len:" + len + " real len:" + realLen);
            return;
        }

        NativeEngine.inputAudioFrame(data, len, timestamp, channelnum, binterleaved);
    }




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
    public static void inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp) {
        NativeEngine.inputVideoFrame(data, len, width, height, fmt, rotation, mirror, timestamp);
    }

    /**
     * 功能描述: 横竖屏变化时，通知sdk把分辨率切换到横屏适配模式
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native int switchResolutionForLandscape();

    public static void setCameraAutoFocusCallBack(Camera.AutoFocusCallback cb) {
        CameraMgr.setCameraAutoFocusCallBack(cb);
    }

    /**
     * 功能描述: 设置视频预览镜像模式，目前仅对前置摄像头生效
     *
     * @return YOUME_SUCCESS - 成功
     * 其他 - 具体错误码
     */
    public static native void setlocalVideoPreviewMirror(boolean enable);


}
