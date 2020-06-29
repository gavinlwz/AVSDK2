package com.youme.voiceengine;
public class YouMeConst{
	public class ChannelState
	{
		public static final int CHANNEL_STATE_JOINIING     = 0;    ///< 正在加入频道
		public static final int CHANNEL_STATE_JOINED       = 1;    ///< 已经加入频道
		public static final int CHANNEL_STATE_LEAVING_ONE  = 2;    ///< 正在离开单个频道
		public static final int CHANNEL_STATE_LEAVING_ALL  = 3;    ///< 正在离开单个频道
		public static final int CHANNEL_STATE_LEAVED       = 4;    ///< 已经离开频道
	}

	public class YouMeUserRole {
	    public static final int YOUME_USER_NONE              = 0;    ///< 非法用户，调用API时不能传此参数
	    public static final int YOUME_USER_TALKER_FREE       = 1;    ///< 自由讲话者，适用于小组通话（建议小组成员数最多10个），每个人都可以随时讲话
	    public static final int YOUME_USER_TALKER_ON_DEMAND  = 2;    ///< 需要通过抢麦等请求麦克风权限之后才可以讲话，适用于较大的组或工会等（比如几十个人），同一个时刻只能有一个或几个人能讲话
	    public static final int YOUME_USER_LISTENER          = 3;    ///< 听众，主播/指挥/嘉宾的听众，同一个时刻只能在一个语音频道里面，只听不讲
	    public static final int YOUME_USER_COMMANDER         = 4;    ///< 指挥，国家/帮派等的指挥官，同一个时刻只能在一个语音频道里面，可以随时讲话，可以播放背景音乐，戴耳机情况下可以监听自己语音
	    public static final int YOUME_USER_HOST              = 5;    ///< 主播，广播型语音频道的主持人，同一个时刻只能在一个语音频道里面，可以随时讲话，可以播放背景音乐，戴耳机情况下可以监听自己语音
	    public static final int YOUME_USER_GUSET             = 6;    ///< 嘉宾，主播或指挥邀请的连麦嘉宾，同一个时刻只能在一个语音频道里面， 可以随时讲话
	}

	public class YouMeEvent
	{
		public static final int YOUME_EVENT_INIT_OK                      = 0;   ///< SDK初始化成功
		public static final int YOUME_EVENT_INIT_FAILED                  = 1;   ///< SDK初始化失败
		public static final int YOUME_EVENT_JOIN_OK                      = 2;   ///< 进入语音频道成功
		public static final int YOUME_EVENT_JOIN_FAILED                  = 3;   ///< 进入语音频道失败
		public static final int YOUME_EVENT_LEAVED_ONE                   = 4;   ///< 退出单个语音频道完成
		public static final int YOUME_EVENT_LEAVED_ALL                   = 5;   ///< 退出所有语音频道完成
		public static final int YOUME_EVENT_PAUSED                       = 6;   ///< 暂停语音频道完成
		public static final int YOUME_EVENT_RESUMED                      = 7;   ///< 恢复语音频道完成
		public static final int YOUME_EVENT_SPEAK_SUCCESS                = 8;   ///< 切换对指定频道讲话成功（适用于多频道模式）
		public static final int YOUME_EVENT_SPEAK_FAILED                 = 9;   ///< 切换对指定频道讲话失败（适用于多频道模式）
		public static final int YOUME_EVENT_RECONNECTING                 = 10;  ///< 断网了，正在重连
		public static final int YOUME_EVENT_RECONNECTED                  = 11;  ///< 断网重连成功
		public static final int YOUME_EVENT_REC_PERMISSION_STATUS        = 12;  ///< 通知录音权限状态，成功获取权限时错误码为YOUME_SUCCESS，获取失败为YOUME_ERROR_REC_NO_PERMISSION（此时不管麦克风mute状态如何，都没有声音输出）
		public static final int YOUME_EVENT_BGM_STOPPED                  = 13;  ///< 通知背景音乐播放结束
		public static final int YOUME_EVENT_BGM_FAILED                   = 14;  ///< 通知背景音乐播放失败
		//public static final int YOUME_EVENT_MEMBER_CHANGE              = 15;  ///< 频道成员变化

		public static final int YOUME_EVENT_OTHERS_MIC_ON                = 16;  ///< 其他用户麦克风打开
		public static final int YOUME_EVENT_OTHERS_MIC_OFF               = 17;  ///< 其他用户麦克风关闭
		public static final int YOUME_EVENT_OTHERS_SPEAKER_ON            = 18;  ///< 其他用户扬声器打开
		public static final int YOUME_EVENT_OTHERS_SPEAKER_OFF           = 19;  ///< 其他用户扬声器关闭
		public static final int YOUME_EVENT_OTHERS_VOICE_ON              = 20;  ///< 其他用户进入讲话状态
		public static final int YOUME_EVENT_OTHERS_VOICE_OFF             = 21;  ///< 其他用户进入静默状态
		public static final int YOUME_EVENT_MY_MIC_LEVEL                 = 22;  ///< 麦克风的语音级别

