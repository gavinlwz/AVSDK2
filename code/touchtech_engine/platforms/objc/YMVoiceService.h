//
//  YMVoiceService.h
//  YmTalkTestRef
//
//  Created by pinky on 2017/5/27.
//  Copyright © 2017年 Youme. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "VoiceEngineCallback.h"
#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#import <ReplayKit/ReplayKit.h>
#elif TARGET_OS_OSX
#import <AppKit/AppKit.h>
#endif
#import "OpenGLESView.h"

@interface YMVoiceService : NSObject
@property (nonatomic, weak) id<VoiceEngineCallback> delegate;

//公共接口
+ (YMVoiceService *)getInstance;
+ (void)destroy;

- (void)setTestServer:(bool) isTest;
- (void)setServerMode:(int) mode;
- (void)setServerIP:(NSString*)ip port:(int)port;

/**
 *  功能描述:   设置用户自定义Log路径
 *  @param path 文件的路径
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t)setUserLogPath:(NSString *)path;

/**
 *  功能描述:   设置是否由外部输入音视频
 *  @param bInputModeEnabled true:外部输入模式，false:SDK内部采集模式
 */
- (void)setExternalInputMode:(bool)bInputModeEnabled;

/**
 *  功能描述:设置是否使用TCP模式来收发数据，针对特殊网络没有UDP端口使用，必须在加入房间之前调用
 *  @param bUseTcp 是否使用。
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t)setTCPMode:(bool)bUseTcp;

/**
 *  功能描述:初始化引擎
 *
 *  @param delegate 回调地址
 *  @param appKey 在申请SDK注册时得到的App Key，也可凭账号密码到http://gmx.dev.net/createApp.html查询
 *  @param appSecret 在申请SDK注册时得到的App Secret，也可凭账号密码到http://gmx.dev.net/createApp.html查询
 *  @param regionId 可选的服务器区域
 *  @param serverRegionName , 可选扩展字段, 如不清楚，传入: @""
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t)initSDK:(id<VoiceEngineCallback>)delegate  appkey:(NSString*)appKey  appSecret:(NSString*)appSecret
        regionId:(YOUME_RTC_SERVER_REGION_t)regionId
           serverRegionName:(NSString*) serverRegionName ;


/**
 *  功能描述:设置身份验证的token
 *  @param token 身份验证用token，设置空字符串，清空token值。
 */
- (void)setToken:(NSString*) token;

/**
 *  功能描述:设置身份验证的token和用户加入房间的时间
 *  @param token 身份验证用token，设置空字符串，清空token值。
 *  @param timeStamp 用户加入房间的时间，单位s
 */
- (void)setTokenV3:(NSString*) token timeStamp: (unsigned int)timeStamp;

/**
 *  功能描述:反初始化引擎
 *
 *  @return 错误码
 */
- (YouMeErrorCode_t)unInit;

/**
 *  功能描述:设置服务器区域
 *  @param serverRegionId 服务器区域, 用YOUME_RTC_SERVER_REGION_t枚举值
 *  @param regionName 可选扩展字段, 如不清楚，传入: @""
 *  @param bAppend 是否追加模式，特殊情况下多区域适配时使用
 */
-(void)setServerRegion:(YOUME_RTC_SERVER_REGION_t)serverRegionId regionName:(NSString*)regionName bAppend:(bool)bAppend;

/**
 *  功能描述:切换语音输出设备
 *  默认输出到扬声器，在加入房间成功后设置（iOS受系统限制，如果已释放麦克风则无法切换到听筒）
 *
 *  @param bOutputToSpeaker true——使用扬声器，false——使用听筒
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t)setOutputToSpeaker:(bool)bOutputToSpeaker;

/**
 *  功能描述:设置扬声器静音
 *
 *  @param mute true——静音，false——取消静音
 */
-(void)setSpeakerMute:(bool)mute;

/**
 *  功能描述:获取扬声器静音状态
 *
 *  @return true——静音，false——没有静音
 */
-(bool)getSpeakerMute;

/**
 *  功能描述:获取麦克风静音状态
 *
 *  @return true——静音，false——没有静音
 */
-(bool)getMicrophoneMute;

/**
 *  功能描述:设置麦克风静音
 *
 *  @param mute true——静音，false——取消静音
 */
-(void)setMicrophoneMute:(bool)mute;

/**
 *  功能描述:设置本地连接信息，用于p2p传输，本接口在join房间之前调用
 *
 *  @param strLocalIP:本端ip
 *  @param localPort:本端数据端口
 *  @param remoteIP:远端ip
 *  @param remotePort:远端数据端口
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
-(int)setLocalConnectionInfo:(NSString *)strLocalIP localPort:(int)iLocalPort remoteIP:(NSString *)strLocalRemoteIP remotePort:(int)iRemotePort;

/**
 *  功能描述:设置是否切换server通路
 *
 *  @param enable: 设置是否切换server通路标志
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
-(int)setRouteChangeFlag:(bool)enable;

/**
 *  功能描述:设置本地预览视图
 *
 *  @param parentView:渲染父视图
 *  @return 返回渲染视图OpenGLESView对象
 */
