# Video SDK for Android API接口手册

## 相关异步/同步处理方法介绍

游密语音引擎SDK接口调用都会立即返回,凡是本身需要较长耗时的接口调用都会采用异步回调的方式,所有接口都可以在主线程中直接使用。

## API调用说明

API的调用可使用“api.”来直接操作，接口使用的基本流程为`初始化`->`收到初始化成功回调通知`->`加入语音频道`->`收到加入频道成功回调通知`->`使用其它接口`->`离开语音频道`->`反初始化`，要确保严格按照上述的顺序使用接口。

### 视频关键接口流程
1. 在收到`YOUME_EVENT_JOIN_OK`事件后
    注册视频回调：`api.SetVideoCallback();`
    控制自己的摄像头打开：`api.StartCapturer();`
    控制自己的摄像头关闭：`api.StopCapturer();`
2. 以上步骤完成，如果自己或者远端有视频流过来，会通知 `YOUME_EVENT_OTHERS_VIDEO_ON` 事件，在该事件里绑定视频流和渲染组件
    `VideoRenderer.getInstance().addRender(String userId, SurfaceViewRenderer view);`
3. 其它API
    是否屏蔽他人视频： `api.maskVideoByUserId()`
    切换摄像头：`api.SwitchCamera();`
    删除渲染绑定：`VideoRenderer.getInstance().deleteRender();`

## 需实现的回调

使用类需要implements接口YouMeCallBackInterface，并实现该接口下的所有回调函数。回调都在子线程中执行，不能用于更新UI等耗时操作。

*  首先要导入相关的包：

``` 
import com.youme.voiceengine.MemberChange;
import com.youme.voiceengine.YouMeCallBackInterface;
```

* 然后implements YouMeCallBackInterface，具体实现回调方法：

``` 
  @Override
  public void onEvent(int eventType, int iErrorCode, String roomid, Object param){
      //操作结果和运行状态相关回调，eventType 参见 《Video SDK for Android-状态码》的 `YouMeEvent`说明
  }
  @Override
  public void onRequestRestAPI(int requestID, int iErrorCode, String strQuery, String strResult){
       //RestAPI请求结果回调，参考 "requestRestApi(String strCommand, String strQueryBody)" 方法调用说明
  }
  @Override
  public void onMemberChange(String channelID, MemberChange[] arrChanges, boolean isUpdate){
      //频道内成员进出通知
  }
  @Override
  public  void onBroadcast(int bc , String room, String param1, String param2, String content){
      //SDK内置连麦抢麦结果通知
  }
  @Override
  public  void onAVStatistic( int avType,  String userID, int value ){
      //SDK通过质量相关事件通知，avType 参见 《Video SDK for Android-状态码》的 `YouMeAVStatisticType`说明
  }
```

## 设置回调

* **语法**
```
void SetCallback(YouMeCallBackInterface callBack);
```

* **功能**
设置回调。在初始化接口--init(appKey,appsecret...)之前调用。

* **参数说明**
`callBack`:implements 接口YouMeCallBackInterface的类对象。

## 判断是否初始化完成

* **语法**
```
boolean isInited();
```

* **功能**
判断是否初始化完成

* **返回值**
true——初始化完成，false——初始化未完成。


## 设置日志等级
* **语法**

```
void  setLogLevel(  int consoleLevel, int fileLevel );
```

* **功能**
设置日志等级


* **参数说明**
`consoleLevel`：控制台日志等级, 有效值参看YOUME_LOG_LEVEL
`fileLevel`：文件日志等级, 有效值参看YOUME_LOG_LEVEL

###  设置用户自定义Log路径

* **语法**

```
int setUserLogPath (String filePath);
```

* **功能**
设置用户自定义Log路径

* **参数说明**
`filePath`:Log文件的路径

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

##  设置是否使用TCP

* **语法**

```
YouMeErrorCode setTCPMode(boolean bUseTcp);
```

* **功能**
设置是否使用TCP模式来收发数据，针对特殊网络没有UDP端口使用，必须在加入房间之前调用

* **参数说明**
`bUseTcp`:是否使用

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 初始化

* **语法**
```
 int init (String strAPPKey, String strAPPSecret, int serverRegionId, String strExtServerRegionName);
```

* **功能**
初始化语音引擎，做APP验证和资源初始化。

* **参数说明**
`strAPPKey`：从游密申请到的 app key, 这个是你们应用程序的唯一标识。
`strAPPSecret`：对应 appKey 的私钥, 这个需要妥善保存，不要暴露给其他人。
`serverRegionId`：设置首选连接服务器的区域码，如果在初始化时不能确定区域，可以填RTC_DEFAULT_SERVER，后面确定时通过 setServerRegion 设置。如果YOUME_RTC_SERVER_REGION定义的区域码不能满足要求，可以把这个参数设为 RTC_EXT_SERVER，然后通过后面的参数serverRegionName 设置一个自定的区域值（如中国用 "cn" 或者 “ch"表示），然后把这个自定义的区域值同步给游密，我们将通过后台配置映射到最佳区域的服务器。
`strExtServerRegionName`：自定义的扩展的服务器区域名。不能为null，可为空字符串“”。只有前一个参数serverRegionId设为RTC_EXT_SERVER时，此参数才有效（否则都将当空字符串“”处理）。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//涉及到的主要回调事件有：
// YOUME_EVENT_INIT_OK  - 表明初始化成功
// YOUME_EVENT_INIT_FAILED - 表明初始化失败，最常见的失败原因是网络错误或者 AppKey-AppSecret 错误
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

## 语音频道管理

### 加入语音频道（单频道）

* **语法**
```
int joinChannelSingleMode (String strUserID, String strRoomID, int userRole, boolean autoRecv);
```

* **功能**
加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）。

* **参数说明**
`strUserID`：全局唯一的用户标识，全局指在当前应用程序的范围内。
`strRoomID`：全局唯一的频道标识，全局指在当前应用程序的范围内。
`userRole`：用户在语音频道里面的角色，见YouMeUserRole定义。
`autoRecv `：是否自动接收频道内其他有人的视频，true表示自动接收，如果为false，需要调用 `setUsersVideoInfo`指定接收流后才会收到对方视频

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//涉及到的主要回调事件有：
//YOUME_EVENT_JOIN_OK - 成功进入语音频道
//YOUME_EVENT_JOIN_FAILED - 进入语音频道失败
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

### 指定讲话频道

* **语法**
```
int speakToChannel(String strRoomID);
```

* **功能**
多频道模式下，指定当前要讲话的频道。