		public static final int YOUME_EVENT_MIC_CTR_ON                   = 23;  ///< 麦克风被其他用户打开
		public static final int YOUME_EVENT_MIC_CTR_OFF                  = 24;  ///< 麦克风被其他用户关闭    
		public static final int YOUME_EVENT_SPEAKER_CTR_ON               = 25;  ///< 扬声器被其他用户打开
		public static final int YOUME_EVENT_SPEAKER_CTR_OFF              = 26;  ///< 扬声器被其他用户关闭

		public static final int YOUME_EVENT_LISTEN_OTHER_ON              = 27;  ///< 取消屏蔽某人语音
		public static final int YOUME_EVENT_LISTEN_OTHER_OFF             = 28;  ///< 屏蔽某人语音

		public static final int YOUME_EVENT_LOCAL_MIC_ON		         = 29;  ///< 自己的麦克风打开
		public static final int YOUME_EVENT_LOCAL_MIC_OFF		         = 30;  ///< 自己的麦克风关闭
		public static final int YOUME_EVENT_LOCAL_SPEAKER_ON	         = 31;  ///< 自己的扬声器打开
		public static final int YOUME_EVENT_LOCAL_SPEAKER_OFF	         = 32;  ///< 自己的扬声器关闭

		public static final int YOUME_EVENT_GRABMIC_START_OK             = 33;  ///< 发起抢麦活动成功
		public static final int YOUME_EVENT_GRABMIC_START_FAILED         = 34;  ///< 发起抢麦活动失败
		public static final int YOUME_EVENT_GRABMIC_STOP_OK              = 35;  ///< 停止抢麦活动成功
		public static final int YOUME_EVENT_GRABMIC_STOP_FAILED          = 36;  ///< 停止抢麦活动失败
		public static final int YOUME_EVENT_GRABMIC_REQUEST_OK	         = 37;  ///< 抢麦成功（可以说话）
		public static final int YOUME_EVENT_GRABMIC_REQUEST_FAILED       = 38;  ///< 抢麦失败
		public static final int YOUME_EVENT_GRABMIC_REQUEST_WAIT         = 39;  ///< 进入抢麦等待队列（仅权重模式下会回调此事件）
		public static final int YOUME_EVENT_GRABMIC_RELEASE_OK           = 40;  ///< 释放麦成功
		public static final int YOUME_EVENT_GRABMIC_RELEASE_FAILED       = 41;  ///< 释放麦失败
		public static final int YOUME_EVENT_GRABMIC_ENDMIC               = 42;  ///< 不再占用麦（到麦使用时间或者其他原因）

		public static final int YOUME_EVENT_GRABMIC_NOTIFY_START         = 43;  ///< [通知]抢麦活动开始
		public static final int YOUME_EVENT_GRABMIC_NOTIFY_STOP          = 44;  ///< [通知]抢麦活动结束
		public static final int YOUME_EVENT_GRABMIC_NOTIFY_HASMIC        = 45;  ///< [通知]有麦可以抢
		public static final int YOUME_EVENT_GRABMIC_NOTIFY_NOMIC         = 46;  ///< [通知]没有麦可以抢

		public static final int YOUME_EVENT_INVITEMIC_SETOPT_OK          = 47;  ///< 连麦设置成功
		public static final int YOUME_EVENT_INVITEMIC_SETOPT_FAILED      = 48;  ///< 连麦设置失败
		public static final int YOUME_EVENT_INVITEMIC_REQUEST_OK         = 49;  ///< 请求连麦成功（连上了，需等待对方回应）
		public static final int YOUME_EVENT_INVITEMIC_REQUEST_FAILED     = 50;  ///< 请求连麦失败
		public static final int YOUME_EVENT_INVITEMIC_RESPONSE_OK        = 51;  ///< 响应连麦成功（被叫方无论同意/拒绝都会收到此事件，错误码是YOUME_ERROR_INVITEMIC_REJECT表示拒绝）
		public static final int YOUME_EVENT_INVITEMIC_RESPONSE_FAILED    = 52;  ///< 响应连麦失败
		public static final int YOUME_EVENT_INVITEMIC_STOP_OK            = 53;  ///< 停止连麦成功
		public static final int YOUME_EVENT_INVITEMIC_STOP_FAILED        = 54;  ///< 停止连麦失败

		public static final int YOUME_EVENT_INVITEMIC_CAN_TALK           = 55;  ///< 双方可以通话了（响应方已经同意）
		public static final int YOUME_EVENT_INVITEMIC_CANNOT_TALK        = 56;  ///< 双方不可以再通话了（有一方结束了连麦或者连麦时间到）

