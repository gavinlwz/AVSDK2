# Video SDK for Android 快速接入

## 概述
游密实时语音SDK（Video SDK）是游密科技公司旗下的一款专注于为开发者提供实时语音技术和服务的云产品。我们的研发团队来自腾讯，其中不少是拥有10年以上音视频经验的专家，专业专注；我们的服务端节点部署遍布全球，为用户提供高效稳定的实时云服务，且弹性可扩展。通过Video SDK，能够让您的应用轻松拥有多人实时视频通话的能力，可广泛应用于社交、游戏、在线教育、视频会议等场景。支持一对一、多人实时视频互动，打破屏幕阻隔，还原最纯粹的面对面聊天场景。

## 四步集成

### Step1：注册账号
在[游密官网](https://console.youme.im/user/register)注册游密账号。

![](https://youme.im/doc/images/talk_sdk_android_access_1.png)

### Step2：添加游戏，获取`Appkey`
在控制台添加游戏，添加成功后 在网页左边的游戏栏会增加一个对应的应用，点击进去就可以获得接入需要的**Appkey**、**Appsecret**。
![](https://youme.im/doc/images/talk_sdk_android_access_3.png)

![](https://youme.im/doc/images/talk_sdk_android_access_4.png)

### Step3：下载Video SDK包体
根据游戏使用的游戏引擎与开发语言，在[下载入口](https://www.youme.im/download.php?type=Talk)下载对应的SDK包体。

### Step4：开发环境配置
[开发环境配置](#快速接入)

## 快速接入
### Android Studio开发环境集成
1.  将SDK内的lib文件夹的所有文件移至Android工程libs文件夹下（可视实际情况自行放置），如下图所示：

  ![](https://youme.im/doc/images/android_libs_view.png)

2. AndroidManifest.xml配置
#### 添加录音和网络相关权限
  ``` xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.ACCESS_WIFI_STATE" />
<uses-permission android:name="android.permission.ACCESS_NETWORK_STATE" />
<uses-permission android:name="android.permission.CHANGE_WIFI_STATE" />
<uses-permission android:name="android.permission.CHANGE_NETWORK_STATE" />
<uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE" />
<uses-permission android:name="android.permission.RECORD_AUDIO" />
<uses-permission android:name="android.permission.MODIFY_AUDIO_SETTINGS" />
<uses-permission android:name="android.permission.READ_PHONE_STATE" />
<uses-permission android:name="android.permission.BLUETOOTH" />
<!-- video -->
<uses-feature android:name="android.hardware.camera" />
<uses-feature android:name="android.hardware.camera.autofocus" />

<uses-permission android:name="android.permission.CAMERA" />
<uses-permission android:name="android.permission.CAPTURE_VIDEO_OUTPUT" />
  ```
3. 设置加载so库以及启动相关服务
  在入口Activity类先导入package:   
  ```java
  import android.content.Intent;
  import android.os.Bundle;
  import com.youme.voiceengine.mgr.YouMeManager;
  ```
  然后在onCreate方法里添加如下代码:
  ```java
  YouMeManager.Init(this);
  super.onCreate(savedInstanceState);
  ```
  
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


### 设置回调监听  
引擎底层对于耗时操作都采用异步回调的方式，函数调用会立即返回，操作结果java层会同步回调。因此，用户必须实现相关接口并在初始化前通过 `api.SetCallback(this)` 注册接收回调通知的对象。

使用类需要implements接口YouMeCallBackInterface，并实现该接口下的所有回调函数。回调都在子线程中执行，不能用于更新UI等耗时操作。

*  首先要导入相关的包：

```java 
  import com.youme.voiceengine.MemberChange;
  import com.youme.voiceengine.YouMeCallBackInterface;
```

* 然后implements接口YouMeCallBackInterface，具体实现回调方法：

``` java
  @Override
  public void onEvent(int eventType, int iErrorCode, String roomid, Object param){}
  @Override
  public void onRequestRestAPI(int requestID, int iErrorCode, String strQuery, String strResult){}
  @Override
  public void onMemberChange(String channelID, MemberChange[] arrChanges, boolean isUpdate){}
  @Override
  public  void onBroadcast(int bc , String room, String param1, String param2, String content){}
  @Override
  public  void onAVStatistic( int avType,  String userID, int value ){}
```