* **参数说明**
`strRoomID`：全局唯一的频道标识，全局指在当前应用程序的范围内。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//涉及到的主要回调事件有：
//YOUME_EVENT_SPEAK_SUCCESS - 成功切入到指定语音频道
//YOUME_EVENT_SPEAK_FAILED - 切入指定语音频道失败，可能原因是网络或服务器有问题
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

### 退出所有语音频道

* **语法**
```
int leaveChannelAll ();
```
* **功能**
退出所有的语音频道（单频道模式下直接调用此函数离开频道即可）。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//涉及到的主要回调事件有：
//YOUME_EVENT_LEAVED_ALL - 成功退出所有语音频道
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

### 切换身份

* **语法**
```
int setUserRole( int userRole );
```

* **功能**
切换身份(仅支持单频道模式，进入房间以后设置)

* **参数说明**
`userRole`：用户身份

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 获取身份

* **语法**
```
int getUserRole();
```

* **功能**
获取身份(仅支持单频道模式)

* **返回值**
用户身份，参见YouMeUserRole类型定义

### 查询是否在某个语音频道内

* **语法**
```
boolean isInChannel( String strChannelID );
```

* **功能**
查询是否在某个语音频道内

* **返回值**
true——在频道内，false——没有在频道内

### 查询是否在语音频道内

* **语法**
```
boolean isInChannel( );
```

* **功能**
查询是否在语音频道内

* **返回值**
true——在频道内，false——没有在频道内

### 设置白名单用户


## 设备状态管理

### 切换语音输出设备

* **语法**
```
int setOutputToSpeaker (boolean bOutputToSpeaker);
```
* **功能**
默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）

* **参数说明**
`bOutputToSpeaker `：true——输出到扬声器，false——输出到听筒。

* **返回值**
如果成功返回YOUME_SUCCESS，否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 设置扬声器状态

* **语法**
```
void setSpeakerMute (boolean bOn);
```
* **功能**
打开/关闭扬声器。建议该状态值在加入房间成功后按需再重置一次。

* **参数说明**
`bOn`:true——关闭扬声器，false——开启扬声器。


### 获取扬声器状态

* **语法**
```
boolean getSpeakerMute ();
```

* **功能**
获取当前扬声器状态。

* **返回值**
true——扬声器关闭，false——扬声器开启。


### 设置麦克风状态

* **语法**
```
void setMicrophoneMute (boolean mute);
```

* **功能**
打开／关闭麦克风。建议该状态值在加入房间成功后按需再重置一次。

* **参数说明**
`mute`:true——关闭麦克风，false——开启麦克风。


### 获取麦克风状态

* **语法**
```
 boolean getSpeakerMute ();
```

* **功能**
获取当前麦克风状态。

* **返回值**
true——麦克风关闭，false——麦克风开启。

### 设置是否通知别人麦克风和扬声器的开关

* **语法**
```
void setAutoSendStatus( boolean bAutoSend );
```

* **功能**
设置是否通知别人,自己麦克风和扬声器的开关状态

### 设置音量

* **语法**
```
void setVolume (int uiVolume);
```

* **功能**
设置当前程序输出音量大小。建议该状态值在加入房间成功后按需再重置一次。

* **参数说明**
`uiVolume`:当前音量大小，范围[0-100]。

### 获取音量

* **语法**
```
int getVolume ();
```

* **功能**
获取当前程序输出音量大小，此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量。

* **返回值**
当前音量大小，范围[0-100]。

## 设置网络

### 设置是否允许使用移动网络

* **语法**
```
void setUseMobileNetworkEnabled (boolean bEnabled);
```

* **功能**
设置是否允许使用移动网络。在WIFI和移动网络都可用的情况下会优先使用WIFI，在没有WIFI的情况下，如果设置允许使用移动网络，那么会使用移动网络进行语音通信，否则通信会失败。

* **参数说明**
`bEnabled`:true——允许使用移动网络，false——禁止使用移动网络。


### 获取是否允许使用移动网络

* **语法**
```
boolean getUseMobileNetworkEnabled ();
```

* **功能**
获取是否允许SDK在没有WIFI的情况使用移动网络进行语音通信。

* **返回值**
true——允许使用移动网络，false——禁止使用移动网络，默认情况下允许使用移动网络。

## 控制他人麦克风

* **语法**
```
int setOtherMicMute (String strUserID, boolean status );
```

* **功能**
控制他人的麦克风状态

* **参数说明**
`strUserID`：要控制的用户ID
`status`：是否静音。true:静音别人的麦克风，false：开启别人的麦克风

* **返回值**
如果成功返回YOUME_SUCCESS，否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 控制他人扬声器

* **语法**
```
int setOtherSpeakerMute (String strUserID, boolean status );
```

* **功能**
控制他人的扬声器状态

* **参数说明**
`strUserID`：要控制的用户ID
`status`：是否静音。true:静音别人的扬声器，false：开启别人的扬声器

* **返回值**
如果成功返回YOUME_SUCCESS，否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 设置是否听某人的语音

* **语法**
```
int setListenOtherVoice (String strUserID, boolean on );
```

* **功能**
设置是否听某人的语音。

* **参数说明**
`strUserID`：要控制的用户ID。
`on`：true表示开启接收指定用户的语音，false表示屏蔽指定用户的语音。

* **返回值**
无。

## 设置当麦克风静音时，是否释放麦克风设备

* **语法**
```
int setReleaseMicWhenMute(boolean enabled);
```

* **功能**
设置当麦克风静音时，是否释放麦克风设备（需要在初始化成功后，加入房间之前调用）

* **参数说明**
`enabled`： true--当麦克风静音时，释放麦克风设备，此时允许第三方模块使用麦克风设备录音。在Android上，语音通过媒体音轨，而不是通话音轨输出；false--不管麦克风是否静音，麦克风设备都会被占用。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 通话管理

### 暂停通话

* **语法**
```
int pauseChannel();
```

* **功能**
暂停通话，释放对麦克风等设备资源的占用。当需要用第三方模块临时录音时，可调用这个接口。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//主要回调事件：
//YOUME_EVENT_PAUSED - 暂停语音频道完成
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

### 恢复通话

* **语法**
```
int resumeChannel();
```

* **功能**
恢复通话，调用PauseChannel暂停通话后，可调用这个接口恢复通话。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//主要回调事件：
//YOUME_EVENT_RESUMED - 恢复语音频道完成
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

## 设置语音检测

* **语法**
```
int setVadCallbackEnabled (boolean enabled);
```

* **功能**
设置是否开启语音检测回调。开启后频道内有人正在讲话与结束讲话都会发起相应回调通知。

* **参数说明**
`enabled`:true——打开，false——关闭。

* **返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。


## 设置是否开启讲话音量回调

* **语法**
```
int setMicLevelCallback(int maxLevel);
```