		public static final int YOUME_EVENT_INVITEMIC_NOTIFY_CALL        = 57;  ///< [通知]有人请求与你连麦
		public static final int YOUME_EVENT_INVITEMIC_NOTIFY_ANSWER      = 58;  ///< [通知]对方对你的连麦请求作出了响应（同意/拒绝/超时，同意的话双方就可以通话了）
		public static final int YOUME_EVENT_INVITEMIC_NOTIFY_CANCEL      = 59;  ///< [通知]连麦过程中，对方结束了连麦或者连麦时间到

		public static final int YOUME_EVENT_SEND_MESSAGE_RESULT          = 60;  ///< sendMessage成功与否的通知，param为回传的requestID
		public static final int YOUME_EVENT_MESSAGE_NOTIFY               = 61;  ///< 收到Message, param为message内容

		public static final int YOUME_EVENT_KICK_RESULT                  = 64;  ///< 踢人的应答
		public static final int YOUME_EVENT_KICK_NOTIFY                  = 65;  ///< 被踢通知   ,param: （踢人者ID，被踢原因，被禁时间）

		public static final int YOUME_EVENT_FAREND_VOICE_LEVEL           = 66;  ///< 远端说话人音量大小

		public static final int YOUME_EVENT_OTHERS_BE_KICKED             = 67;  ///< 房间里其他人被踢出房间

		public static final int YOUME_EVENT_SWITCH_OUTPUT                = 75;  ///< 切换扬声器/听筒

		public static final int YOUME_EVENT_OTHERS_VIDEO_ON              = 200; ///< 收到其它用户的视频流
		//public static final int YOUME_EVENT_OTHERS_VIDEO_OFF           = 201; ///< 其他用户视频流断开
		//public static final int YOUME_EVENT_OTHERS_CAMERA_PAUSE        = 202; ///< 其他用户摄像头暂停
		//public static final int YOUME_EVENT_OTHERS_CAMERA_RESUME       = 203; ///< 其他用户摄像头恢复
		public static final int YOUME_EVENT_MASK_VIDEO_BY_OTHER_USER     = 204; ///< 视频被其他用户屏蔽
		public static final int YOUME_EVENT_RESUME_VIDEO_BY_OTHER_USER   = 205; ///< 视频被其他用户恢复
		public static final int YOUME_EVENT_MASK_VIDEO_FOR_USER          = 206; ///< 屏蔽了谁的视频
		public static final int YOUME_EVENT_RESUME_VIDEO_FOR_USER        = 207; ///< 恢复了谁的视频
		public static final int YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN       = 208; ///< 其它用户的视频流断开（包含网络中断的情况）
		public static final int YOUME_EVENT_OTHERS_VIDEO_INPUT_START     = 209; ///< 其他用户视频输入开始（内部采集下开启摄像头/外部输入下开始input）
		public static final int YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP      = 210; ///< 其他用户视频输入停止（内部采集下停止摄像头/外部输入下停止input）

		public static final int YOUME_EVENT_MEDIA_DATA_ROAD_PASS         = 211; ///音视频数据通路连通，定时检测，一开始收到数据会收到PASS事件，之后变化的时候会发送
		public static final int YOUME_EVENT_MEDIA_DATA_ROAD_BLOCK        = 212; ///音视频数据通路不通

		public static final int YOUME_EVENT_QUERY_USERS_VIDEO_INFO       = 213; ///查询用户视频信息返回
		public static final int YOUME_EVENT_SET_USERS_VIDEO_INFO         = 214; ///设置用户接收视频信息返回

		public static final int YOUME_EVENT_LOCAL_VIDEO_INPUT_START      = 215; ///< 本地视频输入开始（内部采集下开始摄像头/外部输入下开始input）
		public static final int YOUME_EVENT_LOCAL_VIDEO_INPUT_STOP       = 216; ///< 本地视频输入停止（内部采集下停止摄像头/外部输入下停止input）

		public static final int YOUME_EVENT_START_PUSH                   = 217; ///< 设置startPush的返回事件
		public static final int YOUME_EVENT_SET_PUSH_MIX                 = 218; ///< 设置setPushMix的返回事件
		public static final int YOUME_EVENT_ADD_PUSH_MIX_USER            = 219; ///< 设置addPushMixUser的返回事件，参数userID
		public static final int YOUME_EVENT_OTHER_SET_PUSH_MIX           = 220; ///< 在自己调用了setPushMix还没停止的情况下，房间内有别人调用setPushMix，自己被踢

