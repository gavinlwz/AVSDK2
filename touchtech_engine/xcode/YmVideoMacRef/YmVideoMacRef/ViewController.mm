//
//  ViewController.m
//  YmVideoMacRef
//
//  Created by pinky on 2018/5/10.
//  Copyright © 2018年 youme. All rights reserved.
//

#import<CommonCrypto/CommonDigest.h>
#import "ViewController.h"
#import "ParamMacViewController.h"
#import "NSObject+SelectorOnMainThreadMutipleObjects.h"
#import "YMVoiceService.h"
#import "RenderGroupMacView.h"
#include <vector>
#include <string>


#define     strJoinAppKey   @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"
#define     strAppKey       @"YOUME5BE427937AF216E88E0F84C0EF148BD29B691556"
#define     strAppSecret    @"y1sepDnrmgatu/G8rx1nIKglCclvuA5tAvC0vXwlfZKOvPZfaUYOTkfAdUUtbziW8Z4HrsgpJtmV/RqhacllbXD3abvuXIBlrknqP+Bith9OHazsC1X96b3Inii6J7Und0/KaGf3xEzWx/t1E1SbdrbmBJ01D1mwn50O/9V0820BAAE="

//验证tokenV3
//#define     strJoinAppKey   @"YOUMEB16C6C9183F888FC210FBAB338DA6F04A19B93E5"
//#define     strAppKey       @"YOUMEB16C6C9183F888FC210FBAB338DA6F04A19B93E5"
//#define     strAppSecret    @"pHLyyX+I4UOTGF478IjJEdRtqh5K7MoRQZu6/4x6frAzuZA9ozWmuHX6C9bodBh/5NDOL1wWbQ7HUeLIbyBFt6Fs+zKj4YGbKGqE8fSHell5XsnebTRDyYZTFDIa1yxobNUdY7jVYH1masslVVvUY7WEFMtnOHnfzwSrbm8k+EEBAAE="

//#define     strJoinAppKey   @"YOUMEB2944FCBCA6C724E6F2607B4552A090814D2EB80"
//#define     strAppKey       @"YOUMEB2944FCBCA6C724E6F2607B4552A090814D2EB80"
//#define     strAppSecret    @"vO1NSmC8Akm7MtykFZTuoNLVAcEMLYhPBA5q9Lo1uUKOuzQEtDDobXKqkVLP0Pue4y5tRPe4uMchcBqPA8tcWSxPC5e20qs/vg+IHWPai0lN5rXIOXhmAWukx8PF6TTSr3radyGbPuMvelYMsUWs0T8ArDHtBSnmH0dsTySjDwcBAAE="

@interface ViewController()
{
    bool m_bInitOK;
    bool m_bInRoom;
    bool m_bUseTcpMode;
    
    bool m_bMicOpen;
    bool m_bSpeakerOpen;
    bool m_bCameraOpen;
    bool m_bShareOpen;
    bool m_bSaveShareOpen;
    bool m_bPlayMinorStream;
    bool m_bPreviewMirror;
    
    NSString* m_roomid;
    NSString* mLocalUserId;
    
    bool m_bMonitorMicOn;
    bool m_bMonitorBgOn;
    bool m_bMusicPlaying;
    bool m_bMusicPaused;
    
    bool m_bTestServer;
    std::vector<NSString*> m_vecMicDevices;
    
    NSString* m_curMicDevice ;
    
    bool bTokenV3;
@public
    ParamSetting* params;
}

@property (atomic,strong) NSMutableArray *userList;
@property (weak) IBOutlet RenderGroupMacView *videoGroup;
@property (atomic,retain) NSString* currentUserId;
@property (atomic,strong) OpenGLESView*  mGL20ViewFullScreen;

-(NSString*)strVersionFromInt:(uint32_t)nVersion;
@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    
    [self initParam];
    [self createUI];
    
    _tfUserID.stringValue = mLocalUserId;
    _tfChannelID.stringValue = m_roomid;
    m_curMicDevice = @"";
    bTokenV3 = false;
    
    self.videoGroup.layer.borderWidth = 1;
    self.videoGroup.layer.borderColor = [[NSColor blackColor] CGColor];
    [self.videoGroup cleanAll];

    [self comboBoxSetCamera].delegate = self;
    [self updateComboBoxValue];
    
    [self comboBoxSetMic].delegate = self;
    [self updateComboBoxValueMic];
    // Do any additional setup after loading the view.
    
    [self YMSDKSetup];
    
    [self.view addGestureRecognizer:[[NSPressGestureRecognizer  alloc] initWithTarget:self action:@selector(handleClickTap:)]];
    
    CGRect r = [[ NSScreen mainScreen ] frame];
    self.mGL20ViewFullScreen =  [[OpenGLESView alloc] initWithFrame:CGRectMake(0, 0, r.size.width, r.size.height)];
    [self.view addSubview:self.mGL20ViewFullScreen];
    self.mGL20ViewFullScreen.hidden = true ;
}

-(void)handleClickTap:(NSGestureRecognizer*)gest {
    //[self.view endEditing:YES];
    if (gest.state == NSGestureRecognizerStateBegan) {
        
    }
    NSLog(@"gest.state : %d", gest.state);
    //取消全屏显示
    if( self.mGL20ViewFullScreen.hidden == false )
    {
        self.mGL20ViewFullScreen.hidden = true;
        _videoGroup.strChooseUser = @"";
    }
}

-(void)initParam {
    
    params = [ParamSetting sharedInstance];
    
    _userList = [NSMutableArray new];
    m_bInitOK = false;
    m_bInRoom = false;
    m_bMicOpen = false;
    m_bSpeakerOpen = true;
    m_bCameraOpen = false;
    m_bShareOpen = false;
    m_bPlayMinorStream = false;
    m_bPreviewMirror = true;
    
    m_bMonitorMicOn = false;
    m_bMonitorBgOn = false;
    m_bMusicPlaying = false;
    m_bMusicPaused = false;
    
    m_bUseTcpMode = false;

    
    m_bTestServer = false;
    
    [self updateButtonState];
    
    m_roomid = @"12345";
    
    int value = (arc4random() % 1000) + 1;
    mLocalUserId = [NSString stringWithFormat:@"user_%d",value];
}