* **功能**
设置是否开启讲话音量回调, 并设置相应的参数。

* **参数说明**
`maxLevel`:音量最大时对应的级别，最大可设100。根据实际需要设置小于100的值可以减少回调的次数（注意设置较高的值可能会产生大量回调，特别在Unity上会影响其它事件到达，一般建议不超过30）。比如你只在UI上呈现10级的音量变化，那就设10就可以了。设 0 表示关闭回调。

**返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。


## 设置是否开启远端语音音量回调

* **语法**
```
int setFarendVoiceLevelCallback(int maxLevel);
```

* **功能**
设置是否开启远端语音音量回调, 并设置相应的参数

* **参数说明**
`maxLevel`:音量最大时对应的级别，最大可设100。比如你只在UI上呈现10级的音量变化，那就设10就可以了。设 0 表示关闭回调。

**返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 背景音乐管理

### 播放背景音乐

* **语法**
```
 int playBackgroundMusic (String filePath, boolean bRepeat);
```

* **功能**
播放指定的音乐文件。播放的音乐将会通过扬声器输出，并和语音混合后发送给接收方。这个功能适合于主播/指挥等使用。

* **参数说明**
`path`：音乐文件的路径。
`repeat`：是否重复播放，true——重复播放，false——只播放一次就停止播放。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//主要回调事件：
//YOUME_EVENT_BGM_STOPPED - 通知背景音乐播放结束
//YOUME_EVENT_BGM_FAILED - 通知背景音乐播放失败
void onEvent(int eventType, int iErrorCode, String roomid, Object param);
```

### 停止播放背景音乐

* **语法**
```
int stopBackgroundMusic ();
```

* **功能**
停止播放当前正在播放的背景音乐。
这是一个同步调用接口，函数返回时，音乐播放也就停止了。

* **返回值**
如果成功返回YOUME_SUCCESS，表明成功停止了音乐播放流程；否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 暂停播放背景音乐

* **语法**

```
int pauseBackgroundMusic();
```

* **功能**
如果当前正在播放背景音乐的话，暂停播放

* **返回值**
如果成功返回YOUME_SUCCESS，表明成功停止了音乐播放流程；否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 恢复播放背景音乐

* **语法**

```
int resumeBackgroundMusic();
```

* **功能**
如果当前正在播放背景音乐的话，恢复播放

* **返回值**
如果成功返回YOUME_SUCCESS，表明成功停止了音乐播放流程；否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 背景音乐是否在播放

* **语法**

```
boolean isBackgroundMusicPlaying();
```

* **功能**
是否在播放背景音乐

* **返回值**
true——正在播放，false——没有播放

### 设置背景音乐播放音量

* **语法**
```
int setBackgroundMusicVolume (int vol);
```

* **功能**
设定背景音乐的音量。这个接口用于调整背景音乐和语音之间的相对音量，使得背景音乐和语音混合听起来协调。
这是一个同步调用接口。

* **参数说明**
`vol`:背景音乐的音量，范围 [0-100]。

* **返回值**
如果成功（表明成功设置了背景音乐的音量）返回YOUME_SUCCESS，否则返回错误码，具体请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 设置监听

* **语法**
```
int setHeadsetMonitorOn(boolean micEnabled, boolean bgmEnabled);
```

* **功能**
设置是否用耳机监听自己的声音，当不插耳机或外部输入模式时，这个设置不起作用
这是一个同步调用接口。

* **参数说明**
`micEnabled`:是否监听麦克风 true 监听，false 不监听。
`bgmEnabled`:是否监听背景音乐 true 监听，false 不监听。

* **返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 设置混响音效

* **语法**
```
int setReverbEnabled (boolean enabled);
```

* **功能**
设置是否开启混响音效，这个主要对主播/指挥有用。

* **参数说明**
`enabled`:true——打开，false——关闭。

* **返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 设置音频数据回调

```
void setPcmCallbackEnable(YouMeCallBackInterfacePcm callback, int flag, boolean outputToSpeaker, int nOutputSampleRate, int nOutputChannel);
```

* **功能**
设置是否开启音频pcm回调，以及开启哪种类型的pcm回调。
本接口在加入房间前调用。

* **参数说明**
`callback`:实现音频pcm回调的示例
`flag`:说明需要哪些类型的音频回调，共有三种类型的回调，分别是远端音频，录音音频，以及远端和录音数据的混合音频。flag格式形如`YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Romote| YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Record|YouMeConst.YouMePcmCallBackFlag.PcmCallbackFlag_Mix`；
`bOutputToSpeaker`: 是否扬声器静音:true 不静音;false 静音
`nOutputSampleRate`: 音频回调数据的采样率：8000,16000,32000,44100,48000; 具体参考 YOUME_SAMPLE_RATE类型定义
`nOutputChannel`: 音频回调数据的通道数：1 单通道；2 立体声

* **相关回调接口**

  ```
  //pcm回调接口位于YouMeCallBackInterfacePcm
  //以下3个回调分别对应于3种类型的音频pcm回调
  //开启后才会有
  
  //远端数据回调
  //channelNum:声道数
  //samplingRateHz:采样率
  //bytesPerSample:采样深度
  //data:pcm数据buffer
  void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
  //录音数据回调
  void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
  //远端和录音的混合数据回调
  void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, byte[] data);
  ```


## 设置时间戳

### 设置录音时间戳

* **语法**
```
void setRecordingTimeMs (int timeMs);
```

* **功能**
设置当前录音的时间戳。当通过录游戏脚本进行直播时，要保证观众端音画同步，在主播端需要进行时间对齐。
这个接口设置的就是当前游戏画面录制已经进行到哪个时间点了。

* **参数说明**
`timeMs`:当前游戏画面对应的时间点，单位为毫秒。

* **返回值**
无。


### 设置播放时间戳

* **语法**
```
void setPlayingTimeMs (int timeMs);
```

* **功能**
设置当前声音播放的时间戳。当通过录游戏脚本进行直播时，要保证观众端音画同步，游戏画面的播放需要和声音播放进行时间对齐。
这个接口设置的就是当前游戏画面播放已经进行到哪个时间点了。

* **参数说明**
`timeMs`:当前游戏画面播放对应的时间点，单位为毫秒。

* **返回值**
无。


## 设置服务器区域

* **语法**
```
void setServerRegion (int region, String extServerName, boolean bAppend);
```

* **功能**
设置首选连接服务器的区域码.

* **参数说明**
`region`：如果YOUME_RTC_SERVER_REGION定义的区域码不能满足要求，可以把这个参数设为 RTC_EXT_SERVER，然后通过后面的参数regionName 设置一个自定的区域值（如中国用 "cn" 或者 “ch"表示），然后把这个自定义的区域值同步给游密，我们将通过后台配置映射到最佳区域的服务器。
`extServerName`：自定义的扩展的服务器区域名。不能为null，可为空字符串“”。只有前一个参数serverRegionId设为RTC_EXT_SERVER时，此参数才有效（否则都将当空字符串“”处理）。
`bAppend`：true表示添加，false表示替换。

##  RestApi——支持主播相关信息查询

* **语法**
```
int requestRestApi(String strCommand, String strQueryBody);
```

* **功能**
Rest API , 向服务器请求额外数据。支持主播信息，主播排班等功能查询。详情参看文档<RequestRestAPI接口说明>


* **参数说明**
`requestID`：回传id,回调的时候传回，标识消息。
`strCommand`：请求的命令字符串。
`strQueryBody`：请求需要的数据,json格式。

* **返回值**
 返回值 >=0 表示requestID（回传id,回调的时候传回，标识消息）,返回值<0表示失败错误码（详见YouMeErrorCode定义）


* **异步回调**
```
//requestID:回传ID
//iErrorCode:错误码
//strQuery:回传查询请求，json格式。
//strResult:查询结果，json格式。
void onRequestRestAPI(int requestID, int iErrorCode, String strQuery, String strResult);
```

##  安全验证码设置

* **语法**
```
void setToken( String strToken);
```

* **功能**
设置身份验证的token，需要配合后台接口。

* **参数说明**
`strToken`：身份验证用token，设置为NULL或者空字符串，清空token值，不进行身份验证。

##  安全验证码设置,setToken后续将弃用

* **语法**
```
void setTokenV3( String strToken, long timeStamp );
```

* **功能**
设置身份验证的token，需要配合后台接口。

* **参数说明**
`strToken`：身份验证用token，设置为NULL或者空字符串，清空token值，不进行身份验证。
`timeStamp`：用户加入房间的时间，单位s。

##  查询频道用户列表

* **语法**
```
int  getChannelUserList( String strChannelID, int maxCount, boolean notifyMemChange );
```

* **功能**
查询频道当前的用户列表， 并设置是否获取频道用户进出的通知。（必须自己在频道中）

* **参数说明**
`strChannelID`：要查询的频道ID。
`maxCount`：想要获取的最大人数。-1表示获取全部列表。
`notifyMemChange`：其他用户进出房间时，是否要收到通知。

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//channelID:频道ID
//changeList:查询获得的用户列表，或变更列表。
//isUpdate: 是否是增量通知，刚进入频道会收到一次全量通知，为false，之后成员发生变化属于增量通知，为true
void onMemberChange(String channelID, MemberChange[] changeList, boolean isUpdate);
```