		public static final int YOUME_EVENT_OTHERS_DATA_ERROR            = 300; /// 数据错误
		public static final int YOUME_EVENT_OTHERS_NETWORK_BAD           = 301; /// 网络不好
		public static final int YOUME_EVENT_OTHERS_BLACK_FULL            = 302; /// 黑屏
		public static final int YOUME_EVENT_OTHERS_GREEN_FULL            = 303; /// 绿屏
		public static final int YOUME_EVENT_OTHERS_BLACK_BORDER          = 304; /// 黑边
		public static final int YOUME_EVENT_OTHERS_GREEN_BORDER          = 305; /// 绿边
		public static final int YOUME_EVENT_OTHERS_BLURRED_SCREEN        = 306; /// 花屏
		public static final int YOUME_EVENT_OTHERS_ENCODER_ERROR         = 307; /// 编码错误
		public static final int YOUME_EVENT_OTHERS_DECODER_ERROR         = 308; /// 解码错误

		public static final int YOUME_EVENT_CAMERA_DEVICE_CONNECT        = 400; /// 摄像头设备插入，移动端无效
		public static final int YOUME_EVENT_CAMERA_DEVICE_DISCONNECT     = 401; /// 摄像头设备拔出，移动端无效

		public static final int YOUME_EVENT_VIDEO_ENCODE_PARAM_REPORT	 = 500;

		// p2p/server route
		public static final int YOUME_EVENT_RTP_ROUTE_P2P                = 600; /// P2P通路检测ok, 当前通路为P2P
		public static final int YOUME_EVENT_RTP_ROUTE_SEREVER            = 601; /// P2P通路检测失败, 当前通路为server转发
		public static final int YOUME_EVENT_RTP_ROUTE_CHANGE_TO_SERVER   = 602; /// 运行过程中P2P 检测失败，切换到server转发

		public static final int YOUME_EVENT_EOF                          = 1000;
	}
	
	/// 房间内的广播消息
	public class YouMeBroadcast
	{
		public static final int YOUME_BROADCAST_NONE = 0;
		public static final int YOUME_BROADCAST_GRABMIC_BROADCAST_GETMIC = 1;	    ///< 有人抢到了麦
		public static final int YOUME_BROADCAST_GRABMIC_BROADCAST_FREEMIC = 2;	    ///< 有人释放了麦
		public static final int YOUME_BROADCAST_INVITEMIC_BROADCAST_CONNECT = 3;	///< A和B正在连麦
		public static final int YOUME_BROADCAST_INVITEMIC_BROADCAST_DISCONNECT = 4; ///< A和B取消了连麦
	}
	
	public class YouMeErrorCode
	{
		public static final int YOUME_SUCCESS                            = 0;    ///< 成功

		// 参数和状态检查
		public static final int YOUME_ERROR_API_NOT_SUPPORTED            = -1;   ///< 正在使用的SDK不支持特定的API
		public static final int YOUME_ERROR_INVALID_PARAM                = -2;   ///< 传入参数错误
		public static final int YOUME_ERROR_ALREADY_INIT                 = -3;   ///< 已经初始化
		public static final int YOUME_ERROR_NOT_INIT                     = -4;   ///< 还没有初始化，在调用某函数之前要先调用初始化并且要根据返回值确保初始化成功
		public static final int YOUME_ERROR_CHANNEL_EXIST                = -5;   ///< 要加入的频道已经存在
		public static final int YOUME_ERROR_CHANNEL_NOT_EXIST            = -6;   ///< 要退出的频道不存在
		public static final int YOUME_ERROR_WRONG_STATE                  = -7;   ///< 状态错误
		public static final int YOUME_ERROR_NOT_ALLOWED_MOBILE_NETWROK   = -8;   ///< 不允许使用移动网络
		public static final int YOUME_ERROR_WRONG_CHANNEL_MODE           = -9;   ///< 在单频道模式下调用了多频道接口，或者反之
		public static final int YOUME_ERROR_TOO_MANY_CHANNELS            = -10;  ///< 同时加入了太多频道
		public static final int YOUME_ERROR_TOKEN_ERROR                  = -11;  ///< 传入的token认证错误
		public static final int YOUME_ERROR_NOT_IN_CHANNEL               = -12;  ///< 用户不在该频道
		public static final int YOUME_ERROR_BE_KICK                      = -13;  ///< 被踢了，还在禁止时间内，不允许进入房间
		public static final int YOUME_ERROR_DEVICE_NOT_VALID			 = -14;	 ///< 设置的设备不可用
		public static final int YOUME_ERROR_API_NOT_ALLOWED			     = -15;  ///< 没有特定功能的权限
		// 内部操作错误
		public static final int YOUME_ERROR_MEMORY_OUT                   = -100; ///< 内存错误
		public static final int YOUME_ERROR_START_FAILED                 = -101; ///< 启动引擎失败
		public static final int YOUME_ERROR_STOP_FAILED                  = -102; ///<  停止引擎失败
		public static final int YOUME_ERROR_ILLEGAL_SDK                  = -103; ///< 非法使用SDK
		public static final int YOUME_ERROR_SERVER_INVALID               = -104; ///< 语音服务不可用
		public static final int YOUME_ERROR_NETWORK_ERROR                = -105; ///< 网络错误
		public static final int YOUME_ERROR_SERVER_INTER_ERROR           = -106; ///< 服务器内部错误
		public static final int  YOUME_ERROR_QUERY_RESTAPI_FAIL          = -107; ///< 请求RestApi信息失败了
		public static final int YOUME_ERROR_USER_ABORT                   = -108; ///< 用户请求中断当前操作
		public static final int YOUME_ERROR_SEND_MESSAGE_FAIL            = -109; ///< 发送消息失败
	    	    