-(void)updateComboBoxValue {
    //初始化摄像头comboBox
    NSString *str_value=nil;
    id id_value;
    int cameraCount = [[YMVoiceService getInstance] getCameraCount];
    self.comboBoxSetCamera.delegate = nil;
    [[self comboBoxSetCamera] deselectItemAtIndex:[[self comboBoxSetCamera] indexOfSelectedItem]];
    [[self comboBoxSetCamera] removeAllItems];
    self.comboBoxSetCamera.delegate = self;
    if (cameraCount > 0) {
        for (int i = 0; i < cameraCount; i++) {
            str_value = [[YMVoiceService getInstance] getCameraName:i];
            id_value=str_value;
            [[self comboBoxSetCamera] addItemWithObjectValue:id_value];
        }
        [[self comboBoxSetCamera] selectItemAtIndex:0];

    }
}

-(void) updateComboBoxValueMic
{
    m_vecMicDevices.clear();
    [[self comboBoxSetMic] removeAllItems];
    
    [[self comboBoxSetMic] addItemWithObjectValue: @"Default"];
    m_vecMicDevices.push_back( @"" );
    
    NSString* strName = @"";
    NSString* strUid = @"";
    int curIndex = -1;
    int count = [[YMVoiceService getInstance] getRecordDeviceCount];
    if( count > 0 )
    {
        for( int i = 0 ; i < count ; i++ )
        {
            bool bRet = [[YMVoiceService getInstance] getRecordDeviceInfo: i  deviceName: &strName deviceUId:&strUid];
            if( bRet )
            {
                NSString* info =  [[NSString alloc] initWithFormat:@"%@ (%@)", strName,strUid ];
                [[self comboBoxSetMic] addItemWithObjectValue: info];
                //todo,这里要记录uid,要用uid来选择设备
                m_vecMicDevices.push_back( strUid );
                
                if( [m_curMicDevice isEqualToString: strUid ])
                {
                    curIndex = i;
                    [[self comboBoxSetMic] selectItemAtIndex:(curIndex +1)];
                }
                    
            }
        }
    }
    //原来的设备不见了
    if( curIndex == -1 )
    {
        [[YMVoiceService getInstance] setRecordDevice:@""];
        [[self comboBoxSetMic] selectItemAtIndex:0];
    }
}

-(NSString*)strVersionFromInt:(uint32_t)nVersion
{
    int main_ver = (nVersion >> 28) & 0xF;
    int minor_ver = (nVersion >> 22) & 0x3F;
    int release_number = (nVersion >> 14) & 0xFF;
    int build_number = nVersion & 0x00003FFF;
    
    return [NSString stringWithFormat:@"%d.%d.%d.%d", main_ver, minor_ver, release_number, build_number];
}

-(void)YMSDKSetup
{
    uint32_t ver = [[YMVoiceService getInstance] getSDKVersion];
    [[self labelVer] setStringValue: [@"ver:" stringByAppendingString:[self strVersionFromInt:ver]]];
    
    //========================== 设置Log等级 =========================================================
    [[YMVoiceService getInstance] setLogLevelforConsole:LOG_INFO forFile:LOG_INFO];
    
    if (bTokenV3) {
        [[YMVoiceService getInstance]setServerMode:SERVER_MODE_FIXED_IP_MCU];
        [[YMVoiceService getInstance]setServerIP:@"39.106.60.66" port:6006];
    }
    
    YouMeErrorCode_t err = [[YMVoiceService getInstance] initSDK: self appkey:strAppKey
                                appSecret:strAppSecret
                                 regionId:RTC_DEFAULT_SERVER
                         serverRegionName:@"" ];
    
    
}

- (void)createUI{
    self.videoGroup.layer.borderWidth = 1;
    self.videoGroup.layer.borderColor = [[NSColor blackColor] CGColor];
    [self.videoGroup cleanAll];
    [self.buttonEnter setEnabled:NO];
    [self.buttonSpeak setEnabled:NO];
    [self.buttonMic setEnabled:NO];
    [self.buttonOpenParam setEnabled:NO];
    [self.buttonMonitorMicphone setEnabled:NO];
    [self.buttonMonitorBg setEnabled:NO];
    [self.buttonOpenCamera setEnabled:NO];
    [self.buttonOpenShare setEnabled:NO];
    [self.buttonOpenSaveShare setEnabled:NO];
    [self.buttonSwitchStream setEnabled:NO];
    [self.buttonPlayMusic setEnabled:NO];
    [self.buttonPauseMusic setEnabled:NO];
    //==========================     Demo的简单UI      ==========================================================
}

-(void)initedUI
{
    [self.buttonEnter setEnabled:YES];
    [self.buttonOpenParam setEnabled:YES];
}

-(void)joiningUI
{
    [self.buttonOpenParam setEnabled:NO];
    [self.buttonEnter setTitle:@"进入频道中"];
    [self.buttonEnter setEnabled:NO];
    [self.buttonSpeak setEnabled:NO];
    [self.buttonMic setEnabled:NO];
    [self.buttonMonitorBg setEnabled:NO];
    [self.buttonMonitorMicphone setEnabled:NO];
    [self.buttonOpenCamera setEnabled:NO];
    [self.buttonOpenShare setEnabled:NO];
    [self.buttonOpenSaveShare setEnabled:NO];
    [self.buttonSwitchStream setEnabled:NO];
    [self.buttonPlayMusic setEnabled:NO];
    [self.buttonPauseMusic setEnabled:NO];
}

-(void)joinedUI
{
    [self.buttonOpenParam setEnabled:NO];
    [self.buttonEnter setTitle:@"离开频道" ];
    [self.buttonEnter setEnabled:YES];
    [self.buttonSpeak setEnabled:YES];
    [self.buttonMic setEnabled:YES];
    [self.buttonMonitorBg setEnabled:YES];
    [self.buttonMonitorMicphone setEnabled:YES];
    [self.buttonOpenCamera setEnabled:YES];
    [self.buttonOpenShare setEnabled:YES];
    [self.buttonOpenSaveShare setEnabled:YES];
    [self.buttonSwitchStream setEnabled:YES];
    [self.buttonPlayMusic setEnabled:YES];
    [self.buttonPauseMusic setEnabled:YES];
}