### 抢麦相关设置

* **语法**

```
int setGrabMicOption(String pChannelID, int mode, int maxAllowCount, int maxTalkTime, int voteTime);
```

* **功能**
抢麦相关设置（抢麦活动发起前调用此接口进行设置）

* **参数说明**
`pChannelID`：抢麦活动的频道id
`mode`：抢麦模式（1:先到先得模式；2:按权重分配模式）
`maxAllowCount`：允许能抢到麦的最大人数
`maxTalkTime`：允许抢到麦后使用麦的最大时间（秒）
`voteTime`：抢麦仲裁时间（秒），过了X秒后服务器将进行仲裁谁最终获得麦（仅在按权重分配模式下有效）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 发起抢麦活动

* **语法**

```
int startGrabMicAction(String pChannelID, String pContent);
```

* **功能**
抢麦相关设置（抢麦活动发起前调用此接口进行设置）

* **参数说明**
`pChannelID`：抢麦活动的频道id
`pContent`：游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 停止抢麦活动

* **语法**

```
int stopGrabMicAction(String pChannelID, String pContent);
```

* **功能**
停止抢麦活动

* **参数说明**
`pChannelID`：抢麦活动的频道id
`pContent`：游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 发起抢麦请求

* **语法**

```
int requestGrabMic(String pChannelID, int score, boolean isAutoOpenMic, String pContent);
```

* **功能**
停止抢麦活动

* **参数说明**
`pChannelID`：抢麦的频道id
`score`：积分（权重分配模式下有效，游戏根据自己实际情况设置）
`isAutoOpenMic`：抢麦成功后是否自动开启麦克风权限
`pContent`：游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 释放抢到的麦

* **语法**

```
int releaseGrabMic(String pChannelID);
```

* **功能**
释放抢到的麦

* **参数说明**
`pChannelID`：抢麦的频道id

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 连麦相关设置

* **语法**

```
int setInviteMicOption(String pChannelID, int waitTimeout, int maxTalkTime);
```

* **功能**
连麦相关设置（角色是频道的管理者或者主播时调用此接口进行频道内的连麦设置）

* **参数说明**
`pChannelID`：连麦的频道id
`waitTimeout`：等待对方响应超时时间（秒）
`maxTalkTime`：最大通话时间（秒）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 发起连麦请求

* **语法**

```
int requestInviteMic(String pChannelID, String pUserID, String pContent);
```

* **功能**
发起与某人的连麦请求（主动呼叫）

* **参数说明**
`pUserID`：被叫方的用户id
`pContent`：游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 回应连麦请求

* **语法**

```
int responseInviteMic(String pUserID, boolean isAccept, String pContent);
```

* **功能**
对连麦请求做出回应（被动应答）

* **参数说明**
`pUserID`：主叫方的用户id
`isAccept`：是否同意连麦
`pContent`：游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

### 停止连麦

* **语法**

```
int stopInviteMic();
```

* **功能**
停止连麦

* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

## 广播消息

* **语法**
```
int sendMessage( String  pChannelID, String pContent );
```

* **功能**
在语音频道内广播消息。


* **参数说明**
`pChannelID`：频道ID（自己需要进入这个频道）。
`pContent`：消息内容-文本串。

* **返回值**
负数表示错误码，正数表示回调requestID，用于标识这次消息发送

* **异步回调**
```
//event:YOUME_EVENT_SEND_MESSAGE_RESULT: 发送消息的结果回调，param为requestID的字符串
//event:YOUME_EVENT_MESSAGE_NOTIFY:频道内其他人收到消息的通知。param为文本内容
void onEvent(int event, int iErrorCode, String roomid, Object param);
```

## 点对点发送消息

* **语法**
```
int sendMessageToUser( String  pChannelID, String pContent, String pUserID  );
```

* **功能**
在语音频道内，向房间内用户发送消息。


* **参数说明**
`pChannelID`：频道ID（自己需要进入这个频道）。
`pContent`：消息内容-文本串。
`pUserID`：房间用户ID，如果为空，则表示向房间广播消息