		// 麦克风错误
		public static final int YOUME_ERROR_REC_INIT_FAILED              = -201; ///< 录音模块初始化失败
		public static final int YOUME_ERROR_REC_NO_PERMISSION            = -202; ///< 没有录音权限
		public static final int YOUME_ERROR_REC_NO_DATA                  = -203; ///< 虽然初始化成功，但没有音频数据输出，比如oppo系列手机在录音权限被禁止的时候
		public static final int YOUME_ERROR_REC_OTHERS                   = -204; ///< 其他录音模块的错误
		public static final int YOUME_ERROR_REC_PERMISSION_UNDEFINED     = -205; ///< 录音权限未确定，iOS显示是否允许录音权限对话框时，返回的是这个错误码

		// 抢麦错误
		public static final int YOUME_ERROR_GRABMIC_FULL				 = -301; ///< 抢麦失败，人数满
		public static final int YOUME_ERROR_GRABMIC_HASEND				 = -302; ///< 抢麦失败，活动已经结束
		
		// 连麦错误
		public static final int YOUME_ERROR_INVITEMIC_NOUSER			 = -401; ///< 连麦失败，用户不存在
		public static final int YOUME_ERROR_INVITEMIC_OFFLINE			 = -402; ///< 连麦失败，用户已离线
		public static final int YOUME_ERROR_INVITEMIC_REJECT			 = -403; ///< 连麦失败，用户拒绝
		public static final int YOUME_ERROR_INVITEMIC_TIMEOUT			 = -404; ///< 连麦失败，两种情况：1.连麦时，对方超时无应答 2.通话中，双方通话时间到
		
		public static final int YOUME_ERROR_CAMERA_OPEN_FAILED			 = -501; ///< 打开摄像头失败

		
		public static final int YOUME_ERROR_UNKNOWN                      = -1000;///< 未知错误
	}

	public class YouMeKickReason{
		public static final  int  YOUME_KICK_ADMIN  = 1;	///< 管理员踢人
		public static final  int  YOUME_KICK_RELOGIN  = 2;	///< 多端登录被踢
	}

	public class YouMeAVStatisticType
	{
		public static final int YOUME_AVS_AUDIO_CODERATE = 1;               //音频传输码率，userid是自己:上行码率，userid其它人:下行码率，单位Bps
		public static final int YOUME_AVS_VIDEO_CODERATE = 2;               //视频传输码率，userid是自己:上行码率，userid其它人:下行码率，单位Bps
		public static final int YOUME_AVS_VIDEO_FRAMERATE = 3;              //视频帧率，userid是自己:上行帧率，userid其它人:下行帧率
		public static final int YOUME_AVS_AUDIO_PACKET_UP_LOSS_RATE = 4;    //音频上行丢包率,千分比
		public static final int YOUME_AVS_AUDIO_PACKET_DOWN_LOSS_RATE = 5;  //音频下行丢包率,千分比
		public static final int YOUME_AVS_VIDEO_PACKET_UP_LOSS_RATE = 6;    //视频上行丢包率,千分比
		public static final int YOUME_AVS_VIDEO_PACKET_DOWN_LOSS_RATE = 7;  //视频下行丢包率,千分比
		public static final int YOUME_AVS_AUDIO_DELAY_MS = 8; 				//音频延迟，单位毫秒
		public static final int YOUME_AVS_VIDEO_DELAY_MS = 9; 				//视频延迟，单位毫秒
		public static final int YOUME_AVS_VIDEO_BLOCK = 10;                 //视频卡顿,是否发生过卡顿
		public static final int YOUME_AVS_AUDIO_PACKET_UP_LOSS_HALF = 11;	//音频上行的服务器丢包率，千分比
		public static final int YOUME_AVS_VIDEO_PACKET_UP_LOSS_HALF = 12;	//视频上行的服务器丢包率，千分比
		public static final int YOUME_AVS_RECV_DATA_STAT = 13;				//下行带宽，单位Bps
	}