-(void)leavingUI
{
    [self.buttonOpenParam setEnabled:NO];
    [self.buttonEnter setTitle:@"离开频道中"];
    [self.buttonEnter setEnabled:NO];
    [self.buttonSpeak setEnabled:NO];
    [self.buttonMic setEnabled:NO];
    [self.buttonMonitorBg setEnabled:NO];
    [self.buttonMonitorMicphone setEnabled:NO];
    [self.buttonOpenCamera setEnabled:NO];
    [self.buttonOpenShare setEnabled:NO];
    [self.buttonOpenSaveShare setEnabled:NO];
    [self.buttonSwitchStream setEnabled:NO];
    [self.buttonPlayMusic setEnabled:NO];
    [self.buttonPauseMusic setEnabled:NO];
}

-(void)leavedUI
{
    [self refreshUI];
    [self.buttonSpeak setEnabled:NO];
    [self.buttonMic setEnabled:NO];
    [self.buttonMonitorBg setEnabled:NO];
    [self.buttonMonitorMicphone setEnabled:NO];
    [self.buttonOpenCamera setEnabled:NO];
    [self.buttonOpenShare setEnabled:NO];
    [self.buttonOpenSaveShare setEnabled:NO];
    [self.buttonSwitchStream setEnabled:NO];
    [self.buttonPlayMusic setEnabled:NO];
    [self.buttonPauseMusic setEnabled:NO];
}

- (void)refreshUI{
    _buttonSpeak.enabled = false;
    m_bCameraOpen = false;
    m_bShareOpen = false;
    m_bPlayMinorStream = false;
    
    [self.buttonEnter setTitle:@"进入频道" ];
    [self.buttonEnter setEnabled:YES];
    [self.buttonOpenParam setEnabled:YES];
    [_buttonSpeak setTitle:@"关闭扬声器" ];
    [self.buttonMic setTitle:@"关闭麦克风" ];
    [self.buttonMonitorMicphone setTitle:@"打开监听Mic" ];
    [self.buttonMonitorBg setTitle:@"打开监听Bgm" ];
}

-(NSString*)sha1:(NSString*)srcStr
{
    const char *cstr = [srcStr cStringUsingEncoding:NSUTF8StringEncoding];
    NSData *data = [NSData dataWithBytes:cstr length:srcStr.length];
    
    uint8_t digest[CC_SHA1_DIGEST_LENGTH];
    
    CC_SHA1(data.bytes, data.length, digest);
    
    NSMutableString* output = [NSMutableString stringWithCapacity:CC_SHA1_DIGEST_LENGTH * 2];
    
    for(int i = 0; i < CC_SHA1_DIGEST_LENGTH; i++)
        [output appendFormat:@"%02x", digest[i]];
    
    return output;
}

-(void)joinChannel
{
    self.currentUserId = _tfUserID.stringValue;
    m_roomid = _tfChannelID.stringValue;
    _tfTips.stringValue = @"正进入频道...";
    
    if( params )
    {
        // for local video render
        if( params->videoLocalWidth >= 0 && params->videoLocalHeight >= 0   )
        {
            [[YMVoiceService getInstance] setVideoLocalResolutionWidth:params->videoLocalWidth height:params->videoLocalHeight];
        }
        
        // for video encoder and send to remote (main stream)
        if( params->videoNetWidth >= 0 && params->videoNetHeight >= 0   )
        {
            [[YMVoiceService getInstance] setVideoNetResolutionWidth:params->videoNetWidth height:params->videoNetHeight];
        }
        
        // optional, default minor width and height is 0
        // for video encoder and send to remote (minor stream)
        if (params->videoMinorWidth > 0 && params->videoMinorHeight > 0) {
            [[YMVoiceService getInstance] setVideoNetResolutionWidthForSecond:params->videoMinorWidth height:params->videoMinorHeight];
        }
        
        // for video share,
        if (params->videoShareWidth >= 0 && params->videoShareHeight >= 0) {
            [[YMVoiceService getInstance] setVideoNetResolutionWidthForShare:params->videoShareWidth height:params->videoShareHeight];
        }
        
        [[YMVoiceService getInstance] setAVStatisticInterval: params->reportInterval];
        [[YMVoiceService getInstance] setVideoCodeBitrate: params->maxBitrate  minBitrate:params->minBitrate];
        
        if( params->bHighAudio ){
            [[YMVoiceService getInstance] setAudioQuality:HIGH_QUALITY];
        }
        else{
            [[YMVoiceService getInstance] setAudioQuality:LOW_QUALITY];
        }
        
        if( params->bHWEnable ){
            [[YMVoiceService getInstance] setVideoHardwareCodeEnable: TRUE ];
        }
        else{
            [[YMVoiceService getInstance] setVideoHardwareCodeEnable: false ];
        }
        
        [[YMVoiceService getInstance] setVideoFps:params->fps];
        [[YMVoiceService getInstance] setVBR:TRUE];
        
        [[YMVoiceService getInstance] setVideoFpsForShare:params->fps];
        
        [[YMVoiceService getInstance] setFarendVoiceLevelCallback: params->farendLevel ];
    }
    
    if (self->bTokenV3) {
        // 加入房间之前开始鉴权，客户端向sdk传入token和timestamp
        long curTime = [[NSDate date] timeIntervalSince1970];
        NSString *timeStamp = [NSString stringWithFormat:@"%ld", curTime];
        NSString* sha1src = [NSString stringWithFormat:@"%@%@%@%@%@", strAppKey, strAppKey, m_roomid, self.currentUserId, timeStamp];
        NSString* token = [self sha1:sha1src];
        NSLog(@"bruce >>> sha1src=%@", sha1src);
        NSLog(@"bruce >>> token=%@", token);
        [[YMVoiceService getInstance] setTokenV3: token timeStamp:curTime];
    } else {
        [[YMVoiceService getInstance] setToken:@""];
    }
    
    [[YMVoiceService getInstance] joinChannelSingleMode:mLocalUserId channelID:m_roomid userRole:YOUME_USER_HOST autoRecv:true];
    [[YMVoiceService getInstance] setAutoSendStatus: true ];
    
    m_bInRoom = true;
}