* **返回值**
负数表示错误码，正数表示回调requestID，用于标识这次消息发送

* **异步回调**
```
//event:YOUME_EVENT_SEND_MESSAGE_RESULT: 发送消息的结果回调，param为requestID的字符串
//event:YOUME_EVENT_MESSAGE_NOTIFY:频道内其他人收到消息的通知。param为文本内容
void onEvent(int event, int iErrorCode, String roomid, Object param);
```


## 把人踢出房间

* **语法**
```
int  kickOtherFromChannel(  String pUserID, String  pChannelID,  int lastTime  );
```

* **功能**
把人踢出房间。


* **参数说明**
`pUserID`：被踢的用户ID。
`pChannelID`：从哪个房间踢出（自己需要在房间）。
`lastTime`：踢出后，多长时间内不允许再次进入。



* **返回值**
返回YOUME_SUCCESS才会有异步回调通知。其它返回值请参考[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。

* **异步回调**
```
//event:YOUME_EVENT_KICK_RESULT: 踢人方收到，发送消息的结果回调，param为被踢者ID
//event:YOUME_EVENT_KICK_NOTIFY: 被踢方收到，被踢通知，会自动退出所在频道。param: （踢人者ID，被踢原因，被禁时间）
void onEvent(int event, int iErrorCode, String roomid, Object param);
```

### 摄像头操作

#### 启动摄像头采集 
* **语法**

```java
public static int StartCapturer()
```
* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码

    
#### 停止摄像头采集
    
* **语法**

```java
public static void StopCapturer()
```

    
#### 切换前置/后置摄像头  


* **语法**

```java
public static int SwitchCamera()
```
* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
    
    
#### 设置启动摄像头（前/后） 
设置下一次调用startCapture时，采用前置摄像头采集还是后置摄像头。
默认为前置摄像头。

* **语法**

```java
public static void SetCaptureFrontCameraEnable(boolean enable)
```
* **参数说明**
    `enable`: true-前置摄像头，false-后置摄像头。


### 视频参数

#### 设置视频帧率   

* **语法**

```java
public static native int setVideoFps(int fps);
```

* **参数说明**
  `fps`: 帧率（3-30），默认15帧

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
    
#### 设置本地视频渲染回调的分辨率  

* **语法**

```java
public static native int setVideoLocalResolution(int width, int height);
```

* **参数说明**
    `width`: 宽
    `height`: 高

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
    
  
#### 设置网络传输分辨率
设置视频网络传输过程的分辨率（第一路高分辨率）。
接受方收到视频回调的分辨率，等于发送方设置的网络分辨率。

* **语法**

```java
public static native int  setVideoNetResolution( int width, int height );
```

* **参数说明**
    `width`: 宽
    `height`: 高

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
    
#### 设置视频数据上行的码率范围
进房间前设置。

* **语法**

```java
public static native void setVideoCodeBitrate(  int maxBitrate,   int minBitrate);
```

* **参数说明**
    `maxBitrate`: 最大码率，单位kbps
    `minBitrate`: 最小码率，单位kbps
    
    
#### 获取视频数据上行的当前码率

* **语法**

```java
public static native int getCurrentVideoCodeBitrate( );
```
    
* **返回值**

    视频数据上行的当前码率 
    
    
#### 设置VBR的编码模式   
设置是否使用VBR动态码率模式，允许一定范围的波动，可以提高移动时的画面稳定性。
需要在进入房间前设置。

* **语法**

```java
public static native int  setVBR( boolean useVBR );
```

* **参数说明**
    `useVBR`: 默认false，尽可能保持码率平稳，true 允许一定范围的波动，可以提高移动时的画面稳定性
    
* **返回值**

    错误码，YOUME_SUCCESS - 表示成功,其他 - 具体错误码 

#### 设置视频网络传大小流自动调整
设置视频网络传大小流自动调整。
需要在进入房间前设置。

* **语法**

```java
public static native int setVideoNetAdjustmode( int mode );
```

* **参数说明**
    `mode`: 0: 自动调整；1: 上层设置大小流接收选择
    
* **返回值**

    错误码，YOUME_SUCCESS - 表示成功,其他 - 具体错误码 

#### 设置是否同意开启硬编硬解
实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
进房间前设置。

* **语法**

```java
public static native void setVideoHardwareCodeEnable( boolean bEnable );
```

* **参数说明**
    `bEnable`: true:开启，false:不开启
    
#### 获取是否同意开启硬编硬解  
实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
进房间前设置。

* **语法**

```java
public static native boolean getVideoHardwareCodeEnable( );
```

* **返回值**
    true:开启，false:不开启
    
#### 设置视频数据等待超时时间  
设置无视频帧渲染的等待超时时间。连接中的视频，超过设置的timeout时间没有收到数据，会得到YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN通知。

* **语法**

```java
public static native void setVideoNoFrameTimeout(int timeout);
```

* **参数说明**
    `timeout`: 单位毫秒
    

### 渲染

#### 获取android下是否启用GL
获取android下是否启用GL

* **语法**

```java
boolean getUseGL( );

```
* **返回值**
    true:开启，false:不开启， 默认为true;

#### 设置渲染回调
回调的视频数据，会渲染到addRender设置的view上。
需要在任何addRender之前调用。

* **语法**

```java
api.SetVideoCallback();

```

#### 创建渲染

* **语法**

```java
//需要设置api.SetVideoCallback();
//VideoRenderer.getInstance().addRender
public int addRender(String userId, SurfaceViewRenderer view)
```

* **参数说明**
    `userId`: userId 用户ID
    `view`: 视频数据渲染的view
    
* **返回值**
    大于等于0 - renderId, 小于0 - 具体错误码  
    
* **回调**
    视频数据回调

#### 设置自己的userID

* **语法**

```java
//需要设置api.SetVideoCallback();
//VideoRenderer.getInstance().setLocalUserId
public void setLocalUserId(String userId) 
```

* **参数说明**
    `userId`:本端的userid。本地视频或者合流视频画面显示到用本地userId注册的视图上。

    
#### 删除渲染
* **语法**

```java
//需要设置api.SetVideoCallback();
//VideoRenderer.getInstance().deleteRender
public int deleteRender(String userId)

```

* **参数说明**
    `userId`: userId 用户ID
    
* **返回值**
    错误码，0 - 表示成功,其他 - 具体错误码

#### 删除所有渲染

* **语法**

```java
//需要设置api.SetVideoCallback();
//VideoRenderer.getInstance().deleteAllRender
public void deleteAllRender() 

```
#### EGLContext说明
  在对OpenGL开发的过程中，由于正常只使用一个线程来进行绘图操作，
这样有时会满足不了需求，所以需要加入多线程来进行操作。
但是如果创建两个独立的渲染线程，
这样却无法将绘图的信息同时显示在同一屏幕上，
这时就需要使用到OpenGL的共享上下文方案，
可以在两个线程中共享同一资源。

  在Android系统中，实现共享上下文很简单，
只需要在调用eglCreateContext的时候将上一个已经创建好的eglContext传入即可。

#### 设置OpenGL共享上下文

* **语法**

```java
public static Object setSharedEGLContext()
```    


#### 获取OpenGL共享上下文

* **语法**

```java
public static Object sharedEGLContext()
```    

#### 获取相机渲染纹理
获取SDK共享EGLContext线程创建的SurfaceTexture, 然后设置setOnFrameAvailableListener监听。

* **语法**

```java
public static SurfaceContext getCameraSurfaceTexture()
```    
    
#### 渲染视图
`SurfaceViewRenderer`

```java
//渲染一帧视频图像
//创建渲染之后会自动调用。如果要渲染别的数据，可以调用
public void renderFrame(VideoBaseRenderer.I420Frame frame)
//设置渲染的背景色
public void setRenderBackgroundColor( int r, int g, int b, int alpha  )
//清空渲染视图，显示背景色
public void clearImage()
```

### 高低两路视频流
SDK支持向服务器上传品质不同的两路流（不同的分辨率和码率），观看方根据自己的情况，设置拉取不同的流。
默认不上传第二路流。
    
#### 设置网络传输分辨率
设置视频上传第二路流的网络传输过程的分辨率。
接受方收到视频回调的分辨率，等于发送方设置的网络分辨率。
默认不传第二路流。如果设置了第二路流的分辨率，则会上传。

* **语法**

```java
public static int  setVideoNetResolutionForSecond( int width, int height );
```

* **参数说明**
    `width`: 宽
    `height`: 高

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
    
#### 设置视频数据上行码率范围
设置第二路流的上行码率范围。
进房间前设置。

* **语法**

```java
public static void setVideoCodeBitrateForSecond(  int maxBitrate,   int minBitrate);
```

* **参数说明**
    `maxBitrate`: 最大码率，单位kbps
    `minBitrate`: 最小码率，单位kbps
    
* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码 
    

### 设置音视频统计数据时间间隔

* **语法**

``` java
public static void setAVStatisticInterval(  int interval  );
```

* **功能**
设置音视频统计数据时间间隔，SDK会按这个间隔回调 `onAVStatistic`。

* **参数说明**
`interval`:时间间隔

* **返回值**
无。


### 设置Audio传输质量

* **语法**

```
public static void  setAudioQuality(  int quality  );
```

* **功能**
设置Audio的传输质量

* **参数说明**
`quality`:0: low 1: high

* **返回值**
无。

#### 设置视频使用质量稳定的编码模式   
设置第二路流是否使用VBR动态码率模式，允许一定范围的波动，可以提高移动时的画面稳定性。
需要在进入房间前设置。

* **语法**

```java
public static int  setVBRForSecond( boolean useVBR );
```

* **参数说明**
    `useVBR`: 默认false，尽可能保持码率平稳，true 允许一定范围的波动，可以提高移动时的画面稳定性
    
* **返回值**

    错误码，YOUME_SUCCESS - 表示成功,其他 - 具体错误码 
    
#### 查询多个用户视频信息
查询多个用户支持哪种流。

* **语法**

```java
public static int queryUsersVideoInfo(String []userArray);
```

* **参数说明**
    `userArray`: 用户ID列表

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码 
    
* **回调**

```java
//event:YOUME_EVENT_QUERY_USERS_VIDEO_INFO
public void onEvent (int event, int error, String room, Object param)
```
    
#### 设置观看用户哪路流
设置观看用户的哪一路流。如果设置了不支持的流，则采用默认的第一路流。

* **语法**

```java
public static int setUsersVideoInfo(String [] userArray, int [] resolutionArray);
```

* **参数说明**
    `userArray`: 用户ID列表
    `resolutionArray`: 用户对应分辨率列表(每一项为0-第一路流/1-第二路流)

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码


### 外部采集模式
#### 设置是否由外部输入音视频
默认使用内部采集。
如果使用外部采集，需要自己采集音视频，然后把数据传入SDK。
在Init之前调用。

* **语法**

```java
public static void setExternalInputMode( boolean bInputModeEnabled );
```

* **参数说明**

  `bInputModeEnabled`: true:外部输入模式，false:SDK内部采集模式


### 设置外部输入模式的语音采样率

* **语法**

``` java
public static int  setExternalInputSampleRate(  int inputSampleRate, int mixedCallbackSampleRate  );
```

* **功能**
设置外部输入模式的语音采样率

* **参数说明**
`inputSampleRate`:输入语音采样率, 具体参考 YOUME_SAMPLE_RATE类型定义
`mixedCallbackSampleRate`:mix后输出语音采样率, 具体参考 YOUME_SAMPLE_RATE类型定义

* **返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。


#### 输入音频数据（支持单声道——待废弃）
* **语法**

```java
public  static boolean inputAudioFrame(byte[] data, int len, long timestamp)
```

* **参数说明**
   `data`: 指向PCM数据的缓冲区
	`len`: 音频数据的大小
	`timestamp`: 时间戳，单位毫秒

* **返回值**	
	成功/失败

#### 输入音频数据（扩展）
* **语法**

```java
public  static boolean inputAudioFrameEx(byte[] data, int len, long timestamp, int channelnum, boolean binterleaved)
```

* **参数说明**
   `data`: 指向PCM数据的缓冲区
	`len`: 音频数据的大小
	`timestamp`: 时间戳，单位毫秒
	`channelnum`: 声道数，1:单声道，2:双声道，其它非法
	`binterleaved`: 音频数据打包格式（仅对双声道有效）

* **返回值**	
	成功/失败
	
	
#### 输入视频数据
视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)