#if TARGET_OS_IPHONE
-(UIView*) setLocalRender:(UIView*)parentView;
#elif TARGET_OS_OSX
-(NSView*) setLocalRender:(NSView*)parentView;
#endif
/**
 *  功能描述:创建渲染
 *
 *  @param userId:用户ID
 *  @param parentView:渲染父视图
 *  @return 返回渲染视图OpenGLESView对象
 */
#if TARGET_OS_IPHONE
-(UIView*) createRender:(NSString*) userId parentView:(UIView*)parentView singleMode:(BOOL)singleMode;
#elif TARGET_OS_OSX
-(NSView*) createRender:(NSString*) userId parentView:(NSView*)parentView singleMode:(BOOL)singleMode;
#endif

/**
 *  功能描述:删除渲染
 *
 *  @param userId 用户ID
 */
-(void) deleteRender:(NSString*) userId glView:(OpenGLESView*)glView;

/**
 *  功能描述:清除渲染背景
 *
 *  @param userId 用户ID
 */
-(void) cleanRender:(NSString*) userId;

/**
 *  功能描述:删除所有渲染
 *
 */
-(void) deleteAllRender;

/**
 *  功能描述:获取当前绑定的render 数量
 *
 */
-(int)getRenderCount:(NSString*) userId;

/**
 *  功能描述：设置渲染模式
 */
- (int)setRenderMode:(NSString*)userId mode:(YouMeVideoRenderMode_t) mode;

/**
 *  功能描述:设置是否通知其他人自己的开关麦克风和扬声器的状态
 *
 *  @param bAutoSend true——通知，false——不通知
 */
-(void)setAutoSendStatus:(bool)bAutoSend;

/**
 *  功能描述:获取音量大小,此音量值为程序内部的音量，与系统音量相乘得到程序使用的实际音量
 *
 *  @return 音量值[0,100]
 */
- (unsigned int)getVolume;
/**
 *  功能描述:增加音量，音量数值加1
 *
 */
- (void)setVolume:(unsigned int)uiVolume;

/**
 *  功能描述:启用/禁用移动网络
 *
 *  @param bEnabled true-可以启用，false-禁用，默认禁用
 *
 */
- (void)setUseMobileNetworkEnabled:(bool)bEnabled;

/**
 *  功能描述:是否可使用移动网络
 *
 *  @return true-可以使用，false-禁用
 */
- (bool) getUseMobileNetworkEnabled;

//多人语音接口
/**
 *  功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param strChannelID 频道ID，要保证全局唯一
 *  @param userRole 用户角色，用于决定讲话/播放背景音乐等权限
 *  @param bVideoAutoRecv 进入房间后是否自动接收视频
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) joinChannelSingleMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole_t)userRole autoRecv:(bool)autoRecv;

/**
 *  功能描述：加入语音频道（多频道模式，可以同时在多个语音频道里面）
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param strChannelID 频道ID，要保证全局唯一
 *  @param userRole 用户角色，用于决定讲话/播放背景音乐等权限
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) joinChannelMultiMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole_t)userRole;

/**
 *  功能描述:加入语音频道（单频道模式，每个时刻只能在一个语音频道里面）
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param strChannelID 频道ID，要保证全局唯一
 *  @param userRole 用户角色，用于决定讲话/播放背景音乐等权限
 *  @param joinAppKey 加入房间用额外的appkey
 *  @param bVideoAutoRecv 进入房间后是否自动接收视频
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) joinChannelSingleMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole_t)userRole  joinAppKey:(NSString*) joinAppKey autoRecv:(bool)autoRecv;

/**
 *  功能描述：对指定频道说话
 *
 *  @param strChannelID 频道ID，要保证全局唯一
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) speakToChannel:(NSString *)strChannelID;

//退出音频会议
/**
 *  功能描述:退出多频道模式下的某个语音频道
 *
 *  @param strChannelID 频道ID，要保证全局唯一
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t)leaveChannelMultiMode:(NSString *)strChannelID;

/**
 *  功能描述:退出所有语音频道
 *
 *  @return 错误码
 */
- (YouMeErrorCode_t)leaveChannelAll;



/**
 *  功能描述:获取SDK 版本号
 *
 *
 *  @return 整形数字版本号
 */
- (int)getSDKVersion;

/**
 *  功能描述:控制其他人的麦克风开关
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param mute true 静音对方的麦克风，false 取消静音对方麦克风
 *
 */
-(void) setOtherMicMute:(NSString *)strUserID  mute:(bool) mute;

/**
 *  功能描述:控制其他人的扬声器开关
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param mute true 静音对方的扬声器，false 取消静音对方扬声器
 *
 */
-(void) setOtherSpeakerMute: (NSString *)strUserID  mute:(bool) mute;

/**
 *  功能描述:选择消除其他人的语音
 *
 *  @param strUserID 用户ID，要保证全局唯一
 *  @param isOn false屏蔽对方语音，true取消屏蔽
 *
 */
-(void) setListenOtherVoice: (NSString *)strUserID  isOn:(bool) isOn;