-(void)leaveChannel
{
    [self.userList removeAllObjects];
    [[YMVoiceService getInstance] leaveChannelAll];
    
    //退出频道后，重置标志位
    m_bShareOpen = m_bShareOpen ? false : m_bShareOpen;
    m_bSaveShareOpen = m_bSaveShareOpen ? false : m_bSaveShareOpen;
}

- (IBAction)onClickButtonUDPMode:(id)sender
{
    if( m_bInRoom )
        return;
    
    m_bUseTcpMode = !m_bUseTcpMode;
    [[YMVoiceService getInstance] setTCPMode: m_bUseTcpMode];
    
    if( m_bUseTcpMode )
    {
        [self.buttonTcp setTitle: @"Tcp Mode"];
        
    }
    else{
        [self.buttonTcp setTitle: @"Udp Mode"];
    }
}

- (IBAction)onClickButtonTestMode:(id)sender
{
    if( m_bInRoom )
        return;
    
    m_bTestServer = !m_bTestServer;
    [[YMVoiceService getInstance] setTestServer:m_bTestServer];
    
    if( m_bTestServer )
    {
        [self.buttonTestmode setTitle: @"测试服"];
    }
    else{
        [self.buttonTestmode setTitle: @"正式服"];
    }
    
    [[YMVoiceService getInstance] unInit];
    YouMeErrorCode_t err = [[YMVoiceService getInstance] initSDK: self appkey:strAppKey
                                                       appSecret:strAppSecret
                                                        regionId:RTC_DEFAULT_SERVER
                                                serverRegionName:@"" ];
    
}

- (IBAction)onClickButtonEnterRoom:(id)sender
{
    if(![[YMVoiceService getInstance] isInChannel:m_roomid]){
        [self joinChannel];
        [self joiningUI];
    }else{
        [self leavingUI];
        [self leaveChannel];
        [self.buttonEnter setEnabled:NO];
    }
}

- (void)comboBoxSelectionDidChange:(NSNotification *)notification {
    if( notification.object == self.comboBoxSetCamera )
    {
        if ([[self comboBoxSetCamera] indexOfSelectedItem] >= 0) {
            [[YMVoiceService getInstance] setOpenCameraId:[[self comboBoxSetCamera] indexOfSelectedItem]];
        }
    }
    else{
        int index =  [[self comboBoxSetMic] indexOfSelectedItem];
        if( index >= 0 ){
            [[YMVoiceService getInstance] setRecordDevice: m_vecMicDevices[index] ];
            m_curMicDevice = m_vecMicDevices[index];
            NSLog(@"set Mic device:%@", m_vecMicDevices[index] );
        }
    }
}

- (IBAction)onClickButtonOpenMic:(id)sender
{
    m_bMicOpen = !m_bMicOpen;
    [[YMVoiceService getInstance] setMicrophoneMute: !m_bMicOpen];
    [self updateButtonState];
}

- (IBAction)onClickButtonOpenSpeaker:(id)sender
{
    m_bSpeakerOpen = !m_bSpeakerOpen;
    [[YMVoiceService getInstance] setSpeakerMute:!m_bSpeakerOpen];
    [self updateButtonState];
    NSLog(@"set Speaker:%d", m_bSpeakerOpen);
}

- (IBAction)sliderActionVolume:(id)sender {
    NSSlider *slider = (NSSlider *)sender;
    
    int volume = slider.integerValue;
    
    [[YMVoiceService getInstance] setVolume: volume ];
    
    int getVolume = [[YMVoiceService getInstance] getVolume];
    [[self labelVolume] setStringValue: [NSString stringWithFormat:@"%d", getVolume ]];
}

- (IBAction)sliderActionBgVolume:(id)sender {
    NSSlider *slider = (NSSlider *)sender;
    
    int volume = slider.integerValue;
    
    [[YMVoiceService getInstance] setBackgroundMusicVolume: volume ];
    
    [[self labelBgVolume] setStringValue: [NSString stringWithFormat:@"%d", volume ]];
}


- (IBAction)onClickButtonMonitorMic:(id)sender
{
    m_bMonitorMicOn = !m_bMonitorMicOn;
    [[YMVoiceService getInstance] setHeadsetMonitorMicOn: m_bMonitorMicOn  BgmOn:m_bMonitorBgOn ];
    [self updateButtonState];
    
}
- (IBAction)onClickButtonMonitorBgMusic:(id)sender
{
    m_bMonitorBgOn = !m_bMonitorBgOn;
    [[YMVoiceService getInstance] setHeadsetMonitorMicOn: m_bMonitorMicOn  BgmOn:m_bMonitorBgOn ];
    [self updateButtonState];
}


- (IBAction)onClickButtonBackgroundMusic:(id)sender
{
    m_bMusicPlaying = !m_bMusicPlaying;
    if( m_bMusicPlaying )
    {
        NSString *path = [[NSBundle mainBundle] pathForResource:@"res/kanong" ofType:@"mp3"];
        [[YMVoiceService getInstance] playBackgroundMusic:path repeat:true];
    }
    else{
        [[YMVoiceService getInstance] stopBackgroundMusic];
    }
    [self updateButtonState];
}
- (IBAction)onClickButtonPauseMusic:(id)sender
{
    m_bMusicPaused = !m_bMusicPaused;
    if( m_bMusicPaused )
    {
        [[YMVoiceService getInstance] pauseBackgroundMusic];
    }
    else{
        [[YMVoiceService getInstance] resumeBackgroundMusic];
    }
    
    [self updateButtonState];
}

- (IBAction)onClickButtonRefreshMics:(id)sender
{
    [self updateComboBoxValueMic];
}