* **语法**

```java
public  boolean inputVideoFrame(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp)
```

* **参数说明**
  `data`: 视频帧数据
  `len`: 视频数据大小
  `width`: 视频图像宽
  `height`: 视频图像高
  `fmt`: 视频格式
  `rotation`: 视频旋转角度
  `mirror`: 是否镜像
  `timestamp`: 时间戳，单位毫秒

* **返回值**	
	成功/失败
	
    #### 输入加密视频数据，支持大小流
    视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)


    * **语法**

    ```java
    public  boolean inputVideoFrameEncrypt(byte[] data, int len, int width, int height, int fmt, int rotation, int mirror, long timestamp, int streamID)
    ```

    * **参数说明**
      `data`: 视频帧数据
      `len`: 视频数据大小
      `width`: 视频图像宽
      `height`: 视频图像高
      `fmt`: 视频格式
      `rotation`: 视频旋转角度
      `mirror`: 是否镜像
      `timestamp`: 时间戳，单位毫秒
      `streamID`: 表示大小流ID

    * **返回值**    
        成功/失败
        
#### 输入视频数据2

* **语法**

```java
public static boolean inputVideoFrameGLES(int texture, float[] matrix, int width, int height, int fmt, int rotation, int mirror, long timestamp) 
```

* **参数说明**
  `texture`: 纹理ID， 必须使用sdk共享eglContext创建
  `matrix`: texture矩阵坐标，可为null
  `float`: 视频数据大小
  `width`: 视频图像宽
  `height`: 视频图像高
  `fmt`: 视频格式
  `rotation`: 视频旋转角度
  `mirror`: 是否镜像
  `timestamp`: 时间戳，单位毫秒