/**
 *  功能描述: 播放背景音乐，并允许选择混合到扬声器输出麦克风输入
 *  @param path 音乐文件的路径
 *  @param repeat 是否重复播放
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)playBackgroundMusic:(NSString *)path  repeat:(bool)repeat;

/**
 *  功能描述: 如果当前正在播放背景音乐的话，暂停播放
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)pauseBackgroundMusic;

/**
 *  功能描述: 如果当前背景音乐处于暂停状态的话，恢复播放
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)resumeBackgroundMusic;

/**
 *  功能描述: 如果当前正在播放背景音乐的话，停止播放
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)stopBackgroundMusic;

/**
 *  功能描述: 设置背景音乐播放的音量
 *  @param bgVolume 背景音乐的音量，范围 0-100
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setBackgroundMusicVolume:(unsigned int)bgVolume;

/**
 *  功能描述: 设置是否用耳机监听自己的声音，当不插耳机或外部输入模式时，这个设置不起作用
 *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setHeadsetMonitorMicOn:(bool)micEnabled;

/**
 *  功能描述: 设置是否用耳机监听自己的声音或背景音乐，当不插耳机或外部输入模式时，这个设置不起作用
 *  @param micEnabled 是否监听麦克风 true 监听，false 不监听
 *  @param bgmEnabled 是否监听背景音乐 true 监听，false 不监听
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setHeadsetMonitorMicOn:(bool)micEnabled BgmOn:(bool)bgmEnabled;


/**
 *  功能描述: 设置是否开启主播混响模式
 *  @param enabled true 开启，false 关闭
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setReverbEnabled:(bool)enabled;

/**
 *  功能描述: 设置是否开启语音检测回调
 *  @param enabled true 开启，false 关闭
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVadCallbackEnabled:(bool)enabled;

/**
 *  功能描述: 设置是否开启讲话音量回调, 并设置相应的参数
 *  @param maxLevel 音量最大时对应的级别，最大可设100。根据实际需要设置小于100的值可以减少回调的次数。
 *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
 *                   设 0 表示关闭回调。
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setMicLevelCallback:(int) maxLevel;

/**
 *  功能描述: 设置是否开启远端说话人音量回调, 并设置相应的参数
 *  @param maxLevel 音量最大时对应的级别，最大可设100。
 *                   比如你只在UI上呈现10级的音量变化，那就设10就可以了。
 *                   设 0 表示关闭回调。
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setFarendVoiceLevelCallback:(int) maxLevel;

/**
 *  功能描述: 设置当麦克风静音时，是否释放麦克风设备，在初始化之后、加入房间之前调用
 *  @param enabled
 *      true 当麦克风静音时，释放麦克风设备，此时允许第三方模块使用麦克风设备录音。在Android上，语音通过媒体音轨，而不是通话音轨输出。
 *      false 不管麦克风是否静音，麦克风设备都会被占用。
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setReleaseMicWhenMute:(bool) enabled;

/**
 *  功能描述: 设置插入耳机时，是否自动禁用手机系统硬件信号前处理
 *          系统提供的前处理效果包括回声消除、自动增益等，有助于抑制背景音乐等回声噪音，减少系统资源消耗
 *          由于插入耳机可从物理上阻断回声产生，故可设置禁用该效果以保留背景音乐的原生音质效果
 *          默认为false，在初始化之后、加入房间之前调用
 *          注：Windows和macOS不支持该接口
 *  @param enabled
 *      true 当插入耳机时，自动禁用系统硬件信号前处理，拔出时还原
 *      false 插拔耳机不做处理
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setExitCommModeWhenHeadsetPlugin:(bool) enabled;

/**
 *  功能描述: 暂停通话，释放麦克风等设备资源
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)pauseChannel;

/**
 *  功能描述: 恢复通话
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)resumeChannel;

/**
 *  功能描述: 设置当前录音的时间戳，当通过录游戏脚本进行直播时，要保证观众端音画同步，在主播端需要进行时间对齐。 这个接口设置的就是当前游戏画面录制已经进行到哪个时间点了。
 */
- (void) setRecordingTimeMs:(unsigned int)timeMs;

/**
 *  功能描述: 设置当前播放的时间戳，当通过录游戏脚本进行直播时，要保证观众端音画同步，游戏画面的播放需要和声音播放进行时间对齐。 这个接口设置的就是当前游戏画面播放已经进行到哪个时间点了。
 */
- (void) setPlayingTimeMs:(unsigned int)timeMs;


/**
 *  功能描述: 设置PCM数据回调对象
 *  @param  flag:有3中pcm回调，分别为远端pcm,本地录音pcm及远端和录音的混合pcm。flag用于标记打开哪几种回调，形如PcmCallbackFlag_Remote| PcmCallbackFlag_Record|PcmCallbackFlag_Mix。
 *  @param  bOutputToSpeaker 是否扬声器静音:true 不静音;false 静音
 *  @param  nOutputSampleRate pcm callback重采样的采样率
 *  @param  nOutputChannel pcm callback重采样的通道数
 */
- (void) setPcmCallbackEnable:(int)flag outputToSpeaker:(bool)bOutputToSpeaker nOutputSampleRate:(int)nOutputSampleRate nOutputChannel:(int)nOutputChannel;