- (void) updateButtonState
{
    if( m_bInRoom )
    {
        [[self buttonEnter ] setTitle: @"离开频道"];
    }
    else{
        [[self buttonEnter ] setTitle: @"进入频道"];
    }
    
    if( m_bMicOpen )
    {
        [[self buttonMic ] setTitle: @"关闭麦克风"];
    }
    else{
        [[self buttonMic ] setTitle: @"打开麦克风"];
    }
    
    if( m_bSpeakerOpen )
    {
        [[self buttonSpeak ] setTitle: @"关闭扬声器"];
    }
    else{
        [[self buttonSpeak ] setTitle: @"打开扬声器"];
    }
    
    if( m_bMonitorMicOn )
    {
        [[self buttonMonitorMicphone ] setTitle: @"关闭监听Mic"];
    }
    else{
        [[self buttonMonitorMicphone ] setTitle: @"打开监听Mic"];
    }
    
    if( m_bMonitorBgOn )
    {
        [[self buttonMonitorBg ] setTitle: @"关闭监听Bgm"];
    }
    else{
        [[self buttonMonitorBg ] setTitle: @"打开监听Bgm"];
    }
    
    
    if( m_bMusicPlaying )
    {
        [[self buttonPlayMusic ] setTitle: @"停止播放"];
    }
    else{
        [[self buttonPlayMusic ] setTitle: @"播放背景音乐"];
    }
    
    if( m_bMusicPaused )
    {
        [[self buttonPauseMusic ] setTitle: @"恢复背景音乐"];
    }
    else{
        [[self buttonPauseMusic ] setTitle: @"暂停背景音乐"];
    }
    
    if( m_bCameraOpen )
    {
        [[self buttonOpenCamera ] setTitle: @"关闭摄像头"];
    }
    else{
        [[self buttonOpenCamera ] setTitle: @"打开摄像头"];
    }
    
    if( m_bShareOpen )
    {
        [[self buttonOpenShare ] setTitle: @"停止共享"];
    }
    else{
        [[self buttonOpenShare ] setTitle: @"开始共享"];
    }
    
    if( m_bSaveShareOpen )
    {
        [[self buttonOpenSaveShare ] setTitle: @"停止屏幕录像"];
    }
    else{
        [[self buttonOpenSaveShare ] setTitle: @"开始屏幕录像"];
    }
    
    if ( m_bPlayMinorStream )
    {
        [[self buttonSwitchStream ] setTitle: @"切换大流"];
    }
    else
    {
        [[self buttonSwitchStream ] setTitle: @"切换小流"];
    }
}


- (IBAction)onClickButtonOpenCamera:(id)sender {
    YouMeErrorCode_t ret = YOUME_SUCCESS;
    if (!m_bInRoom) {
        return;
    }
    if(m_bCameraOpen) {
        ret = [[YMVoiceService getInstance] stopCapture];
    }else {
        ret = [[YMVoiceService getInstance] startCapture];
    }
    if (ret == YOUME_SUCCESS) {
        m_bCameraOpen = !m_bCameraOpen;
        [self updateButtonState];
    }else {
        _tfTips.stringValue = m_bCameraOpen?@"关闭摄像头失败，请检查设备":@"打开摄像头失败，请检查设备";
    }

}

- (IBAction)onClickButtonOpenShare:(id)sender {
    YouMeErrorCode_t ret = YOUME_SUCCESS;
    if (!m_bInRoom) {
        return;
    }
    
    ret =  [[YMVoiceService getInstance] checkSharePermission];
    if (ret) {
        return;
    }
    
    if(m_bShareOpen) {
        [[YMVoiceService getInstance] stopShare];
        ret = YOUME_SUCCESS;
    }else {
        
        ret = [[YMVoiceService getInstance] startShare:3 windowid:0];
    }
    if (ret == YOUME_SUCCESS) {
        m_bShareOpen = !m_bShareOpen;
        [self updateButtonState];
    }else {
        _tfTips.stringValue = m_bShareOpen?@"关闭共享失败，请检查设备":@"打开共享失败，请检查设备";
    }
    
}

- (IBAction)onClickButtonSwitchStream:(id)sender {
    YouMeErrorCode_t ret = YOUME_SUCCESS;
    if (!m_bInRoom) {
        return;
    }
    
    [[YMVoiceService getInstance] setVideoNetAdjustmode:1 ];
    
    NSMutableArray * userArray = [[NSMutableArray alloc] init];
    NSMutableArray * resolutionArray = [[NSMutableArray alloc] init];

    if(m_bPlayMinorStream) {
        for(int i = 0; i < [self.userList count]; ++i)
        {
            [userArray addObject: [self.userList objectAtIndex:i] ];
            [resolutionArray addObject:@"0" ];
        }
    }else {
        for(int i = 0; i < [self.userList count]; ++i)
        {
            [userArray addObject: [self.userList objectAtIndex:i] ];
            [resolutionArray addObject:@"1" ];
        }
    }
    
    [[YMVoiceService getInstance] setUsersVideoInfo:userArray resolutionArray:resolutionArray];
    if (ret == YOUME_SUCCESS) {
        m_bPlayMinorStream = !m_bPlayMinorStream;
        [self updateButtonState];
    }else {
        _tfTips.stringValue = m_bPlayMinorStream?@"切换大流失败":@"切换小流失败";
    }
}

- (IBAction)onClickButtonPreviewMirror:(id)sender {
    YouMeErrorCode_t ret = YOUME_SUCCESS;
    if (!m_bInRoom) {
        return;
    }
    
    if (m_bPreviewMirror) {
        [[YMVoiceService getInstance] setLocalVideoPreviewMirror:false];
    } else {
        [[YMVoiceService getInstance] setLocalVideoPreviewMirror:true];
    }
    m_bPreviewMirror = !m_bPreviewMirror;
}

- (IBAction)onClickButtonOpenSaveShare:(id)sender {
    YouMeErrorCode_t ret = YOUME_SUCCESS;
    if (!m_bInRoom) {
        return;
    }
    
    if( m_bSaveShareOpen )
    {
        [[YMVoiceService getInstance] stopSaveScreen ];
    }
    else{
        NSString *documentPath = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
//        NSString* dir = @"/Users/pinky/Downloads/";
        NSString* filePath = [documentPath stringByAppendingString:@"/screen.mp4"];
        NSLog(@"screen save path:%@", filePath );
        ret = [[YMVoiceService getInstance] startSaveScreen: filePath ];
    }
    
    if (ret == YOUME_SUCCESS) {
        m_bSaveShareOpen = !m_bSaveShareOpen;
        [self updateButtonState];
    }else {
        _tfTips.stringValue = m_bSaveShareOpen?@"关闭录像失败，请检查设备":@"打开录像失败，请检查设备";
    }

}

