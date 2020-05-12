# Video SDK for Android 典型场景实现方法

## 实现视频通话 ##

### 相关接口

`api.init` 引擎初始化
`api.setVideoLocalResolution` 设置本地采集分辨率
`api.setVideoNetResolution` 设置网络传输分辨率
`api.joinChannelSingleMode` 加入房间
`VideoRenderer.getInstance().addRender` 创建渲染
`api.SetVideoCallback` 激活视频回调绑定
`api.startCapture` 开始摄像头采集
`api.setMicrophoneMute`	设置麦克风状态
`api.setSpeakerMute`	设置扬声器状态

### 关键调用顺序

1. 调用初始化（api.init），等待 `YOUME_EVENT_INIT_OK` 事件
2. 加入房间（api.joinChannelSingleMode），等待加入成功的通知事件 `YouMeConst.YouMeEvent.YOUME_EVENT_JOIN_OK`
3. 激活视频回调绑定 `api.SetVideoCallback()`
4. 打开摄像头（api.startCapture），设置麦克风扬声器等设备（api.setMicrophoneMute，api.setSpeakerMute）
5. 接收到视频数据回调 `YouMeConst.YouMeEvent.YOUME_EVENT_OTHERS_VIDEO_ON`，绑定渲染（VideoRenderer.getInstance().addRender）