/**
 *  功能描述: 设置是否回调视频解码前H264数据，需要在加入房间之前设置
 *  @param  enable 是否回调
 *  @param  needDecodeandRender 是否需要解码并渲染:true 需要;false 不需要
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setVideoPreDecodeCallbackEnable:(bool)enable needDecodeandRender:(bool)decodeandRender;

/**
 *  功能描述:Rest API , 向服务器请求额外数据
 *  @param requestID 回传id,回调的时候传回，标识消息
 *  @param strCommand 请求的命令字符串
 *  @param strQueryBody 请求需要的数据,json格式，内容参考restAPI
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)requestRestApi:(NSString*) strCommand strQueryBody:(NSString*) strQueryBody requestID:(int*)requestID;

/**
 *  功能描述:查询频道的用户列表
 *  @param channelID 要查询的频道ID
 *  @param maxCount 想要获取的最大数量，-1表示获取全部
 *  @param notifyMemChange  其他用户进出房间时，是否要收到通知
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) getChannelUserList:(NSString*) channelID maxCount:(int)maxCount notifyMemChange:(bool)notifyMemChange ;


//---------------------抢麦接口---------------------//
/**
* 功能描述:    抢麦相关设置（抢麦活动发起前调用此接口进行设置）
* @param channelID 抢麦活动的频道id
* @param mode 抢麦模式（1:先到先得模式；2:按权重分配模式）
* @param maxAllowCount 允许能抢到麦的最大人数
* @param maxTalkTime 允许抢到麦后使用麦的最大时间（秒）
* @param voteTime 抢麦仲裁时间（秒），过了X秒后服务器将进行仲裁谁最终获得麦（仅在按权重分配模式下有效）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) setGrabMicOption:(NSString*) channelID mode:(int)mode maxAllowCount:(int)maxAllowCount maxTalkTime:(int)maxTalkTime voteTime:(unsigned int)voteTime;

/**
* 功能描述:    发起抢麦活动
* @param channelID 抢麦活动的频道id
* @param pContent 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) startGrabMicAction:(NSString*) channelID strContent:(NSString*) pContent;

/**
* 功能描述:    停止抢麦活动
* @param channelID 抢麦活动的频道id
* @param pContent 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) stopGrabMicAction:(NSString*) channelID strContent:(NSString*) pContent;

/**
* 功能描述:    发起抢麦请求
* @param channelID 抢麦的频道id
* @param score 积分（权重分配模式下有效，游戏根据自己实际情况设置）
* @param isAutoOpenMic 抢麦成功后是否自动开启麦克风权限
* @param pContent 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) requestGrabMic:(NSString*) channelID score:(int)score isAutoOpenMic:(bool)isAutoOpenMic strContent:(NSString*) pContent;

/**
* 功能描述:    释放抢到的麦
* @param channelID 抢麦活动的频道id
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) releaseGrabMic:(NSString*) channelID;


//---------------------连麦接口---------------------//
/**
* 功能描述:    连麦相关设置（角色是频道的管理者或者主播时调用此接口进行频道内的连麦设置）
* @param channelID 连麦的频道id
* @param waitTimeout 等待对方响应超时时间（秒）
* @param maxTalkTime 最大通话时间（秒）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) setInviteMicOption:(NSString*) channelID waitTimeout:(int)waitTimeout maxTalkTime:(int)maxTalkTime;

/**
* 功能描述:    发起与某人的连麦请求（主动呼叫）
* @param pUserID 被叫方的用户id
* @param pContent 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) requestInviteMic:(NSString*) channelID strUserID:(NSString*)pUserID strContent:(NSString*) pContent;

/**
* 功能描述:    对连麦请求做出回应（被动应答）
* @param pUserID 主叫方的用户id
* @param isAccept 是否同意连麦
* @param pContent 游戏传入的上下文内容，通知回调会传回此内容（目前只支持纯文本格式）
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) responseInviteMic:(NSString*) pUserID isAccept:(bool)isAccept strContent:(NSString*) pContent;

/**
* 功能描述:    停止连麦
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t) stopInviteMic;

/**
 * 功能描述:   启动摄像头采集
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)startCapture;

/**
 * 功能描述:   停止摄像头采集
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)stopCapture;

/**
* 功能描述:   检查桌面/窗口共享权限
* @return   YOUME_SUCCESS - 成功
*          其他 - 具体错误码
*/
- (YouMeErrorCode_t)checkSharePermission;

/**
 * 功能描述:   启动桌面/窗口共享
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)startShare:(int) mode windowid:(int)windowid;


//共享窗口的边框，需要建一个窗口，需要传入一个NSWIndowController;
-(void) setShareContext:(void*) context;

/**
 * 功能描述:   停止桌面/窗口共享
 */
- (void)stopShare;

/**
 * 开始共享屏幕采集并录像
 * @param filePath 录像文件路径，目前只支持mp4格式
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)startSaveScreen:(NSString*) filePath;

//- (YouMeErrorCode_t)setTranscriberEnabled:(bool)enabled;

/**
 * 停止录像
 */
- (void)stopSaveScreen;