- (IBAction)onClickButtonParam:(id)sender {

}

- (void)onTranslateTextComplete:(YouMeErrorCode_t)errorcode requestID:(unsigned int)requestID  text:(NSString*)text  srcLangCode:(YouMeLanguageCode_t)srcLangCode destLangCode:(YouMeLanguageCode_t)destLangCode;
{
    NSLog(@"onTranslateTextComplete:err:%d, requestId:%d, text:%@, src:%d, dest:%d", errorcode, requestID, text, srcLangCode, destLangCode );
}
- (void)onYouMeEvent:(YouMeEvent_t)eventType errcode:(YouMeErrorCode_t)iErrorCode roomid:(NSString *)roomid param:(NSString *)param
{
    NSLog(@"onYouMeEvent: type:%d, err:%d, room:%@,param:%@", eventType, iErrorCode, roomid, param );
    
    NSDictionary* dictPramEventMap = @{
                                       @(YOUME_EVENT_INIT_OK) : NSStringFromSelector(@selector(youmeEventInitOKWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_INIT_FAILED) : NSStringFromSelector(@selector(youmeEventInitFailedWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_JOIN_OK) : NSStringFromSelector(@selector(youmeEventJoinOKWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_JOIN_FAILED) : NSStringFromSelector(@selector(youmeEventJoinFailedWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_LEAVED_ALL) : NSStringFromSelector(@selector(youmeEventLeavedAllWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_ON) : NSStringFromSelector(@selector(youmeEventOthersVideoOnWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_SHUT_DOWN) : NSStringFromSelector(@selector(youmeEventOthersVideoOffWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_VIDEO_INPUT_STOP) : NSStringFromSelector(@selector(youmeEventOthersVideoOffWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_OTHERS_SHARE_INPUT_STOP) : NSStringFromSelector(@selector(youmeEventOthersVideoOffWithErrCode:roomid:param:)),
                                       @(YOUME_EVENT_CAMERA_DEVICE_DISCONNECT) : NSStringFromSelector(@selector(youmeEventCameraDisconnect:roomid:param:)),
                                       @(YOUME_EVENT_CAMERA_DEVICE_CONNECT) : NSStringFromSelector(@selector(youmeEventCameraConnect:roomid:param:)),
                                       @(YOUME_EVENT_AUDIO_INPUT_DEVICE_DISCONNECT) : NSStringFromSelector(@selector(youmeEventAudioInputDisconnect:roomid:param:)),
                                       @(YOUME_EVENT_AUDIO_INPUT_DEVICE_CONNECT) : NSStringFromSelector(@selector(youmeEventAudioInputConnect:roomid:param:)),
                                       };
    
    if ([dictPramEventMap objectForKey:@(eventType)]) {
        [self performSelectorOnMainThread:NSSelectorFromString([dictPramEventMap objectForKey:@(eventType)])
                               withObject:@(iErrorCode)
                               withObject:roomid
                               withObject:param
                            waitUntilDone:YES];
    } else {
        dispatch_async (dispatch_get_main_queue (), ^{
            NSString* strTmp = @"Evt: %d, err:%d, param:%@ ,room:%@ ";
            
            NSString* showInfo = [NSString stringWithFormat: strTmp, eventType, iErrorCode, param, roomid ];
            self.tfTips.stringValue = showInfo;
        });
    }
    
}


/////////////////////////////////////////////////////
// 回调逻辑处理
-(void)youmeEventInitOKWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    // SDK验证成功
    [self initedUI];
    self->m_bInitOK = TRUE;
    [[YMVoiceService getInstance] setVadCallbackEnabled:true];
    _tfTips.stringValue = @"SDK验证成功!";
}

-(void)youmeEventInitFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    //SDK验证失败
    self->m_bInitOK = FALSE;
    _tfTips.stringValue = @"SDK验证失败!";
}

-(void)youmeEventJoinOKWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    self->m_bInRoom = true;
    [self.buttonTcp setEnabled: false];
    [self.buttonTestmode setEnabled: false];
    [self joinedUI];
    [self updateButtonState];
    _buttonSpeak.enabled = true;
    //获取房间成员列表和变化通知
    [[YMVoiceService getInstance] getChannelUserList:roomid maxCount:100 notifyMemChange:true];
    [[YMVoiceService getInstance] setMicrophoneMute: !m_bMicOpen];
    [[YMVoiceService getInstance] setSpeakerMute: !m_bSpeakerOpen];
    [[YMVoiceService getInstance] setHeadsetMonitorMicOn: m_bMonitorMicOn  BgmOn: m_bMonitorBgOn ];
    [[YMVoiceService getInstance] setVolume: 80 ];
    
    [[YMVoiceService getInstance] setBackgroundMusicVolume: 80];
    self.tfTips.stringValue = @"加入房间成功";
    if(params->beauty){
        [[YMVoiceService getInstance] openBeautify:true];
        [[YMVoiceService getInstance] beautifyChanged:0.5];
    }else{
        [[YMVoiceService getInstance] openBeautify:false];
    }
    
    [[YMVoiceService getInstance] setPcmCallbackEnable: PcmCallbackFlag_Mix| PcmCallbackFlag_Remote | PcmCallbackFlag_Record  outputToSpeaker:true nOutputSampleRate:48000 nOutputChannel:2];
    
    [[YMVoiceService getInstance] setMixVideoWidth:params->videoLocalWidth Height:params->videoLocalHeight];
    [[YMVoiceService getInstance] addMixOverlayVideoUserId:mLocalUserId PosX:0 PosY:0 PosZ:0 Width:params->videoLocalWidth Height:params->videoLocalHeight];
    NSString* userid = param;
    [self.videoGroup applyRenderViewWithTag:userid];
    
    [[YMVoiceService getInstance] setVideoNetAdjustmode:1 ];
}

-(void)youmeEventJoinFailedWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    [self leavedUI];
    self->m_bInRoom = false;
    [self.buttonTcp setEnabled: true];
    [self.buttonTestmode setEnabled:true];
    NSString* strTmp = [NSString stringWithFormat:@"加入房间失败,errcode:%ld", [iErrorCodeNum integerValue]];
    NSLog(@"%@", strTmp);
    self.tfTips.stringValue = strTmp;
}


-(void)youmeEventLeavedAllWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    [self leavedUI];
    self->m_bInRoom = false;
    [self.buttonTcp setEnabled: true];
    [self.buttonTestmode setEnabled: true];
    self->m_bMusicPaused = false;
    self->m_bMusicPlaying = false;
    [self updateButtonState];
    NSString* strErrorCode = [NSString stringWithFormat:@"已离开房间,errcode:%ld",[iErrorCodeNum integerValue]];
    self.tfTips.stringValue = strErrorCode;

    [self.userList removeAllObjects];
    
    [self.videoGroup cleanAll];
    
    [[YMVoiceService getInstance] removeAllOverlayVideo];
}

-(void)youmeEventOthersVideoOnWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSString* userid = param;
    //int renderid = [[YMVoiceService getInstance] createRender:userid];  //createRender后，视频流会通知到frameRender回调
    if([userid isEqualToString:self.currentUserId]){
        if ([self.videoGroup applyRenderViewWithTag:userid]) {
            //[self.userList setObject:userid forKey:[NSNumber numberWithInt:renderid]];
        }
    }else{
        if ([self.videoGroup applyRenderViewWithTag:userid]) {
            // [self.userList setObject:userid forKey:[NSNumber numberWithInt:renderid]];
        }
    }
    
    //[self.userList addObject:userid];
    NSLog(@"User:%@ start video input", param  );
//    [[YMVoiceService getInstance] maskVideoByUserId:userid mask:1];
}

-(void)youmeEventOthersVideoOffWithErrCode:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    NSString* userid = param;
    //int renderid = [[YMVoiceService getInstance] createRender:userid];  //createRender后，视频流会通知到frameRender回调
    if([userid isEqualToString:self.currentUserId]){
        if ([self.videoGroup deattachRenderViewWithTag:userid]) {
            //[self.userList setObject:userid forKey:[NSNumber numberWithInt:renderid]];
        }
    }else{
        if ([self.videoGroup deattachRenderViewWithTag:userid]) {
            // [self.userList setObject:userid forKey:[NSNumber numberWithInt:renderid]];
        }
    }
//    [self.userList removeObject:param];
    NSLog(@"User:%@ stop video input", param  );
}

-(void)youmeEventCameraDisconnect:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    [self updateComboBoxValue];
}