* **返回值**	
	成功/失败
	
#### 停止视频输入
停止视频数据输入(在inputVideoFrame之后调用，房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP事件)

* **语法**

```java
public void stopInputVideoFrame()
```

### 视频合流
#### 设置合流总尺寸
把远端的视频和本地自己的视频合到一个画面，称为合流。本接口设置合流画面总的尺寸。

* **语法**

```java
public static void setMixVideoSize(int width, int height)
```

* **参数说明**

    `width`: 宽
    `height`: 高

#### 设置user的合流位置 
设置user的视频数据在合流画面中展现的位置和尺寸。

* **语法**

```java
public static void addMixOverlayVideo(String userId, int x, int y, int z, int width, int height)
```

* **参数说明**

    `userId`: 宽
    `x`: x坐标
    `y`: y坐标
    `z`: z坐标，影响视频的展示层级
    `width`: 宽
    `height`: 高

#### 取消user的合流
取消对user的合流。

```java
public static void removeMixOverlayVideo(String userId)
```

* **参数说明**

    `userId`:  user ID 
    
#### 取消所有user的合流
取消对所有user的合流。

```objectivec
public static void removeMixAllOverlayVideo()
```

#### 设置合流回调外部
混流后视频数据已YUV420P格式回调到外部

```java
public static boolean setOpenVideoMixerYUVCallBack(boolean enabled)
```

* **参数说明**

    `enabled`:  true-打开，false-关闭
 
#### 视频回调

* **语法**

```java
//interface VideoMgr.VideoFrameCallback
//设置回调： api.setVideoFrameCallback( myCallback )
public void onVideoFrameCallback(String userId, byte[] data, int len, int width, int height, int fmt, long timestamp)
```

* **参数说明**

    `userId`: userID 
    `data`: 视频帧数据
    `len`: 视频数据大小
    `width`: 视频图像宽
    `height`: 视频图像高
    `fmt`: 视频格式，参看YouMeConst.YOUME_VIDEO_FMT
    `timestamp`: 时间戳，单位毫秒   
    
#### 视频回调2

* **语法**

```java
public void onVideoFrameCallbackGLES(String userId, int fmt, int texture, float[] matrix, int width, int height, long timestamp)
```

* **参数说明**

    `userId`: userID 
    `fmt`:视频格式，参看YouMeConst.YOUME_VIDEO_FMT
    `texture`: 纹理ID
    `matrix`: texture矩阵坐标
    `width`: 视频图像宽
    `height`: 视频图像高
    `fmt`: 视频格式
    `timestamp`: 时间戳，单位毫秒   


#### 合流视频回调

* **语法**

```java
//interface VideoFrameCallback
//设置回调： api.setVideoFrameCallback( myCallback )
public void onVideoFrameMixed(byte[] data, int len, int width, int height, int fmt, long timestamp)
```

* **参数说明**

    `data`: 视频帧数据
    `len`: 视频数据大小
    `width`: 视频图像宽
    `height`: 视频图像高
    `fmt`: 视频格式，参看YouMeConst.YOUME_VIDEO_FMT
    `timestamp`: 时间戳，单位毫秒
    
#### 合流视频回调2
* **语法**

```java
//interface VideoFrameCallback
//设置回调： api.setVideoFrameCallback( myCallback )
public void onVideoFrameMixedGLES(int fmt, int texture, float[] matrix, int width, int height, long timestamp)
```

* **参数说明**

    `fmt`:视频格式，参看YouMeConst.YOUME_VIDEO_FMT
    `texture`: 纹理ID
    `matrix`: texture矩阵坐标
    `width`: 视频图像宽
    `height`: 视频图像高
    `timestamp`: 时间戳，单位毫秒


#### 音频回调
* **语法**

```java
//interface YouMeAudioCallbackInterface
//设置回调： api.setAudioFrameCallback( myCallback )
- (void)onAudioFrameCallback: (NSString*)userId data:(void*) data len:(int)len timestamp:(uint64_t)timestamp;

```

* **参数说明**

    `userId`: userID 
    `data`: 视频帧数据
    `len`: 视频数据大小
    `timestamp`: 时间戳，毫秒

#### 合流音频回调
* **语法**

```java
//interface YouMeAudioCallbackInterface
//设置回调： api.setAudioFrameCallback( myCallback )
- (void)onAudioFrameMixedCallback: (void*)data len:(int)len timestamp:(uint64_t)timestamp;

```

* **参数说明**
    `data`: 视频帧数据
    `len`: 视频数据大小
    `timestamp`: 时间戳，毫秒

### 美颜
内置美颜

#### 开启美颜  
打开摄像头之前调用

* **语法**

```java
public static void openBeautify(boolean open)
```

* **参数说明**
    `open`: true-开启美颜，false-关闭美颜 


    
#### 设置美颜强度  
可随时调节

* **语法**

```java
public static void setBeautyLevel(float level) ;
```

* **参数说明**
    `level`: 美颜参数，0.0 - 1.0 ，默认为0，几乎没有美颜效果，0.5左右效果明显


### 自定义滤镜

#### 设置回调  
设置视频纹理自定义处理回调

* **语法**

```java
public static void setVideoMixerFilterListener(VideoMixerFilterListener listener)
```

* **参数说明**
    `listener`: 回调类对象，详细请参考VideoMixerFilterListener


### 自定义数据类型

#### 设置回调

* **语法**

```java
public static void setRecvCustomDataCallback (YouMeCustomDataCallbackInterface callback)
```

* **参数说明**
    `callback`: 收到其它人自定义数据的回调对象，详细请参考YouMeCustomDataCallbackInterface

 
#### 数据回调

* **语法**

```java
public void onRecvCustomData(byte[] data, long timestamp);
```