/**
 *  功能描述: 设置预览帧率
 *  @param  fps 帧率（3-60），默认15帧
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoPreviewFps:(int)fps;

/**
 *  功能描述: 设置帧率
 *  @param  fps 帧率（3-60），默认15帧
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoFps:(int)fps;

/**
 *  功能描述: 设置小流帧率
 *  @param  fps 帧率（3-30），默认15帧
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoFpsForSecond:(int)fps;

/**
 *  功能描述: 设置共享视频帧率
 *  @param  fps 帧率（3-30），默认15帧
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoFpsForShare:(int)fps;

/**
 *  功能描述: 设置本地视频渲染回调的分辨率
 *  @param width 宽像素单位
 *  @param height 高像素单位
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoLocalResolutionWidth:(int)width height:(int)height;

#if TARGET_OS_IPHONE

/**
 * 功能描述:   是否启动前置摄像头
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setCaptureFrontCameraEnable:(bool)enable;

/**
 * 功能描述: 切换前置/后置摄像头
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)switchCamera;

/**
 * 功能描述: 权限检测结束后重置摄像头
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)resetCamera;

#else

/**
 *  功能描述: 获取windwos/macos平台，摄像头个数
 *  @return int - 数目，摄像头id为0-count
 */
- (int) getCameraCount;

/**
 *  功能描述: 获取windows/macos平台cameraId 对应名称
 *  @param  cameraId:摄像头id
 *  @return string - 成功:非空name 失败:空字符串
 */
- (NSString*) getCameraName:(int)cameraId;

/**
 *  功能描述: 设置windows/macos平台打开摄像头id
 *  @param  cameraId:摄像头id
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setOpenCameraId:(int)cameraId;

/**
 *  功能描述: 获取mac平台，audio采集设备数量
 *  @return int - 数目，
 */
- (int) getRecordDeviceCount;

/**
 *  功能描述: 获取macos平台 record设备 对应信息
 *  @param  index:列表中的位置
 *  @param  deviceName:设备名称
 *  @param  deviceUid:设备唯一ID，用于设置设备
 *  @return string - 成功:非空name 失败:空字符串
 */
- (boolean_t) getRecordDeviceInfo:(int)index deviceName:(NSString**)deviceName deviceUId:(NSString**)deviceUId;

/**
 *  功能描述: 设置windows/macos平台打开摄像头id
 *  @param  cameraId:摄像头id
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setRecordDevice:(NSString*) deviceUId;

#endif

/**
 * 功能描述:   向房间广播消息
 * @param channelID 广播房间
 * @param strContent 广播内容-文本串
 * @param requestID 返回消息标识，回调的时候会回传该值
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) sendMessage:(NSString*) channelID  strContent:(NSString*) strContent requestID:(int*) requestID;

/**
 * 功能描述:   向房间广播消息
 * @param channelID 广播房间
 * @param strContent 广播内容-文本串
 * @param toUserID 接收端用户ID
 * @param requestID 返回消息标识，回调的时候会回传该值
 * @return   YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) sendMessageToUser:(NSString*) channelID  strContent:(NSString*) strContent toUserID:(NSString*) toUserID requestID:(int*) requestID;

/**
 *  功能描述: 把某人踢出房间
 *  @param  userID 被踢的用户ID
 *  @param  channelID 从哪个房间踢出
 *  @param  lastTime 踢出后，多长时间内不允许再次进入
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) kickOtherFromChannel:(NSString*) userID  channelID:(NSString*)channelID   lastTime:(int) lastTime;

/**
 *  功能描述: 设置日志等级
 *  @param consoleLevel 控制台日志等级
 *  @param fileLevel 文件日志等级
 */
- (void) setLogLevelforConsole:(YOUME_LOG_LEVEL_t) consoleLevel forFile:(YOUME_LOG_LEVEL_t)fileLevel;

/**
 *  功能描述: 设置外部输入模式的语音采样率
 *  @param inputSampleRate 输入语音采样率
 *  @param mixedCallbackSampleRate mix后输出语音采样率
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setExternalInputSampleRate:(YOUME_SAMPLE_RATE_t)inputSampleRate mixedCallbackSampleRate:(YOUME_SAMPLE_RATE_t)mixedCallbackSampleRate;

/**
 *  功能描述: 设置视频大小流接收模式
 *  @param mode 模式 0:自动调整，1:手动调整
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoNetAdjustmode:(int)mode;

/**
 *  功能描述: 设置视频网络传输过程的分辨率, 第一路高分辨率
 *  @param width 宽
 *  @param height 高
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoNetResolutionWidth:(int)width height:(int)height;

/**
 *  功能描述: 设置视频网络传输过程的分辨率， 第二路第分辨率，默认不发送
 *  @param width 宽
 *  @param height 高
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoNetResolutionWidthForSecond:(int)width height:(int)height;

/**
 *  功能描述: 设置视频网络传输过程的分辨率， 共享视频分辨率，默认不发送
 *  @param width 宽
 *  @param height 高
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t)setVideoNetResolutionWidthForShare:(int)width height:(int)height;

/**
 *  功能描述: 设置音视频统计数据时间间隔
 *  @param interval 时间间隔
 */
- (void) setAVStatisticInterval:(int) interval ;