-(void)youmeEventCameraConnect:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param {
    [self updateComboBoxValue];
}

-(void)youmeEventAudioInputDisconnect:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param
{
    _tfTips.stringValue = [[NSString alloc] initWithFormat:@"Input Plugout:(%@)" ,param];
//    [self updateComboBoxValueMic];
}

-(void)youmeEventAudioInputConnect:(NSNumber*)iErrorCodeNum  roomid:(NSString *)roomid param:(NSString*)param
{
    _tfTips.stringValue = [[NSString alloc] initWithFormat:@"Input Plugin:(%@)", param];
//    [self updateComboBoxValueMic];
}

- (void)setRepresentedObject:(id)representedObject {
    [super setRepresentedObject:representedObject];

    // Update the view, if already loaded.
}


///////////////////////////////////////////////////////////////////////////////////////
// sdk回调实现  VoiceEngineCallback

- (void)onVideoFrameCallback: (NSString*)userId data:(void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp{
    //NSLog(@" ----------- onVideoFrameCallback !!!!");
    if(userId != nil){
        char* pTmpBuffer = (char*)malloc(len);
        memcpy(pTmpBuffer, data, len);
        dispatch_async (dispatch_get_main_queue (), ^{
            
            //处理全屏显示
            if( self.videoGroup.strChooseUser == userId )
            {
                //全屏尺寸初始化
                if( self.mGL20ViewFullScreen.hidden == true ){
                    CGRect r = [[ NSScreen mainScreen ] frame];
                    float widthRate = (float)r.size.width / width;
                    float heightRate = (float)r.size.height / height;
                    float rate = widthRate > heightRate ? heightRate : widthRate;
                    
                    self.mGL20ViewFullScreen.bounds = CGRectMake(0, 0, width * rate, height * rate);
                    self.mGL20ViewFullScreen.hidden = false;
                }
                
                [self.mGL20ViewFullScreen displayYUV420pData:pTmpBuffer width:width height:height];
            }
            
            [self.videoGroup displayRenderViewWithData:(void*)pTmpBuffer width:width height:height tag:userId];
            free(pTmpBuffer);
        });
    }
}
- (void)onVideoFrameMixedCallback: (void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp{
//    NSLog(@" ----------- onVideoFrameMixedCallback !!!!, %lld", timestamp );
    NSString* userId = mLocalUserId;
    if(userId != nil){
        char* pTmpBuffer = (char*)malloc(len);
        memcpy(pTmpBuffer, data, len);
//
        
        //处理全屏显示
        if( self.videoGroup.strChooseUser == userId )
        {
            dispatch_async (dispatch_get_main_queue (), ^{
                //全屏尺寸初始化
                if( self.mGL20ViewFullScreen.hidden == true ){
                    CGRect r = [[ NSScreen mainScreen ] frame];
                    float widthRate = (float)r.size.width / width;
                    float heightRate = (float)r.size.height / height;
                    float rate = widthRate > heightRate ? heightRate : widthRate;
                    
                    self.mGL20ViewFullScreen.bounds = CGRectMake(0, 0, width * rate, height * rate);
                    self.mGL20ViewFullScreen.hidden = false;
                }
            });
            
            [self.mGL20ViewFullScreen displayYUV420pData:pTmpBuffer width:width height:height];
        }
        
        [self.videoGroup displayRenderViewWithData:(void*)pTmpBuffer width:width height:height tag:userId];
        free(pTmpBuffer);
    }
    
}
- (void)onVideoFrameCallbackForGLES:(NSString*)userId  pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp{
    //NSLog(@" ----------- onVideoFrameCallbackForGLES !!!!");
    CVPixelBufferRef  pixelBufferRef = CVPixelBufferRetain(pixelBuffer);
    dispatch_async (dispatch_get_main_queue (), ^{
        if(userId != nil){
            //处理全屏显示
            if( self.videoGroup.strChooseUser == userId )
            {
                //全屏尺寸初始化
                if( self.mGL20ViewFullScreen.hidden == true ){
                    size_t nWidth = CVPixelBufferGetWidth(pixelBuffer);
                    size_t nHeight = CVPixelBufferGetHeight(pixelBuffer);
                    CGRect r = [[ NSScreen mainScreen ] frame];
                    float widthRate = (float)r.size.width / nWidth;
                    float heightRate = (float)r.size.height / nHeight;
                    float rate = widthRate > heightRate ? heightRate : widthRate;
                    
                    self.mGL20ViewFullScreen.bounds = CGRectMake(0, 0, nWidth * rate, nHeight * rate);
                    self.mGL20ViewFullScreen.hidden = false;
                }
                
                
                [self.mGL20ViewFullScreen displayPixelBuffer:pixelBuffer];
            }
            
            [self.videoGroup displayRenderViewWithData:pixelBuffer tag:userId];
            
        }
        CVPixelBufferRelease(pixelBufferRef);
    });
}
- (void)onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp{
//    NSLog(@" ----------- onVideoFrameMixedCallbackForGLES !!!!, %lld", timestamp);
    CVPixelBufferRef  pixelBufferRef = CVPixelBufferRetain(pixelBuffer);
    dispatch_async (dispatch_get_main_queue (), ^{
        if( self.videoGroup.strChooseUser == mLocalUserId )
        {
                //全屏尺寸初始化
                if( self.mGL20ViewFullScreen.hidden == true ){
                    size_t nWidth = CVPixelBufferGetWidth(pixelBuffer);
                    size_t nHeight = CVPixelBufferGetHeight(pixelBuffer);
                    CGRect r = [[ NSScreen mainScreen ] frame];
                    float widthRate = (float)r.size.width / nWidth;
                    float heightRate = (float)r.size.height / nHeight;
                    float rate = widthRate > heightRate ? heightRate : widthRate;
                    
                    self.mGL20ViewFullScreen.bounds = CGRectMake(0, 0, nWidth * rate, nHeight * rate);
                    self.mGL20ViewFullScreen.hidden = false;
                }
            
            
            [self.mGL20ViewFullScreen displayPixelBuffer:pixelBuffer];
        }
        
        [self.videoGroup displayRenderViewWithData:pixelBuffer  tag:mLocalUserId];
        CVPixelBufferRelease(pixelBufferRef);
    });
}

- (void)onMemberChange:(NSString*) channelID changeList:(NSArray*) changeList isUpdate:(bool) isUpdate
{
    NSUInteger count = [changeList count];
    NSLog(@"MemberChagne:%@, count:%ld",channelID, (unsigned long)count );
    for( int i = 0 ; i < count ;i++ ){
        MemberChangeOC* change = [changeList objectAtIndex:i ];
        if( change.isJoin == 1 ){
            NSLog(@"%@ 进入", change.userID);
            [self.userList addObject:change.userID];
        }
        else{
            NSLog(@"%@ 离开了", change.userID );
            [self.userList removeObject:change.userID];
            //在用户离开的时候可以删除它的渲染资源
            
            dispatch_async (dispatch_get_main_queue (), ^{
                [self.videoGroup deattachRenderViewWithTag:change.userID];
//                NSArray* keys = [self.userList allKeysForObject:change.userID];
//                if(keys!=nil && keys.count>0){
//                    // [[YMVoiceService getInstance] deleteRender:[keys[0] intValue]];
//                }
            });
        }
    }
}

- (void)onPcmDataRemote: (int)channelNum samplingRateHz:(int)samplingRateHz bytesPerSample:(int)bytesPerSample data:(void*) data dataSizeInByte:(int)dataSizeInByte {
    
        NSString *txtPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"dump_onPcmDataRemote.pcm"];
    
        NSFileManager *fileManager = [NSFileManager defaultManager];
        if(![fileManager fileExistsAtPath:txtPath isDirectory:FALSE]){
            [fileManager createFileAtPath:txtPath contents:nil attributes:nil];
        }
        NSFileHandle *handle = [NSFileHandle fileHandleForUpdatingAtPath:txtPath];
        [handle seekToEndOfFile];
        [handle writeData:[NSData dataWithBytes:data length:dataSizeInByte]];
        [handle closeFile];
}

- (void)onPcmDataRecord: (int)channelNum samplingRateHz:(int)samplingRateHz bytesPerSample:(int)bytesPerSample data:(void*) data dataSizeInByte:(int)dataSizeInByte {
    
    NSString *txtPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"dump_onPcmDataRecord.pcm"];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if(![fileManager fileExistsAtPath:txtPath isDirectory:FALSE]){
        [fileManager createFileAtPath:txtPath contents:nil attributes:nil];
    }
    NSFileHandle *handle = [NSFileHandle fileHandleForUpdatingAtPath:txtPath];
    [handle seekToEndOfFile];
    [handle writeData:[NSData dataWithBytes:data length:dataSizeInByte]];
    [handle closeFile];
}

- (void)onPcmDataMix: (int)channelNum samplingRateHz:(int)samplingRateHz bytesPerSample:(int)bytesPerSample data:(void*) data dataSizeInByte:(int)dataSizeInByte {
    
    NSString *txtPath = [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent:@"dump_onPcmDataMix.pcm"];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if(![fileManager fileExistsAtPath:txtPath isDirectory:FALSE]){
        [fileManager createFileAtPath:txtPath contents:nil attributes:nil];
    }
    NSFileHandle *handle = [NSFileHandle fileHandleForUpdatingAtPath:txtPath];
    [handle seekToEndOfFile];
    [handle writeData:[NSData dataWithBytes:data length:dataSizeInByte]];
    [handle closeFile];
}


@end