	public class YOUME_LOG_LEVEL
	{
		public static final  int  LOG_DISABLED = 0;
		public static final  int  LOG_FATAL = 1;
		public static final  int  LOG_ERROR = 10;
		public static final  int  LOG_WARNING = 20;
		public static final  int  LOG_INFO = 40;
		public static final  int  LOG_DEBUG = 50;
		public static final  int  LOG_VERBOSE = 60;
	}

	public class YOUME_SAMPLE_RATE
	{
		public static final  int  SAMPLE_RATE_8 = 8000;
		public static final  int  SAMPLE_RATE_16 = 16000;
		public static final  int  SAMPLE_RATE_24 = 24000;
		public static final  int  SAMPLE_RATE_32 = 32000;
		public static final  int  SAMPLE_RATE_44 = 44100;
		public static final  int  SAMPLE_RATE_48 = 48000;
	}
    
    public class YOUME_AUDIO_QUALITY
    {
        public static final int LOW_QUALITY = 0;
        public static final int HIGH_QUALITY = 1;
    }
	
	public class YOUME_RTC_SERVER_REGION {
		public static final int RTC_CN_SERVER      = 0;  // 中国
		public static final int RTC_HK_SERVER      = 1;  // 香港
		public static final int RTC_US_SERVER      = 2;  // 美国东部
		public static final int RTC_SG_SERVER      = 3;  // 新加坡
		public static final int RTC_KR_SERVER      = 4;  // 韩国
		public static final int RTC_AU_SERVER      = 5;  // 澳洲
		public static final int RTC_DE_SERVER      = 6;  // 德国
		public static final int RTC_BR_SERVER      = 7;  // 巴西
		public static final int RTC_IN_SERVER      = 8;  // 印度
		public static final int RTC_JP_SERVER      = 9;  // 日本
		public static final int RTC_IE_SERVER      = 10; // 爱尔兰
		public static final int RTC_USW_SERVER     = 11; // 美国西部
		public static final int RTC_USM_SERVER     = 12; // 美国中部
		public static final int RTC_CA_SERVER      = 13; // 加拿大
		public static final int RTC_LON_SERVER     = 14; //伦敦
		public static final int RTC_FRA_SERVER     = 15; //法兰克福
		public static final int RTC_DXB_SERVER     = 16; //迪拜
		
	    public static final int RTC_EXT_SERVER     = 10000; // 使用扩展服务器
    	public static final int RTC_DEFAULT_SERVER = 10001; // 缺省服务器
	}
	
	public class YOUME_VIDEO_FMT{
		public static final int VIDEO_FMT_RGBA32 = 0;
		public static final int VIDEO_FMT_BGRA32 = 1;
		public static final int VIDEO_FMT_YUV420P = 2;
		public static final int VIDEO_FMT_NV21 = 3;
		public static final int VIDEO_FMT_YV12 = 4;
		public static final int VIDEO_FMT_CVPIXELBUFFER  = 5;
		public static final int VIDEO_FMT_TEXTURE = 6;
		public static final int VIDEO_FMT_TEXTURE_OES = 7;
		public static final int VIDEO_FMT_RGB24 = 8;
		public static final int VIDEO_FMT_NV12 = 9;
		public static final int VIDEO_FMT_H264 = 10;
		public static final int VIDEO_FMT_ABGR32 = 11;
	}
	
	public class YouMeVideoMirrorMode {
		public static final int YOUME_VIDEO_MIRROR_MODE_AUTO = 0;                 //内部采集自适应镜像，外部采集为关闭镜像
		public static final int YOUME_VIDEO_MIRROR_MODE_ENABLED = 1;              //近端和远端都镜像
		public static final int YOUME_VIDEO_MIRROR_MODE_DISABLED = 2;             //关闭镜像
		public static final int YOUME_VIDEO_MIRROR_MODE_NEAR = 3;                 //近端镜像
		public static final int YOUME_VIDEO_MIRROR_MODE_FAR = 4;                  //远端镜像
	}

	public class  YouMePcmCallBackFlag{
		public static final int PcmCallbackFlag_Remote = 0x1;       //远端pcm回调
		public static final int PcmCallbackFlag_Record = 0x2;       //本地录音回调
		public static final int PcmCallbackFlag_Mix = 0x4;          //本地录音和远端pcm进行mix之后的回调
	}