/**
 *  功能描述: 设置Audio的传输质量
 *  @param quality 0:low 1:high
 *
 */
- (void) setAudioQuality:(YOUME_AUDIO_QUALITY_t)quality;

/**
 *  功能描述: 设置视频数据上行的码率的上下限。
 *  @param maxBitrate 最大码率，单位kbps.  0：使用默认值
 *  @param minBitrate 最小码率，单位kbps.  0：使用默认值
 *
 *  @warning:需要在进房间之前设置
 */
- (void) setVideoCodeBitrate:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate;

/**
 *  功能描述: 设置视频数据上行的码率的上下限,第二路(默认不传)
 *  @param maxBitrate 最大码率，单位kbps.  0：使用默认值
 *  @param minBitrate 最小码率，单位kbps.  0：使用默认值
 *
 *  @warning:需要在进房间之前设置
 */
- (void) setVideoCodeBitrateForSecond:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate;

/**
 *  功能描述: 设置视频数据上行的码率的上下限, 共享视频流
 *  @param maxBitrate 最大码率，单位kbps.  0：使用默认值
 *  @param minBitrate 最小码率，单位kbps.  0：使用默认值
 *
 *  @warning:需要在进房间之前设置
 */
- (void) setVideoCodeBitrateForShare:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate;

/**
 *  功能描述: 设置视频编码是否采用VBR动态码率方式
 *
 *  @return None
 *
 *  @warning:需要在进房间之前设置
 */
- (YouMeErrorCode_t) setVBR:( bool) useVBR;

/**
 *  功能描述: 设置小流视频编码是否采用VBR动态码率方式
 *
 *  @return None
 *
 *  @warning:需要在进房间之前设置
 */
- (YouMeErrorCode_t) setVBRForSecond:( bool) useVBR;

/**
 *  功能描述: 设置共享π流视频编码是否采用VBR动态码率方式
 *
 *  @return None
 *
 *  @warning:需要在进房间之前设置
 */
- (YouMeErrorCode_t) setVBRForShare:( bool) useVBR;

/**
 *  功能描述: 获取视频数据上行的当前码率。
 *
 *  @return 视频数据上行的当前码率
 */
- (unsigned int) getCurrentVideoCodeBitrate;

/**
 *  功能描述: 设置视频数据是否同意开启硬编硬解
 *  @param bEnable true:开启，false:不开启
 *
 *  @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
 *  @warning:需要在进房间之前设置
 */
- (void) setVideoHardwareCodeEnable:(bool) bEnable;

/**
 *  功能描述: 获取视频数据是否同意开启硬编硬解
 *  @return true:开启，false:不开启， 默认为true;
 *
 *  @note: 实际是否开启硬解，还跟服务器配置及硬件是否支持有关，要全部支持开启才会使用硬解。并且如果硬编硬解失败，也会切换回软解。
 */
- (bool) getVideoHardwareCodeEnable;

/**
 *  功能描述: 获取是否使用GL进行视频前处理
 *  @return true:开启，false:不开启， 默认为true;
 *
 *  @note: 无
 */
- (bool) getUseGL;

/**
 *  功能描述: 设置视频无帧渲染的等待超时时间，超过这个时间会给上层回调YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN
 *  @param timeout 单位毫秒
 */
- (void) setVideoNoFrameTimeout:(int) timeout;

/**
 *  功能描述:判断是否初始化完成
 *
 *  @return true——完成，false——还未完成
 */
- (bool) isInited;

/**
 *  功能描述:切换身份(仅支持单频道模式，进入房间以后设置)
 *
 *  @param eUserRole 用户身份
 *
 *  @return 错误码，详见YouMeConstDefine.h定义
 */
- (YouMeErrorCode_t) setUserRole:(YouMeUserRole_t) eUserRole;

/**
 *  功能描述:获取身份(仅支持单频道模式)
 *
 *  @return 身份定义，详见YouMeConstDefine.h定义
 */
- (YouMeUserRole_t) getUserRole;


/**
 *  功能描述:查询是否在某个语音频道内
 *
 *  @param strChannelID 要查询的频道ID
 *
 *  @return true——在频道内，false——没有在频道内
 */
- (bool) isInChannel:(NSString*) strChannelID;

/**
 *  功能描述:查询是否在语音频道内
 *
 *
 *  @return true——在频道内，false——没有在频道内
 */
- (bool) isInChannel;

/**
 *  功能描述:背景音乐是否在播放
 *
 *  @return true——正在播放，false——没有播放
 */
- (bool) isBackgroundMusicPlaying;


/**
 *  功能描述: 查询多个用户视频信息（支持分辨率）
 *  @param userArray 用户ID列表
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) queryUsersVideoInfo:(NSMutableArray*)userArray;


/**
 *  功能描述: 设置多个用户视频信息（支持分辨率）
*   @param userArray 用户ID列表
 *  @param resolutionArray 用户对应分辨率列表
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) setUsersVideoInfo:(NSMutableArray*)userArray resolutionArray:(NSMutableArray*)resolutionArray;

/**
 *  功能描述: 美颜开关，默认是关闭美颜
 *  @param open true表示开启美颜，false表示关闭美颜
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) openBeautify:(bool) open ;

/**
 *  功能描述: 美颜强度参数设置
 *  @param param 美颜参数，0.0 - 1.0 ，默认为0，几乎没有美颜效果，0.5左右效果明显
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) beautifyChanged:(float) param ;

//#if TARGET_OS_IPHONE
///**
// *  功能描述: 瘦脸开关
// *  @param stretch true 开启瘦脸，false关闭，默认 false
// *  @return YOUME_SUCCESS - 成功
// *          其他 - 具体错误码
// */
//- (YouMeErrorCode_t) stretchFace:(bool) stretch ;
//#endif