* **参数说明**

    `data`: 视频帧数据
    `timestamp`: 时间戳，单位毫秒   


#### 输入自定义数据

* **语法**

```java
public static int inputCustomData(byte[] data,int len,long timestamp)
```

* **参数说明**
    `data`: 自定义数据，要广播的自定义数据
    `len`: 数据长度，不能大于1024
    `timestamp`: 时间戳

#### 点对点输入自定义数据

* **语法**

```java
public static int inputCustomDataToUser(byte[] data,int len,long timestamp, String userId)
```

* **参数说明**
    `data`: 自定义数据，要广播的自定义数据
    `len`: 数据长度，不能大于1024
    `timestamp`: 时间戳
    `userId`: 接收端用户

### 其他操作
#### 屏蔽他人视频

* **语法**

```java
public static void maskVideoByUserId(String userId, boolean mask);
```

* **参数说明**
    `userId`: 用户ID
    `mask`: true - 屏蔽, false - 恢复不屏蔽

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码

    
#### 翻译

* **语法**

```java
public static native int translateText(  String text, int destLangCode, int srcLangCode);
```

* **功能**
翻译一段文字为指定语言。

* **参数说明**
`text`: 要翻译的内容。
`destLangCode`:要翻译成什么语言。
`srcLangCode`:要翻译的是什么语言。

* **返回值**
如果返回值<=0，则表示错误码。具体参见[YouMeErrorCode类型定义](/doc/TalkStatusCode.html#youmeerrorcode类型定义)。如果>0，则表示返回本次请求的requestID( 翻译请求的ID，用于在回调中确定翻译结果是对应哪次请求)。

* **回调**

```c++
//errorcode：错误码
//requestID：请求ID（与translateText接口输出参数requestID一致）
//text：翻译结果
//srcLangCode：源语言编码
//destLangCode：目标语言编码
public  void onTranslateTextComplete( int errorcode, int requestID, String text, int srcLangCode, int destLangCode );
```



### 回调数据
#### 设置视频数据回调方式
硬编解码默认回调opengl纹理方式，使用该方法可以回调yuv格式

* **语法**

```java
public static native int setVideoFrameRawCbEnabled(boolean enabled);
```

* **参数说明**
    `enabled`: true 打开，false 关闭，默认关闭

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码


### 相机设置
#### 获取摄像头是否支持变焦

* **语法**

```java
public static native boolean isCameraZoomSupported();
```

* **参数说明**
    

* **返回值**

    true 支持，false 不支持

#### 设置摄像头变焦

* **语法**

```java
public static native float setCameraZoomFactor(float zoomFactor);
```

* **参数说明**
    `zoomFactor`: 1.0 表示原图像大小， 大于1.0 表示放大，默认1.0f

* **返回值**

    设置后当前缩放比例


#### 获取摄像头是否支持自定义坐标对焦

* **语法**

```java
public static native boolean isCameraFocusPositionInPreviewSupported();
```

* **参数说明**
    

* **返回值**

    true 支持，false 不支持


#### 设置摄像头自定义对焦坐标
左上角坐标（0.0,1.0）,右下角（1.0,0.0）,默认（0.5,0.5）
* **语法**

```java
public static native boolean setCameraFocusPositionInPreview( float x , float y );
```

* **参数说明**
    `x`: 横坐标, 0.0f-1.0f
    `y`: 纵坐标, 0.0f-1.0f
 
* **返回值**

    true 成功，false 失败


#### 获取摄像头是否支持闪光灯

* **语法**

```java
public static native boolean isCameraTorchSupported();
```

* **参数说明**
    

* **返回值**

    true 支持，false 不支持

#### 设置摄像头打开关闭闪光灯

* **语法**

```java
public static native boolean setCameraTorchOn(boolean isOn );
```

* **参数说明**
    `isOn`: true 打开，false 关闭

 
* **返回值**

    true 成功，false 失败


#### 获取摄像头是否自动人脸对焦

* **语法**

```java
public static native boolean isCameraAutoFocusFaceModeSupported();
```

* **参数说明**
    

* **返回值**

    true 支持，false 不支持

#### 设置摄像头打开关闭人脸对焦

* **语法**

```java
public static native boolean setCameraAutoFocusFaceModeEnabled(boolean enabled);
```

* **参数说明**
    `enabled`: true 打开，false 关闭

 
* **返回值**

    true 成功，false 失败



##  反初始化

* **语法**
```
int unInit ();
```

* **功能**
反初始化引擎，可在退出游戏时调用，以释放SDK所有资源。

* **返回值**
如果成功则返回YOUME_SUCCESS，其它具体参见[YouMeErrorCode类型定义](/doc/TalkAndroidStatusCode.php#YouMeErrorCode类型定义)。



## 录屏模块接口(ScreenRecorder)
该录屏接口要求系统Android 5.0及以上，在录屏开始时需要申请录屏权限，用户允许后开始录屏，由于系统隐私设置，录屏数据通过录屏数据接口交给SDK
### 录屏初始化
* **语法**
```java
public static void init(Context env)
```

* **参数说明**
    `env`: activity上下文变量

### 录屏分辨率设置
* **语法**
```java
public static void setResolution(int width, int height)
```

### 录屏帧率设置
* **语法**
```java
public static void setFps(int fps)
```

### 开始录屏
* **语法**
```java
public static boolean startScreenRecorder() 
```

* **参数说明**

* **返回值**    
   true 成功，false 失败

### 停止录屏
* **语法**
```java
public static boolean stopScreenRecorder()
```

* **参数说明**

* **返回值**    
   true 成功，false 失败

### 录屏数据接口
* **语法**
```java
protected void onActivityResult(int requestCode, int resultCode, Intent data)
```
* **参考demo**
``` 
    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if(requestCode == ScreenRecorder.SCREEN_RECODER_REQUEST_CODE) {
            if (resultCode == Activity.RESULT_OK) {
                ScreenRecorder.onActivityResult(requestCode, resultCode, data);
            }else {
                isOpenShare = false;
            }
        }
    }
```

## 局域网传输接口
### 设置局域网信息
* **语法**
```java
public static native int setLocalConnectionInfo(String pLocalIP, int iLocalPort, String pRemoteIP, int iRemotePort)
```

* **参数说明**

    `pLocalIP`: 本端ip
    `iLocalPort`: 本端数据端口
    `pRemoteIP`: 远端ip
    `iRemotePort`: 远端数据端口

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码

### 设置P2P连接异常时是否切换server转发
* **语法**
```java
public static native int setRouteChangeFlag(boolean enable)
```

* **参数说明**
    `enable`: 设置是否切换server通路标志

* **返回值**

    错误码，0 - 表示成功,其他 - 具体错误码