	public class YouMeLanguageCode
	{
		public static final int	LANG_AUTO	=	0	;    //
		public static final int	LANG_AF	=	1	;    //	南非荷兰语
		public static final int	LANG_AM	=	2	;    //	阿姆哈拉语
		public static final int	LANG_AR	=	3	;    //	阿拉伯语
		public static final int	LANG_AR_AE	=	4	;    //	阿拉伯语+阿拉伯联合酋长国
		public static final int	LANG_AR_BH	=	5	;    //	阿拉伯语+巴林
		public static final int	LANG_AR_DZ	=	6	;    //	阿拉伯语+阿尔及利亚
		public static final int	LANG_AR_KW	=	7	;    //	阿拉伯语+科威特
		public static final int	LANG_AR_LB	=	8	;    //	阿拉伯语+黎巴嫩
		public static final int	LANG_AR_OM	=	9	;    //	阿拉伯语+阿曼
		public static final int	LANG_AR_SA	=	10	;    //	阿拉伯语+沙特阿拉伯
		public static final int	LANG_AR_SD	=	11	;    //	阿拉伯语+苏丹
		public static final int	LANG_AR_TN	=	12	;    //	阿拉伯语+突尼斯
		public static final int	LANG_AZ	=	13	;    //	阿塞拜疆
		public static final int	LANG_BE	=	14	;    //	白俄罗斯语
		public static final int	LANG_BG	=	15	;    //	保加利亚语
		public static final int	LANG_BN	=	16	;    //	孟加拉
		public static final int	LANG_BS	=	17	;    //	波斯尼亚语
		public static final int	LANG_CA	=	18	;    //	加泰罗尼亚语
		public static final int	LANG_CA_ES	=	19	;    //	加泰罗尼亚语+西班牙
		public static final int	LANG_CO	=	20	;    //	科西嘉
		public static final int	LANG_CS	=	21	;    //	捷克语
		public static final int	LANG_CY	=	22	;    //	威尔士语
		public static final int	LANG_DA	=	23	;    //	丹麦语
		public static final int	LANG_DE	=	24	;    //	德语
		public static final int	LANG_DE_CH	=	25	;    //	德语+瑞士
		public static final int	LANG_DE_LU	=	26	;    //	德语+卢森堡
		public static final int	LANG_EL	=	27	;    //	希腊语
		public static final int	LANG_EN	=	28	;    //	英语
		public static final int	LANG_EN_CA	=	29	;    //	英语+加拿大
		public static final int	LANG_EN_IE	=	30	;    //	英语+爱尔兰
		public static final int	LANG_EN_ZA	=	31	;    //	英语+南非
		public static final int	LANG_EO	=	32	;    //	世界语
		public static final int	LANG_ES	=	33	;    //	西班牙语
		public static final int	LANG_ES_BO	=	34	;    //	西班牙语+玻利维亚
		public static final int	LANG_ES_AR	=	35	;    //	西班牙语+阿根廷
		public static final int	LANG_ES_CO	=	36	;    //	西班牙语+哥伦比亚
		public static final int	LANG_ES_CR	=	37	;    //	西班牙语+哥斯达黎加
		public static final int	LANG_ES_ES	=	38	;    //	西班牙语+西班牙
		public static final int	LANG_ET	=	39	;    //	爱沙尼亚语
		public static final int	LANG_ES_PA	=	40	;    //	西班牙语+巴拿马
		public static final int	LANG_ES_SV	=	41	;    //	西班牙语+萨尔瓦多
		public static final int	LANG_ES_VE	=	42	;    //	西班牙语+委内瑞拉
		public static final int	LANG_ET_EE	=	43	;    //	爱沙尼亚语+爱沙尼亚
		public static final int	LANG_EU	=	44	;    //	巴斯克
		public static final int	LANG_FA	=	45	;    //	波斯语
		public static final int	LANG_FI	=	46	;    //	芬兰语
		public static final int	LANG_FR	=	47	;    //	法语
		public static final int	LANG_FR_BE	=	48	;    //	法语+比利时
		public static final int	LANG_FR_CA	=	49	;    //	法语+加拿大
		public static final int	LANG_FR_CH	=	50	;    //	法语+瑞士
		public static final int	LANG_FR_LU	=	51	;    //	法语+卢森堡
		public static final int	LANG_FY	=	52	;    //	弗里斯兰
		public static final int	LANG_GA	=	53	;    //	爱尔兰语
		public static final int	LANG_GD	=	54	;    //	苏格兰盖尔语
		public static final int	LANG_GL	=	55	;    //	加利西亚
		public static final int	LANG_GU	=	56	;    //	古吉拉特文
		public static final int	LANG_HA	=	57	;    //	豪撒语
		public static final int	LANG_HI	=	58	;    //	印地语
		public static final int	LANG_HR	=	59	;    //	克罗地亚语
		public static final int	LANG_HT	=	60	;    //	海地克里奥尔
		public static final int	LANG_HU	=	61	;    //	匈牙利语
		public static final int	LANG_HY	=	62	;    //	亚美尼亚
		public static final int	LANG_ID	=	63	;    //	印度尼西亚
		public static final int	LANG_IG	=	64	;    //	伊博
		public static final int	LANG_IS	=	65	;    //	冰岛语
		public static final int	LANG_IT	=	66	;    //	意大利语
		public static final int	LANG_IT_CH	=	67	;    //	意大利语+瑞士
		public static final int	LANG_JA	=	68	;    //	日语
		public static final int	LANG_KA	=	69	;    //	格鲁吉亚语
		public static final int	LANG_KK	=	70	;    //	哈萨克语
		public static final int	LANG_KN	=	71	;    //	卡纳达
		public static final int	LANG_KM	=	72	;    //	高棉语
		public static final int	LANG_KO	=	73	;    //	朝鲜语
		public static final int	LANG_KO_KR	=	74	;    //	朝鲜语+南朝鲜
		public static final int	LANG_KU	=	75	;    //	库尔德
		public static final int	LANG_KY	=	76	;    //	吉尔吉斯斯坦
		public static final int	LANG_LA	=	77	;    //	拉丁语
		public static final int	LANG_LB	=	78	;    //	卢森堡语
		public static final int	LANG_LO	=	79	;    //	老挝
		public static final int	LANG_LT	=	80	;    //	立陶宛语
		public static final int	LANG_LV	=	81	;    //	拉托维亚语+列托
		public static final int	LANG_MG	=	82	;    //	马尔加什
		public static final int	LANG_MI	=	83	;    //	毛利
		public static final int	LANG_MK	=	84	;    //	马其顿语
		public static final int	LANG_ML	=	85	;    //	马拉雅拉姆
		public static final int	LANG_MN	=	86	;    //	蒙古
		public static final int	LANG_MR	=	87	;    //	马拉地语
		public static final int	LANG_MS	=	88	;    //	马来语
		public static final int	LANG_MT	=	89	;    //	马耳他
		public static final int	LANG_MY	=	90	;    //	缅甸
		public static final int	LANG_NL	=	91	;    //	荷兰语
		public static final int	LANG_NL_BE	=	92	;    //	荷兰语+比利时
		public static final int	LANG_NE	=	93	;    //	尼泊尔
		public static final int	LANG_NO	=	94	;    //	挪威语
		public static final int	LANG_NY	=	95	;    //	齐切瓦语
		public static final int	LANG_PL	=	96	;    //	波兰语
		public static final int	LANG_PS	=	97	;    //	普什图语
		public static final int	LANG_PT	=	98	;    //	葡萄牙语
		public static final int	LANG_PT_BR	=	99	;    //	葡萄牙语+巴西
		public static final int	LANG_RO	=	100	;    //	罗马尼亚语
		public static final int	LANG_RU	=	101	;    //	俄语
		public static final int	LANG_SD	=	102	;    //	信德
		public static final int	LANG_SI	=	103	;    //	僧伽罗语
		public static final int	LANG_SK	=	104	;    //	斯洛伐克语
		public static final int	LANG_SL	=	105	;    //	斯洛语尼亚语
		public static final int	LANG_SM	=	106	;    //	萨摩亚
		public static final int	LANG_SN	=	107	;    //	修纳
		public static final int	LANG_SO	=	108	;    //	索马里
		public static final int	LANG_SQ	=	109	;    //	阿尔巴尼亚语
		public static final int	LANG_SR	=	110	;    //	塞尔维亚语
		public static final int	LANG_ST	=	111	;    //	塞索托语
		public static final int	LANG_SU	=	112	;    //	巽他语
		public static final int	LANG_SV	=	113	;    //	瑞典语
		public static final int	LANG_SV_SE	=	114	;    //	瑞典语+瑞典
		public static final int	LANG_SW	=	115	;    //	斯瓦希里语
		public static final int	LANG_TA	=	116	;    //	泰米尔
		public static final int	LANG_TE	=	117	;    //	泰卢固语
		public static final int	LANG_TG	=	118	;    //	塔吉克斯坦
		public static final int	LANG_TH	=	119	;    //	泰语
		public static final int	LANG_TL	=	120	;    //	菲律宾
		public static final int	LANG_TR	=	121	;    //	土耳其语
		public static final int	LANG_UK	=	122	;    //	乌克兰语
		public static final int	LANG_UR	=	123	;    //	乌尔都语
		public static final int	LANG_UZ	=	124	;    //	乌兹别克斯坦
		public static final int	LANG_VI	=	125	;    //	越南
		public static final int	LANG_XH	=	126	;    //	科萨
		public static final int	LANG_YID	=	127	;    //	意第绪语
		public static final int	LANG_YO	=	128	;    //	约鲁巴语
		public static final int	LANG_ZH	=	129	;    //	汉语
		public static final int	LANG_ZH_TW	=	130	;    //	繁体
		public static final int	LANG_ZU	=	131	;    //	/祖鲁语
	}
	
}