/**
 *  功能描述: 屏蔽视频流
 *  @param strUserId 用户ID
 *  @param mask true 屏蔽, false 恢复，不再屏蔽
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) maskVideoByUserId:(NSString*) strUserId  mask:(bool) mask;


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
 * @return 成功/失败
 */
- (bool)inputVideoFrame:(void *)data Len:(int)len Width:(int)width Height:(int)height Fmt:(int)fmt Rotation:(int)rotation Mirror:(int)mirror Timestamp:(uint64_t)timestamp;

/**
 *  功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面。(待废弃)
 *  @param data 指向PCM数据的缓冲区
 *  @param len  音频数据的大小
 *  @param timestamp 时间戳
 *  @return 成功/失败
 */
- (bool)inputAudioFrame:(void *)data Len:(int)len Timestamp:(uint64_t)timestamp;

/**
 *  功能描述: 将提供的音频数据混合到麦克风或者扬声器的音轨里面
 *  @param data 指向PCM数据的缓冲区
 *  @param len  音频数据的大小
 *  @param timestamp 时间戳
 *  @param channelnum  声道数，1:单声道，2:双声道，其它非法
 *  @param binterleaved 音频数据打包格式（仅对双声道有效）
 *  @return 成功/失败
 */
- (bool)inputAudioFrameEx:(void *)data Len:(int)len Timestamp:(uint64_t)timestamp ChannelNum:(int)channelnum bInterleaved:(bool)binterleaved;

/**
 *  功能描述: 将多路音频数据流混合到麦克风或者扬声器的音轨里面。
 *  @param streamId  音频数据的流id
 *  @param data 指向PCM数据的缓冲区
 *  @param len  音频数据的大小
 *  @param frameInfo 音频数据的格式信息
 *  @param timestamp 时间戳
 *  @return 成功/失败
 */
- (bool)inputAudioFrameForMixStreamId:(int)streamId data:(void*)data length:(int)len frameInfo:(YMAudioFrameInfo_t)frameInfo timestamp:(uint64_t)timestamp;

/**
 * 功能描述: 视频数据输入(房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_START事件)
 * @param PixelBufferRef 视频帧数据
 * @param width 视频图像宽
 * @param height 视频图像高
 * @param fmt 视频格式
 * @param rotation 视频角度
 * @param mirror 镜像
 * @param timestamp 时间戳
 * @return 成功/失败
 */
- (bool)inputPixelBuffer:(CVPixelBufferRef)PixelBufferRef Width:(int)width Height:(int)height Fmt:(int)fmt Rotation:(int)rotation Mirror:(int)mirror Timestamp:(uint64_t)timestamp;

#if TARGET_OS_IPHONE
/**
 * 功能描述: 根据系统传送的音视频类型，输入音视频数据
 * @param sampleBufferType 系统传递上来的流类型，是video，audio，还是mic
 * @param sampleBuffer 系统传上来的数据流
 * @return 成功/失败
 */
- (bool)inputPixelBufferShare:(RPSampleBufferType)sampleBufferType withBuffer:(CMSampleBufferRef)sampleBuffer;
#endif

/**
 * 功能描述: 停止视频数据输入(在inputVideoFrame之后调用，房间内其它用户会收到YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP事件)
 */
- (void)stopInputVideoFrame;

/**
 * 功能描述: 停止视频数据输入(在inputVideoFrameForShare之后调用，房间内其它用户会收到YOUME_EVENT_OTHERS_SHARE_INPUT_STOP事件)
 */
- (void)stopInputVideoFrameForShare;

/*
 * 设置合流后的总体尺寸
 * @param width
 * @param height
 */
- (void)setMixVideoWidth:(int)width Height:(int)height;

/**
 * 设置具体的user的视频数据在合流画面中展现的位置和尺寸。
 * @param userId 指定userId
 * @param x       x
 * @param y       y
 * @param z       z (Z 值小的在前面)
 * @param width   缩放后的宽度(居中缩放裁剪)
 * @param height  缩放后的高度(居中缩放裁剪)
 */
- (void)addMixOverlayVideoUserId:(NSString*)userId PosX:(int)x PosY:(int)y PosZ:(int)z Width:(int)width Height:(int)height;

/**
 * remove the specified user's video from the mixing
 * @param userId 从画布中移除指定userId的
 */
- (void)removeMixOverlayVideoUserId:(NSString*)userId;

- (void)removeAllOverlayVideo;

- (bool) releaseMicSync;
- (bool) resumeMicSync;

/**
 *  功能描述: 打开/关闭 外部扩展滤镜回调
 *  @param  enabled true表示打开回调，false表示关闭回调
 *  @return void
 */
-(bool)setExternalFilterEnabled:(bool)enabled;

-(YouMeErrorCode_t) setAudioMixerTrackSamplerate:(int)sampleRate;
-(YouMeErrorCode_t) setAudioMixerTrackVolume:(int)volume;
-(YouMeErrorCode_t) setAudioMixerInputVolume:(int)volume;
-(YouMeErrorCode_t) pushAudioMixerTrackwithBuffer:(void*)pBuf nSizeInByte:(int)nSizeInByte nChannelNUm:(int)nChannelNUm nSampleRate:(int) nSampleRate nBytesPerSample:(int)nBytesPerSample bFloat:(bool) bFloat timestamp:(uint64_t) timestamp;
-(YouMeErrorCode_t) inputAudioToMixwithIndexId:(NSString*)indexId Buffer:(void*)pBuf nSizeInByte:(int)nSizeInByte nChannelNUm:(int)nChannelNUm nSampleRate:(int) nSampleRate nBytesPerSample:(int)nBytesPerSample bFloat:(bool) bFloat timestamp:(uint64_t) timestamp;

/**
 *  功能描述: 输入用户自定义数据，以广播形式发送到房间内其它成员
 *  @param  data 自定义数据
 *  @param  len 数据长度，不能大于1024
 *  @param  timestamp 时间戳
 *  @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
- (YouMeErrorCode_t) inputCustomData:(const void *)data Len:(int)len Timestamp:(uint64_t)timestamp;




- (int)enableMainQueueDispatch:(BOOL)enabled;

- (int)enableLocalVideoRender: (bool)enabled;
- (int)enableLocalVideoSend: (bool)enabled;

- (int)muteAllRemoteVideoStreams:(BOOL)mute;
- (int)setDefaultMuteAllRemoteVideoStreams:(BOOL)mute;

- (int)muteRemoteVideoStream:(NSString*)uid
                        mute:(BOOL)mute;

#if TARGET_OS_IPHONE
#pragma mark Video camera control
- (BOOL)isCameraZoomSupported;
- (CGFloat)setCameraZoomFactor:(CGFloat)zoomFactor;

- (BOOL)isCameraFocusPositionInPreviewSupported;
- (BOOL)setCameraFocusPositionInPreview:(CGPoint)position;

- (BOOL)isCameraTorchSupported;
- (BOOL)setCameraTorchOn:(BOOL)isOn;

- (BOOL)isCameraAutoFocusFaceModeSupported;
- (BOOL)setCameraAutoFocusFaceModeEnabled:(BOOL)enable;
#endif

- (int)setLocalVideoMirrorMode:(YouMeVideoMirrorMode_t) mode;

- (int)setLocalVideoPreviewMirror:(bool) enable;

- (void)setVideoFrameRawCbEnable:(bool) bEnable;

/**
 * 开启服务器推流，推送自己的数据
 * @param pushUrl 推流地址，可以用restApi获取
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  startPush:(NSString*) pushUrl ;
/**
 * 关闭服务器推流，对应startPush
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  stopPush;

/**
 * 开启服务器混流推流，将多个人的画面混到一起，对混流画面就行推流。每个房间只允许一个混流推流。
 * 开启服务器混流推流后，可以通过addPushMixUser/removePushMixUser改变推流的画面。
 * @param pushUrl 推流地址，可以用restApi获取
 * @param width 推流的画面的宽
 * @param height 推流的画面的高
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  setPushMix:(NSString*) pushUrl  width:(int) width  height:(int) height ;
/**
 * 关闭服务器混流推流，对应setPushMix
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  clearPushMix;

/**
 * 在服务器混流推流中，加入一个user的数据
 * @param userId 需要加入的用户ID
 * @param x 用户画面在混流画面中的位置，x
 * @param y 用户画面在混流画面中的位置，y
 * @param z 用户画面在混流画面中的位置，z,代表前后关系
 * @param width 用户画面在混流画面中的宽
 * @param height 用户画面在混流画面中的高
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  addPushMixUser:(NSString*) userId x:(int)x y:(int)y z:(int)z width:(int)width height:(int)height ;

/**
 * 删除混流推流中的一个用户
 * @param userId 需要删除的用户ID
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t)  removePushMixUser: (NSString*) userId ;


/**
 * 通知sdk发生了横竖屏切换，仅在需要SDK交换宽高设置的时候通知，SDK自动获取当前是横屏还是竖屏，对摄像头旋转自动纠正，并在横屏时交换原先设置的宽高设置
 * @return YOUME_SUCCESS - 成功
 *          其他 - 具体错误码
 */
-(YouMeErrorCode_t) screenRotationChange ;


/*
 * 功能：国际语言文本翻译
 * @param requestID：请求ID
 * @param text：内容
 * @param destLangCode：目标语言编码
 * @param srcLangCode：源语言编码
 * @return 错误码
 */
-(YouMeErrorCode_t) translateText:(unsigned int*) requestID  text:(NSString*)text destLangCode:(YouMeLanguageCode_t)destLangCode srcLangCode:(YouMeLanguageCode_t)srcLangCode;


@end

