//
//  YMVoiceService.m
//  YmTalkTestRef
//
//  Created by pinky on 2017/5/27.
//  Copyright © 2017年 Youme. All rights reserved.
//
#include <vector>
#include <map>
#import <AVFoundation/AVFoundation.h>

#ifdef MAC_OS
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif
#import "YMVoiceService.h"
#import "IYouMeVoiceEngine.h"
#import "IYouMeVideoCallback.h"
#import "YouMeEngineManagerForQiniu.hpp"
#import "YouMeEngineAudioMixerUtils.hpp"
#import "IYouMeVoiceEngine.h"

@interface  YMVoiceService()
-(void) youme_dispatch_async_choose:(dispatch_block_t)block;
@end

@implementation MemberChangeOC
@end

class YouMeVoiceImp : public IYouMeEventCallback, public IRestApiCallback,public IYouMeMemberChangeCallback, public IYouMePcmCallback,
public IYouMeChannelMsgCallback , public IYouMeAVStatisticCallback,public IYouMeVideoFrameCallback, public IYouMeAudioFrameCallback,
public IYouMeCustomDataCallback,public IYouMeVideoPreDecodeCallback,public IYouMeTranslateCallback
{
public:
    virtual void onEvent(const YouMeEvent event, const YouMeErrorCode error, const char * room, const char * param) override ;
    
    virtual void onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override;
    virtual void onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override;
    virtual void onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte) override;
    
    
    virtual  void onRequestRestAPI( int requestID, const YouMeErrorCode &iErrorCode, const  char* strQuery, const  char*  strResult )override;
    
    virtual void onMemberChange( const  char* channel, const char* listMemberChange , bool bUpdate)override;
	
	virtual void onBroadcast(const YouMeBroadcast bc, const  char* channel, const  char* param1, const char* param2, const  char* strContent)override;
    
    virtual void onAVStatistic( YouMeAVStatisticType type,  const char* userID,  int value ) override  ;
    
    virtual void onAudioFrameCallback(const char * userId, void* data, int len, uint64_t timestamp) override;
    virtual void onAudioFrameMixedCallback(void* data, int len, uint64_t timestamp) override;
    
    virtual void onVideoFrameCallback(const char * userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp)override;
    virtual void onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp)override;

    virtual void onVideoFrameCallbackGLESForIOS(const char * userId, void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp)override;
    virtual void onVideoFrameMixedCallbackGLESForIOS(void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp)override;
    
    virtual int  onVideoRenderFilterCallback(int textureId, int width, int height, int rotation, int mirror)override;
    
    virtual void OnCustomDataNotify(const void*pData, int iDataLen, unsigned long long ulTimeSpan) override;
    
    virtual void onTranslateTextComplete(YouMeErrorCode errorcode, unsigned int requestID, const std::string& text, YouMeLanguageCode srcLangCode, YouMeLanguageCode destLangCode) override;

    virtual void onVideoPreDecode(const char * userId, void* data, int dataSizeInByte, unsigned long long timestamp) override;
};

@interface YMVoiceService(){
    YouMeVoiceImp*  imp;
    std::mutex  viewMutex;
    OpenGLESView*   localVideoView;
    std::multimap<std::string, OpenGLESView*> mapVideoView;
    std::string localUserId;
    
    bool m_enableMainQueueDispatch;
}

- (void)onVideoFrameCallback: (NSString*)userId data:(void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp;

- (void)onVideoFrameMixedCallback: (void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp;

- (void)onVideoFrameCallbackForGLES:(NSString*)userId  pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp;

- (void)onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp;
@end

void YouMeVoiceImp::onEvent(const YouMeEvent event, const YouMeErrorCode error, const char * room, const char * param)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;

    if(event == YOUME_EVENT_JOIN_OK){
        //IYouMeVoiceEngine::getInstance()->setVideoCallback(this);
        IYouMeVoiceEngine::getInstance()->setVideoCallback(this);
    }
    if (event == YOUME_EVENT_QUERY_USERS_VIDEO_INFO) {
//        NSError *error;
//        NSMutableArray * userVideoInfoArray = [[NSMutableArray alloc] init];
//        NSMutableArray * videoInfoArray = [[NSMutableArray alloc] init];
//        NSData* jsonData=[[NSString stringWithUTF8String:param] dataUsingEncoding:NSUTF8StringEncoding];
//        NSArray *allarry = [NSKeyedUnarchiver unarchiveObjectWithData:jsonData];
//        if(allarry){
//        for (int i = 0; ; i++) {
//            NSDictionary *dict=[allarry objectAtIndex:i];
//            if (!dict) {
//                break;
//            }
//            NSString* userid = [dict objectForKey:@"userid"];
//            NSArray *resolutionArray=[dict objectForKey:@"resolution"];
//            if(resolutionArray){
//                int resolutionType = [[resolutionArray objectAtIndex:i] intValue];
//            }
//
//        }
//        }

        NSLog(@"query user video info :%@", [NSString stringWithUTF8String:param]);
    }
    
    [[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onYouMeEvent:errcode:roomid:param:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onYouMeEvent:event errcode:error
                            roomid:[NSString stringWithCString:room encoding:NSUTF8StringEncoding]
                             param:[NSString stringWithCString:param encoding:NSUTF8StringEncoding]];
    }];

}
void YouMeVoiceImp::OnCustomDataNotify(const void * data, int len, uint64_t timestamp) {
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    SEL sel = @selector(onCustomDataCallback:len:timestamp:);
    if(delegate && [delegate respondsToSelector:sel])
        [delegate onCustomDataCallback:data len:len timestamp:timestamp];
    //}];
    
}

void YouMeVoiceImp::onTranslateTextComplete(YouMeErrorCode errorcode, unsigned int requestID, const std::string& text, YouMeLanguageCode srcLangCode, YouMeLanguageCode destLangCode)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    SEL sel = @selector(onTranslateTextComplete:requestID:text:srcLangCode:destLangCode: );
    if( delegate && [delegate respondsToSelector: sel] )
    {
        [delegate onTranslateTextComplete:errorcode requestID:requestID text:[NSString stringWithCString:text.c_str() encoding:NSUTF8StringEncoding]  srcLangCode:srcLangCode destLangCode:destLangCode ];
    }
    
}

void YouMeVoiceImp::onPcmDataRemote(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    SEL sel = @selector(onPcmDataRemote:samplingRateHz:bytesPerSample:data:dataSizeInByte:);
    if(delegate && [delegate respondsToSelector:sel])
        [delegate onPcmDataRemote:channelNum samplingRateHz:samplingRateHz bytesPerSample:bytesPerSample data:data dataSizeInByte:dataSizeInByte];
    //}];
}

void YouMeVoiceImp::onPcmDataRecord(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    SEL sel = @selector(onPcmDataRecord:samplingRateHz:bytesPerSample:data:dataSizeInByte:);
    if(delegate && [delegate respondsToSelector:sel])
        [delegate onPcmDataRecord:channelNum samplingRateHz:samplingRateHz bytesPerSample:bytesPerSample data:data dataSizeInByte:dataSizeInByte];
    //}];
}

void YouMeVoiceImp::onPcmDataMix(int channelNum, int samplingRateHz, int bytesPerSample, void* data, int dataSizeInByte)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    SEL sel = @selector(onPcmDataMix:samplingRateHz:bytesPerSample:data:dataSizeInByte:);
    if(delegate && [delegate respondsToSelector:sel])
        [delegate onPcmDataMix:channelNum samplingRateHz:samplingRateHz bytesPerSample:bytesPerSample data:data dataSizeInByte:dataSizeInByte];
    //}];
}


void YouMeVoiceImp::onRequestRestAPI( int requestID, const YouMeErrorCode &iErrorCode, const  char* strQuery, const  char*  strResult )
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;

    
    NSString* query = [NSString stringWithUTF8String:strQuery];
    NSString* result = [NSString stringWithUTF8String:strResult];
    [[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onRequestRestAPI:iErrorCode:query:result:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onRequestRestAPI:requestID iErrorCode:iErrorCode  query:query  result:result ];
    }];
 
}

void YouMeVoiceImp::onMemberChange(const char* channel, const char* listMemberChange, bool isUpdate)
{

    if( [YMVoiceService getInstance].delegate ){
        NSString* channelID  = [NSString stringWithUTF8String: channel ];
        
        NSError *jsonDecodeError = nil;
        //{"channelid":"2418video","memchange":[{"isJoin":true,"userid":"1001590"}]}
        NSDictionary *dict = [NSJSONSerialization JSONObjectWithData:[[NSString stringWithUTF8String:listMemberChange] dataUsingEncoding:NSUTF8StringEncoding] options:kNilOptions error:&jsonDecodeError];
        if(dict != nil){
            NSMutableArray<MemberChangeOC*>* arr = [NSMutableArray arrayWithCapacity:15];
            NSArray *songArray = [dict objectForKey:@"memchange"];
            for (int i=0; i<songArray.count; i++) {
                MemberChangeOC* change = [MemberChangeOC new];
                NSDictionary *userInfo = [songArray objectAtIndex:i];
                change.userID = [userInfo objectForKey:@"userid"];
                change.isJoin = [(NSNumber*)[userInfo objectForKey:@"isJoin"] boolValue];
                [arr addObject: change ];
            }
            [[YMVoiceService getInstance] youme_dispatch_async_choose:^{
                [[YMVoiceService getInstance].delegate onMemberChange:channelID changeList:[arr copy] isUpdate:isUpdate ];
            }];
        }
    }
}

void YouMeVoiceImp::onBroadcast(const YouMeBroadcast bc, const  char* channel, const  char* param1, const  char* param2, const  char* strContent)
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;

    NSString* channelID  = [NSString stringWithUTF8String: channel ];
    NSString* p1  = [NSString stringWithUTF8String: param1 ];
    NSString* p2  = [NSString stringWithUTF8String: param2];
    NSString* content  = [NSString stringWithUTF8String: strContent ];
    SEL sel = @selector(onBroadcast:strChannelID:strParam1:strParam2:strContent:);
    [[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onBroadcast:bc strChannelID:channelID strParam1:p1 strParam2:p2 strContent:content ];
    }];
}



void YouMeVoiceImp::onAVStatistic( YouMeAVStatisticType type,  const char* userID,  int value )
{
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    NSString* strUserID = [NSString stringWithUTF8String:userID];
    
    [[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onAVStatistic:userID:value:);
        if(delegate && [delegate respondsToSelector:sel])
          [delegate onAVStatistic:type userID:strUserID value:value ];
    }];
}


void YouMeVoiceImp::onAudioFrameCallback(const char * userId, void * data, int len, uint64_t timestamp) {
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    NSString *str= [NSString stringWithCString:userId encoding:NSUTF8StringEncoding];
    
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onAudioFrameCallback:data:len:timestamp:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onAudioFrameCallback:str data:data len:len timestamp:timestamp];
   // }];

}




void YouMeVoiceImp::onAudioFrameMixedCallback(void * data, int len, uint64_t timestamp) {
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onAudioFrameMixedCallback:len:timestamp:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onAudioFrameMixedCallback:data len:len timestamp:timestamp];
    //}];
  
}

void YouMeVoiceImp::onVideoFrameCallback(const char * userId, void * data, int len, int width, int height, int fmt, uint64_t timestamp) {
    NSString *str= [NSString stringWithCString:userId encoding:NSUTF8StringEncoding];
    [[YMVoiceService getInstance] onVideoFrameCallback:str data:data len:len width:width height:height fmt:fmt timestamp:timestamp];

    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onVideoFrameCallback:data:len:width:height:fmt:timestamp:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onVideoFrameCallback:str data:data len:len width:width height:height fmt:fmt timestamp:timestamp];
    //}];
 
}

void YouMeVoiceImp::onVideoFrameMixedCallback(void * data, int len, int width, int height, int fmt, uint64_t timestamp) {
    [[YMVoiceService getInstance] onVideoFrameMixedCallback:data len:len width:width height:height fmt:fmt timestamp:timestamp];
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
   
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        SEL sel = @selector(onVideoFrameMixedCallback:len:width:height:fmt:timestamp:);
        if(delegate && [delegate respondsToSelector:sel])
            [delegate onVideoFrameMixedCallback:data len:len width:width height:height fmt:fmt timestamp:timestamp];
    //}];

}

void YouMeVoiceImp::onVideoFrameCallbackGLESForIOS(const char * userId, void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp){
    NSString *str= [NSString stringWithCString:userId encoding:NSUTF8StringEncoding];
    [[YMVoiceService getInstance] onVideoFrameCallbackForGLES:str pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:timestamp];

    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
        if(delegate && [delegate respondsToSelector:@selector(onVideoFrameCallbackForGLES:pixelBuffer:timestamp:)])
            [delegate onVideoFrameCallbackForGLES:str pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:timestamp];
    //}];

}
void YouMeVoiceImp::onVideoFrameMixedCallbackGLESForIOS(void* pixelBuffer, int width, int height, int fmt, uint64_t timestamp){
    [[YMVoiceService getInstance] onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:timestamp];
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
   
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    
        if(delegate && [delegate respondsToSelector:@selector(onVideoFrameMixedCallbackForGLES:timestamp:)])
            [delegate onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:timestamp];
    //}];
  
}

int  YouMeVoiceImp::onVideoRenderFilterCallback(int textureId, int width, int height, int rotation, int mirror){
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    SEL sel = @selector(onVideoRenderFilterCallback:width:height:rotation:mirror:);
    if(delegate && [delegate respondsToSelector:sel])
        return  [delegate onVideoRenderFilterCallback:textureId width:width height:height rotation:rotation mirror:mirror];
    return 0;
}

void YouMeVoiceImp::onVideoPreDecode(const char *userId, void *data, int dataSizeInByte, unsigned long long timestamp) {
    id<VoiceEngineCallback> delegate = [YMVoiceService getInstance].delegate;
    //[[YMVoiceService getInstance] youme_dispatch_async_choose:^{
    SEL sel = @selector(onVideoPreDecodeDataForUser:data:len:ts:);
    if(delegate && [delegate respondsToSelector:sel])
        [delegate onVideoPreDecodeDataForUser:userId data:data len:dataSizeInByte ts:(uint64_t)timestamp];
    //}];
}

static YMVoiceService *s_YMVoiceService_sharedInstance = nil;

extern void SetServerMode(SERVER_MODE serverMode);
extern void SetServerIpPort(const char* ip, int port);

@implementation YMVoiceService
//公共接口
+ (YMVoiceService *)getInstance
{
    @synchronized (self)
    {
        if (s_YMVoiceService_sharedInstance == nil)
        {
            s_YMVoiceService_sharedInstance = [self alloc];
            
            s_YMVoiceService_sharedInstance->imp = new YouMeVoiceImp();
            s_YMVoiceService_sharedInstance->localVideoView = nil;
        }
    }
    
    return s_YMVoiceService_sharedInstance;
}

+ (void)destroy
{
    delete s_YMVoiceService_sharedInstance->imp;
    s_YMVoiceService_sharedInstance->imp = nil;
    s_YMVoiceService_sharedInstance = nil;
}


- (void)setTestServer:(bool) isTest{
    if(isTest)
    {
       SetServerMode(SERVER_MODE_TEST);
    }else
    {
        SetServerMode(SERVER_MODE_FORMAL);
    }
}

- (void)setServerMode:(int) mode{
    SetServerMode((SERVER_MODE)mode);
}

- (void)setServerIP:(NSString*)ip port:(int)port {
    SetServerIpPort([ip UTF8String], port);
}

- (YouMeErrorCode_t)setTCPMode:(bool)bUseTcp
{
    return IYouMeVoiceEngine::getInstance()->setTCPMode(bUseTcp);
}
-(void) youme_dispatch_sync:(dispatch_block_t)block{
    
    if ([NSThread isMainThread]) {
        block();
    }
    else{
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}

-(void) youme_dispatch_async:(dispatch_block_t)block{
    
    if ([NSThread isMainThread]) {
        block();
    }
    else{
        dispatch_async(dispatch_get_main_queue(), block);
    }
}

-(void) youme_dispatch_async_choose:(dispatch_block_t)block{
    if ([NSThread isMainThread] || !m_enableMainQueueDispatch) {
        block();
    }
    else{
        dispatch_async(dispatch_get_main_queue(), block);
    }
}


- (int)setRenderMode:(NSString*)userId mode:(YouMeVideoRenderMode_t) mode
{
    if( !userId )
    {
        return -1;
    }
    std::unique_lock<std::mutex> viewLock(viewMutex);
    std::string strUserId = [userId UTF8String];
    auto iter = mapVideoView.find(strUserId);
    if (iter == mapVideoView.end())
    {
        return -1;
    }
    
    int count = mapVideoView.count(strUserId);
    for (int i = 0; i < count; i++, iter++)
    {
        __block OpenGLESView* glView =  iter->second;
        if( glView == nil )
        {
            return -1;
        }
        
        [self youme_dispatch_sync:^{
            [glView setRenderMode: (GLVideoRenderMode)mode ];
        }];
    }
    
    return 0;
}

- (int)setLocalVideoMirrorMode:(YouMeVideoMirrorMode_t) mode
{
    IYouMeVoiceEngine::getInstance()->setLocalVideoMirrorMode( mode );
    return 0;
}

- (int)setLocalVideoPreviewMirror:(bool) enable
{
    IYouMeVoiceEngine::getInstance()->setLocalVideoPreviewMirror( enable );
    return 0;
}

-(YMView*) setLocalRender:(YMView*)parentView{
    [self youme_dispatch_sync:^{
        //std::unique_lock<std::mutex> viewLock(viewMutex);
        if(localVideoView){
            [localVideoView removeFromSuperview];
            localVideoView = nil;
        }
        if(parentView){
           OpenGLESView* glView = nil;
            glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(0, 0, parentView.bounds.size.width, parentView.bounds.size.height)];
            
            glView.autoresizesSubviews = true;
#if TARGET_OS_OSX
            glView.autoresizingMask = NSViewHeightSizable | NSViewWidthSizable | NSViewMinXMargin |NSViewMaxYMargin;
            [parentView addSubview:glView positioned:NSWindowAbove relativeTo:nil];
#else
            glView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth
                                      | UIViewAutoresizingFlexibleLeftMargin |UIViewAutoresizingFlexibleTopMargin;
            [parentView insertSubview:glView atIndex:0];
#endif

            localVideoView = glView;
        }
    }];
    return localVideoView;
}

-(YMView*) createRender:(NSString*) userId parentView:(YMView*)parentView singleMode:(BOOL)singleMode
{
    if(!userId || !parentView)
        return nil;
    __block OpenGLESView* glView = nil;
     std::string strUserId = [userId UTF8String];
     if(singleMode){
        auto iter = mapVideoView.find(strUserId);
        if (iter != mapVideoView.end()) {
            [self deleteRender:userId glView:nil];
        }
     }
     [self youme_dispatch_sync:^{
      
      glView = [[OpenGLESView alloc] initWithFrame:CGRectMake(0, 0, parentView.bounds.size.width, parentView.bounds.size.height)];
         
      glView.autoresizesSubviews = true;
#if TARGET_OS_OSX
         glView.autoresizingMask = NSViewHeightSizable | NSViewWidthSizable | NSViewMinXMargin |NSViewMaxYMargin;
         [parentView addSubview:glView positioned:NSWindowAbove relativeTo:nil];
#else
         glView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleLeftMargin |UIViewAutoresizingFlexibleTopMargin;
         [parentView insertSubview:glView atIndex:0];
#endif
      std::unique_lock<std::mutex> viewLock(viewMutex);
      mapVideoView.insert(std::make_pair(strUserId, glView));
     }];
    return glView;
}



-(void) deleteRender:(NSString*) userId glView:(OpenGLESView*)glView {
    if(!userId)
        return ;
    std::string strUserId = [userId UTF8String];
    [self youme_dispatch_sync:^{
        std::unique_lock<std::mutex> viewLock(viewMutex);
        auto map_it = mapVideoView.find(strUserId);//删除(c,5)
        auto pos = mapVideoView.equal_range(map_it->first);//利用一对multimap<string,int>指向第一个出现(c,5)的位置和最后一个出现(c,5)的位置
        while (pos.first!=pos.second)
        {
            if ( glView == nil || pos.first->second == glView)//当pos指向5时
            {
                OpenGLESView* glView = pos.first->second;
                [glView removeFromSuperview];
                glView = nil;
                pos.first = mapVideoView.erase(pos.first);//删除后会改变pos迭代器，故赋值给自身，指向删除后的下一个键值对
            }
            else
                ++pos.first;//不进行删除操作则自增
        }
    }];
    IYouMeVoiceEngine::getInstance()->deleteRender([userId UTF8String]);
}


-(void) cleanRender:(NSString*) userId{
    if(!userId)
        return ;
    std::string strUserId = [userId UTF8String];
    [self youme_dispatch_sync:^{
        std::unique_lock<std::mutex> viewLock(viewMutex);
        
        int count = mapVideoView.count(strUserId);
        auto iter = mapVideoView.find(strUserId);
        for (int i = 0; i < count; i++, iter++)
        {
            OpenGLESView* glView = iter->second;
            //[glView setRenderBackgroudColor:[YMColor clearColor]];
            [glView clearFrame];
        }
        
    }];
}

-(void) deleteAllRender{
    
    [self youme_dispatch_sync:^{
        std::unique_lock<std::mutex> viewLock(viewMutex);
        std::map<std::string, OpenGLESView*>::iterator iter = mapVideoView.begin();
        while(iter != mapVideoView.end()){
            OpenGLESView* glView = iter->second;
            [glView removeFromSuperview];
            glView = nil;
            mapVideoView.erase(iter);
            iter = mapVideoView.begin();
        }
        if(localVideoView){
            [localVideoView removeFromSuperview];
            localVideoView = nil;
        }
    }];
}

-(int)getRenderCount:(NSString*) userId
{
    std::string strUserId = [userId UTF8String];
    //std::unique_lock<std::mutex> viewLock(viewMutex);
    return mapVideoView.count(strUserId);
}


- (YouMeErrorCode_t)setUserLogPath:(NSString *)path
{
    return IYouMeVoiceEngine::getInstance ()->setUserLogPath([path UTF8String]);
}

- (void)setExternalInputMode:(bool)bInputModeEnabled
{
    IYouMeVoiceEngine::getInstance()->setExternalInputMode(bInputModeEnabled);
}

- (YouMeErrorCode_t)initSDK:(id<VoiceEngineCallback>)delegate  appkey:(NSString*)appKey  appSecret:(NSString*)appSecret
        regionId:(YOUME_RTC_SERVER_REGION)regionId  serverRegionName:(NSString*) serverRegionName
{
    m_enableMainQueueDispatch = false;
    self.delegate = delegate;
    IYouMeVoiceEngine::getInstance()->setMemberChangeCallback( self->imp );
    IYouMeVoiceEngine::getInstance()->setRestApiCallback( self->imp );
    IYouMeVoiceEngine::getInstance()->setNotifyCallback( self->imp );
    IYouMeVoiceEngine::getInstance()->setAVStatisticCallback( self->imp );
    IYouMeVoiceEngine::getInstance()->setTranslateCallback( self->imp );
    YouMeEngineManagerForQiniu::getInstance()->setAudioFrameCallback(self->imp);
    IYouMeVoiceEngine::getInstance()->setRecvCustomDataCallback(self->imp);
    return IYouMeVoiceEngine::getInstance()->init( self->imp, [appKey UTF8String], [appSecret UTF8String],
                                                  ( YOUME_RTC_SERVER_REGION )regionId , [serverRegionName UTF8String]);
}


- (YouMeErrorCode_t)unInit
{
    IYouMeVoiceEngine::getInstance()->setVideoCallback(NULL);
    return IYouMeVoiceEngine::getInstance ()->unInit ();
}

- (void) setPcmCallbackEnable:(int)flag outputToSpeaker:(bool)bOutputToSpeaker nOutputSampleRate:(int)nOutputSampleRate nOutputChannel:(int)nOutputChannel{
    IYouMeVoiceEngine::getInstance()->setPcmCallbackEnable( self->imp, flag, bOutputToSpeaker ,nOutputSampleRate, nOutputChannel);
}

- (YouMeErrorCode_t) setVideoPreDecodeCallbackEnable:(bool)enable needDecodeandRender:(bool)decodeandRender{
    return IYouMeVoiceEngine::getInstance()->setVideoPreDecodeCallbackEnable( self->imp, decodeandRender );
}

//开启测试服
//-(void)setTestServer:(bool)isTest
//{
//    return IYouMeVoiceEngine::getInstance()->setTestServer(isTest);
//}

-(void)setServerRegion:(YOUME_RTC_SERVER_REGION)serverRegionId regionName:(NSString*)regionName bAppend:(bool)bAppend
{
    return IYouMeVoiceEngine::getInstance()->setServerRegion( ( YOUME_RTC_SERVER_REGION )serverRegionId, [regionName UTF8String], bAppend);
}

//切换语音输出设备，bOutputToSpeaker:true——扬声器，false——听筒，默认为true
- (YouMeErrorCode)setOutputToSpeaker:(bool)bOutputToSpeaker
{
    return IYouMeVoiceEngine::getInstance ()->setOutputToSpeaker(bOutputToSpeaker);
}

//设置扬声器静音,mute:true——静音，false——取消静音
-(void)setSpeakerMute:(bool)mute
{
    return IYouMeVoiceEngine::getInstance()->setSpeakerMute(mute);
}

//获取扬声器静音状态,return true——静音，false——没有静音
-(bool)getSpeakerMute
{
    return IYouMeVoiceEngine::getInstance()->getSpeakerMute();
}

//获取麦克风静音状态,return true——静音，false——没有静音
-(bool)getMicrophoneMute
{
    return IYouMeVoiceEngine::getInstance()->getMicrophoneMute();
}

//设置麦克风静音,mute:true——静音，false——取消静音
-(void)setMicrophoneMute:(bool)mute
{
    return IYouMeVoiceEngine::getInstance()->setMicrophoneMute(mute);
}


-(void) setOtherMicMute:(NSString *)strUserID  mute:(bool) mute
{
    IYouMeVoiceEngine::getInstance()->setOtherMicMute( [strUserID UTF8String], mute);
}
-(void) setOtherSpeakerMute: (NSString *)strUserID  mute:(bool) mute
{
    IYouMeVoiceEngine::getInstance()->setOtherSpeakerMute([strUserID UTF8String],mute);
}
-(void) setListenOtherVoice: (NSString *)strUserID  isOn:(bool) isOn
{
    IYouMeVoiceEngine::getInstance()->setListenOtherVoice([strUserID UTF8String],isOn);
}

//获取当前音量大小
- (unsigned int)getVolume
{
    return IYouMeVoiceEngine::getInstance ()->getVolume ();
}

//设置音量
- (void)setVolume:(unsigned int)uiVolume
{
    IYouMeVoiceEngine::getInstance ()->setVolume(uiVolume);
}

-(void)setAutoSendStatus:(bool)bAutoSend{
    IYouMeVoiceEngine::getInstance ()->setAutoSendStatus( bAutoSend );
}

-(int)setLocalConnectionInfo:(NSString *)strLocalIP localPort:(int)iLocalPort remoteIP:(NSString *)strRemoteIP remotePort:(int)iRemotePort
{
    return IYouMeVoiceEngine::getInstance()->setLocalConnectionInfo( [strLocalIP UTF8String], iLocalPort, [strRemoteIP UTF8String], iRemotePort, 0);
}

-(void)clearLocalConnectionInfo {
    IYouMeVoiceEngine::getInstance ()->clearLocalConnectionInfo();
}

-(int)setRouteChangeFlag:(bool)enable
{
    return IYouMeVoiceEngine::getInstance()->setRouteChangeFlag(enable);
}

//多人语音接口

//加入音频会议
- (YouMeErrorCode)joinChannelSingleMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole)userRole autoRecv:(bool)autoRecv
{
    localUserId = [strUserID UTF8String];
    return IYouMeVoiceEngine::getInstance()->joinChannelSingleMode([strUserID UTF8String],[strChannelID UTF8String], userRole, autoRecv);
}

- (YouMeErrorCode_t) joinChannelSingleMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole_t)userRole  joinAppKey:(NSString*) joinAppKey autoRecv:(bool)autoRecv
{
    localUserId = [strUserID UTF8String];
    return IYouMeVoiceEngine::getInstance()->joinChannelSingleMode([strUserID UTF8String],[strChannelID UTF8String], userRole, [joinAppKey UTF8String], autoRecv);
}

- (YouMeErrorCode_t) joinChannelMultiMode:(NSString *)strUserID channelID:(NSString *)strChannelID userRole:(YouMeUserRole_t)userRole
{
    localUserId = [strUserID UTF8String];
    return IYouMeVoiceEngine::getInstance()->joinChannelMultiMode([strUserID UTF8String],[strChannelID UTF8String], userRole);
}

- (YouMeErrorCode) speakToChannel:(NSString *)strChannelID{
    return IYouMeVoiceEngine::getInstance()->speakToChannel( [ strChannelID UTF8String ]);
}

//退出音频会议
- (YouMeErrorCode)leaveChannelMultiMode:(NSString *)strChannelID
{
    IYouMeVoiceEngine::getInstance()->setVideoCallback(NULL);
    return IYouMeVoiceEngine::getInstance ()->leaveChannelMultiMode ([strChannelID UTF8String]);
}

- (YouMeErrorCode)leaveChannelAll
{
    IYouMeVoiceEngine::getInstance()->setVideoCallback(NULL);
    return IYouMeVoiceEngine::getInstance ()->leaveChannelAll ();
}

//是否使用移动网络,默认不用
- (void)setUseMobileNetworkEnabled:(bool)bEnabled
{
    IYouMeVoiceEngine::getInstance ()->setUseMobileNetworkEnabled (bEnabled);
}

- (bool) getUseMobileNetworkEnabled{
    return IYouMeVoiceEngine::getInstance ()->getUseMobileNetworkEnabled();
}

- (void)setToken:(NSString*) token
{
    IYouMeVoiceEngine::getInstance()->setToken( [token UTF8String] );
}

- (void)setTokenV3:(NSString*) token timeStamp:(unsigned int)timeStamp
{
    IYouMeVoiceEngine::getInstance()->setToken( [token UTF8String ], timeStamp);
}

- (YouMeErrorCode)playBackgroundMusic:(NSString *)path   repeat:(bool)repeat
{
    return IYouMeVoiceEngine::getInstance ()->playBackgroundMusic([path UTF8String], repeat );
}

- (YouMeErrorCode)pauseBackgroundMusic
{
    return IYouMeVoiceEngine::getInstance()->pauseBackgroundMusic();
}

- (YouMeErrorCode)resumeBackgroundMusic
{
    return IYouMeVoiceEngine::getInstance()->resumeBackgroundMusic();
}

- (YouMeErrorCode)stopBackgroundMusic
{
    return IYouMeVoiceEngine::getInstance()->stopBackgroundMusic();
}

- (YouMeErrorCode)setBackgroundMusicVolume:(unsigned int)bgVolume
{
    return IYouMeVoiceEngine::getInstance()->setBackgroundMusicVolume(bgVolume);
}

//获取SDK版本号
- (int)getSDKVersion
{
    return IYouMeVoiceEngine::getInstance ()->getSDKVersion();
}

//  功能描述: 设置是否将麦克风声音旁路到扬声器输出，可以为主播等提供监听自己声音功能
- (YouMeErrorCode_t)setHeadsetMonitorMicOn:(bool)micEnabled
{
    return IYouMeVoiceEngine::getInstance()->setHeadsetMonitorOn(micEnabled);
}

//  功能描述: 设置是否用耳机监听自己的声音或背景音乐，可以为主播等提供监听自己声音和背景音乐的功能
- (YouMeErrorCode)setHeadsetMonitorMicOn:(bool)micEnabled BgmOn:(bool)bgmEnabled
{
    return IYouMeVoiceEngine::getInstance()->setHeadsetMonitorOn(micEnabled, bgmEnabled);
}

//  功能描述: 设置主播是否开启混响模式
- (YouMeErrorCode)setReverbEnabled:(bool)enabled
{
    return IYouMeVoiceEngine::getInstance()->setReverbEnabled(enabled);
}

- (YouMeErrorCode)setVadCallbackEnabled:(bool)enabled
{
    return IYouMeVoiceEngine::getInstance()->setVadCallbackEnabled(enabled);
}

- (YouMeErrorCode) setMicLevelCallback:(int) maxLevel{
    return IYouMeVoiceEngine::getInstance()->setMicLevelCallback(maxLevel);
}

- (YouMeErrorCode) setFarendVoiceLevelCallback:(int) maxLevel{
    return IYouMeVoiceEngine::getInstance()->setFarendVoiceLevelCallback(maxLevel);
}

- (YouMeErrorCode) setReleaseMicWhenMute:(bool) enabled{
    return IYouMeVoiceEngine::getInstance()->setReleaseMicWhenMute(enabled);
}

- (YouMeErrorCode_t) setExitCommModeWhenHeadsetPlugin:(bool) enabled{
    return IYouMeVoiceEngine::getInstance()->setExitCommModeWhenHeadsetPlugin(enabled);
}

- (void) setRecordingTimeMs:(unsigned int)timeMs
{
    return IYouMeVoiceEngine::getInstance()->setRecordingTimeMs(timeMs);
}

- (void) setPlayingTimeMs:(unsigned int)timeMs
{
    return IYouMeVoiceEngine::getInstance()->setPlayingTimeMs(timeMs);
}


- (YouMeErrorCode)pauseChannel
{
    return IYouMeVoiceEngine::getInstance ()->pauseChannel();
}


- (YouMeErrorCode_t)requestRestApi:(NSString*) strCommand strQueryBody:(NSString*) strQueryBody requestID:(int*)requestID{
    return IYouMeVoiceEngine::getInstance()->requestRestApi( [strCommand UTF8String], [strQueryBody UTF8String], requestID   );
}

- (YouMeUserRole_t) getUserRol {
    return IYouMeVoiceEngine::getInstance()->getUserRole();
}

- (YouMeErrorCode) getChannelUserList:(NSString*) channelID maxCount:(int)maxCount notifyMemChange:(bool)notifyMemChange {
    return IYouMeVoiceEngine::getInstance()->getChannelUserList([channelID UTF8String], maxCount, notifyMemChange );
}

- (YouMeErrorCode)resumeChannel
{
    return IYouMeVoiceEngine::getInstance ()->resumeChannel();
}

- (YouMeErrorCode_t) setGrabMicOption:(NSString*) channelID mode:(int)mode maxAllowCount:(int)maxAllowCount maxTalkTime:(int)maxTalkTime voteTime:(unsigned int)voteTime
{
	return IYouMeVoiceEngine::getInstance ()->setGrabMicOption([channelID UTF8String], mode, maxAllowCount, maxTalkTime, voteTime);
}

- (YouMeErrorCode_t) startGrabMicAction:(NSString*) channelID strContent:(NSString*) pContent
{
	return IYouMeVoiceEngine::getInstance ()->startGrabMicAction([channelID UTF8String], [pContent UTF8String]);
}

- (YouMeErrorCode_t) stopGrabMicAction:(NSString*) channelID strContent:(NSString*) pContent
{
	return IYouMeVoiceEngine::getInstance ()->stopGrabMicAction([channelID UTF8String], [pContent UTF8String]);
}

- (YouMeErrorCode_t) requestGrabMic:(NSString*) channelID score:(int)score isAutoOpenMic:(bool)isAutoOpenMic strContent:(NSString*) pContent
{
	return IYouMeVoiceEngine::getInstance ()->requestGrabMic([channelID UTF8String], score, isAutoOpenMic, [pContent UTF8String]);
}

- (YouMeErrorCode_t) releaseGrabMic:(NSString*) channelID
{
	return IYouMeVoiceEngine::getInstance ()->releaseGrabMic([channelID UTF8String]);
}

- (YouMeErrorCode) setInviteMicOption:(NSString*) channelID waitTimeout:(int)waitTimeout maxTalkTime:(int)maxTalkTime
{
	return IYouMeVoiceEngine::getInstance ()->setInviteMicOption([channelID UTF8String], waitTimeout, maxTalkTime);
}

- (YouMeErrorCode) requestInviteMic:(NSString*) channelID strUserID:(NSString*)pUserID strContent:(NSString*) pContent
{
	return IYouMeVoiceEngine::getInstance ()->requestInviteMic([channelID UTF8String], [pUserID UTF8String], [pContent UTF8String]);
}

- (YouMeErrorCode) responseInviteMic:(NSString*) pUserID isAccept:(bool)isAccept strContent:(NSString*) pContent
{
	return IYouMeVoiceEngine::getInstance ()->responseInviteMic([pUserID UTF8String], isAccept, [pContent UTF8String]);
}

- (YouMeErrorCode) stopInviteMic
{
	return IYouMeVoiceEngine::getInstance ()->stopInviteMic();
}

- (bool) releaseMicSync
{
    return IYouMeVoiceEngine::getInstance ()->releaseMicSync();
}

- (bool) resumeMicSync
{
    return IYouMeVoiceEngine::getInstance ()->resumeMicSync();
}

-(bool)setExternalFilterEnabled:(bool)enabled
{
    return IYouMeVoiceEngine::getInstance ()->setExternalFilterEnabled(enabled);
}

- (int)openVideoEncoder:(NSString *)path
{
    return IYouMeVoiceEngine::getInstance ()->openVideoEncoder([path UTF8String]);
}

// Camera capture
- (YouMeErrorCode)startCapture{
    return IYouMeVoiceEngine::getInstance ()->startCapture();
}

- (YouMeErrorCode)stopCapture{
    return IYouMeVoiceEngine::getInstance ()->stopCapture();
}

- (YouMeErrorCode)checkSharePermission
{
#ifdef MAC_OS
    // check screen capture permisson
    return IYouMeVoiceEngine::getInstance()->checkSharePermission();
#endif
    return YOUME_SUCCESS;
}

- (YouMeErrorCode)startShare:(int) mode windowid:(int)windowid
{
#ifdef MAC_OS
    return IYouMeVoiceEngine::getInstance()->StartShareStream(mode, windowid);
#else
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

-(void) setShareContext:(void*) context
{
#ifdef MAC_OS
    return IYouMeVoiceEngine::getInstance()->setShareContext(context);
#endif

}

- (void)stopShare
{
    IYouMeVoiceEngine::getInstance ()->StopShareStream();
}

- (YouMeErrorCode_t)startSaveScreen:(NSString*) filePath
{
    return IYouMeVoiceEngine::getInstance ()->startSaveScreen( [filePath UTF8String]);
}


//- (YouMeErrorCode_t)setTranscriberEnabled:(bool)enabled
//{
//    return IYouMeVoiceEngine::getInstance()->setTranscriberEnabled( enabled ,  self->imp );
//}

- (void)stopSaveScreen
{
    IYouMeVoiceEngine::getInstance ()->stopSaveScreen();
}

- (YouMeErrorCode)setVideoPreviewFps:(int)fps{
    return IYouMeVoiceEngine::getInstance ()->setVideoPreviewFps(fps);
}

- (YouMeErrorCode)setVideoFps:(int)fps{
    return IYouMeVoiceEngine::getInstance ()->setVideoFps(fps);
}

- (YouMeErrorCode)setVideoFpsForSecond:(int)fps{
    return IYouMeVoiceEngine::getInstance ()->setVideoFpsForSecond(fps);
}

- (YouMeErrorCode)setVideoFpsForShare:(int)fps{
    return IYouMeVoiceEngine::getInstance ()->setVideoFpsForShare(fps);
}

- (YouMeErrorCode)setVideoLocalResolutionWidth:(int)width height:(int)height{
    return IYouMeVoiceEngine::getInstance ()->setVideoLocalResolution(width, height);
}

- (YouMeErrorCode)setCaptureFrontCameraEnable:(bool)enable{
    return IYouMeVoiceEngine::getInstance ()->setCaptureFrontCameraEnable(enable);
}

- (YouMeErrorCode)switchCamera{
    return IYouMeVoiceEngine::getInstance ()->switchCamera();
}

- (YouMeErrorCode)resetCamera{
    return IYouMeVoiceEngine::getInstance ()->resetCamera();
}

- (int) getCameraCount{
    return IYouMeVoiceEngine::getInstance ()->getCameraCount();
}

- (NSString*) getCameraName:(int)cameraId{
    std::string str = IYouMeVoiceEngine::getInstance ()->getCameraName(cameraId);
    return [NSString stringWithCString:str.c_str() encoding:NSUTF8StringEncoding];
}

- (YouMeErrorCode_t) setOpenCameraId:(int)cameraId{
#if TARGET_OS_OSX
    return IYouMeVoiceEngine::getInstance ()->setOpenCameraId(cameraId);
#else
    return YOUME_ERROR_API_NOT_SUPPORTED;
#endif
}

#if TARGET_OS_OSX

- (int) getRecordDeviceCount
{
    return IYouMeVoiceEngine::getInstance()->getRecordDeviceCount();
}

- (boolean_t) getRecordDeviceInfo:(int)index deviceName:(NSString**)deviceName deviceUId:(NSString**)deviceUId
{
    char name[MAX_DEVICE_ID_LENGTH] = {0};
    char uid[MAX_DEVICE_ID_LENGTH] = {0};
    bool bRet = IYouMeVoiceEngine::getInstance()->getRecordDeviceInfo( index, name, uid );
    
    if( bRet )
    {
        *deviceName = [NSString stringWithCString: name encoding:NSUTF8StringEncoding];
        *deviceUId = [NSString stringWithCString: uid encoding:NSUTF8StringEncoding];
    }
    
    return bRet;

}

- (YouMeErrorCode_t) setRecordDevice:(NSString*) deviceUId
{
    return IYouMeVoiceEngine::getInstance()->setRecordDevice( [deviceUId UTF8String] );
}
#endif

- (YouMeErrorCode_t) sendMessage:(NSString*) channelID  strContent:(NSString*) strContent requestID:(int*)requestID{
    return IYouMeVoiceEngine::getInstance ()->sendMessage( [channelID UTF8String], [strContent UTF8String], NULL, requestID );
}

- (YouMeErrorCode_t) sendMessageToUser:(NSString*) channelID  strContent:(NSString*) strContent toUserID:(NSString*) toUserID requestID:(int*)requestID{
    return IYouMeVoiceEngine::getInstance ()->sendMessage( [channelID UTF8String], [strContent UTF8String], [toUserID UTF8String], requestID );
}

- (YouMeErrorCode_t) kickOtherFromChannel:(NSString*) userID  channelID:(NSString*)channelID lastTime:(int)lastTime
{
    return IYouMeVoiceEngine::getInstance ()->kickOtherFromChannel( [userID UTF8String], [channelID UTF8String], lastTime  );
}

- (void) setLogLevelforConsole:(YOUME_LOG_LEVEL_t) consoleLevel forFile:(YOUME_LOG_LEVEL_t)fileLevel
{
    return IYouMeVoiceEngine::getInstance ()->setLogLevel(consoleLevel, fileLevel);
}

- (YouMeErrorCode_t) setExternalInputSampleRate:(YOUME_SAMPLE_RATE_t)inputSampleRate mixedCallbackSampleRate:(YOUME_SAMPLE_RATE_t)mixedCallbackSampleRate
{
    return IYouMeVoiceEngine::getInstance ()->setExternalInputSampleRate( inputSampleRate, mixedCallbackSampleRate );
}

- (YouMeErrorCode_t)setVideoNetAdjustmode:(int)mode {
    return IYouMeVoiceEngine::getInstance ()->setVideoNetAdjustmode( mode );
}

- (YouMeErrorCode_t)setVideoNetResolutionWidth:(int)width height:(int)height {
    return IYouMeVoiceEngine::getInstance ()->setVideoNetResolution( width, height );
}

- (YouMeErrorCode_t)setVideoNetResolutionWidthForSecond:(int)width height:(int)height {
    return IYouMeVoiceEngine::getInstance ()->setVideoNetResolutionForSecond( width, height );
}

- (YouMeErrorCode_t)setVideoNetResolutionWidthForShare:(int)width height:(int)height {
    return IYouMeVoiceEngine::getInstance ()->setVideoNetResolutionForShare( width, height );
}

- (void) setAVStatisticInterval:(int) interval
{
    return IYouMeVoiceEngine::getInstance()->setAVStatisticInterval( interval );
}

- (void) setAudioQuality:(YOUME_AUDIO_QUALITY_t)quality
{
    return IYouMeVoiceEngine::getInstance ()->setAudioQuality( quality );
}

- (void) setVideoCodeBitrate:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate
{
    return IYouMeVoiceEngine::getInstance ()->setVideoCodeBitrate( maxBitrate, minBitrate );
}

- (void) setVideoCodeBitrateForSecond:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate
{
    return IYouMeVoiceEngine::getInstance ()->setVideoCodeBitrateForSecond( maxBitrate, minBitrate );
}

- (void) setVideoCodeBitrateForShare:(unsigned int) maxBitrate  minBitrate:(unsigned int ) minBitrate
{
    return IYouMeVoiceEngine::getInstance ()->setVideoCodeBitrateForShare( maxBitrate, minBitrate );
}

- (unsigned int) getCurrentVideoCodeBitrate
{
    return IYouMeVoiceEngine::getInstance ()->getCurrentVideoCodeBitrate();
}

- (YouMeErrorCode_t) setVBR:( bool) useVBR
{
    return IYouMeVoiceEngine::getInstance ()->setVBR( useVBR );
}

- (YouMeErrorCode_t) setVBRForSecond:( bool) useVBR
{
    return IYouMeVoiceEngine::getInstance ()->setVBRForSecond( useVBR );
}

- (YouMeErrorCode_t) setVBRForShare:( bool) useVBR
{
    return IYouMeVoiceEngine::getInstance ()->setVBRForShare( useVBR );
}

- (void) setVideoHardwareCodeEnable:(bool) bEnable
{
    return IYouMeVoiceEngine::getInstance ()->setVideoHardwareCodeEnable( bEnable );
}

- (void) setVideoFrameRawCbEnable:(bool) bEnable
{
    IYouMeVoiceEngine::getInstance ()->setVideoFrameRawCbEnabled( bEnable );
}

- (bool) getVideoHardwareCodeEnable
{
    return IYouMeVoiceEngine::getInstance ()->getVideoHardwareCodeEnable();
}

- (bool) getUseGL
{
    return IYouMeVoiceEngine::getInstance ()->getUseGL();
}

- (void) setVideoNoFrameTimeout:(int) timeout
{
    return IYouMeVoiceEngine::getInstance ()->setVideoNoFrameTimeout(timeout);
}

- (bool) isInited
{
    return IYouMeVoiceEngine::getInstance()->isInited();
}


- (YouMeErrorCode_t) setUserRole:(YouMeUserRole_t) eUserRole
{
    return IYouMeVoiceEngine::getInstance()->setUserRole( eUserRole );
}

- (YouMeUserRole_t) getUserRole
{
    return IYouMeVoiceEngine::getInstance()->getUserRole();
}


- (bool) isInChannel:(NSString*) strChannelID
{
    return IYouMeVoiceEngine::getInstance()->isInChannel( [strChannelID UTF8String]);
}

- (bool) isInChannel
{
    return IYouMeVoiceEngine::getInstance()->isInChannel( );
}

- (bool) isBackgroundMusicPlaying
{
    return IYouMeVoiceEngine::getInstance()->isBackgroundMusicPlaying();
}

-(YouMeErrorCode_t) queryUsersVideoInfo:(NSMutableArray*)userArray
{
    std::vector<std::string> vecUsers;
    NSUInteger size = [userArray count];
    for (NSUInteger i = 0; i < size; ++i) {
        NSString* userid = [userArray objectAtIndex:i];
        vecUsers.push_back([userid UTF8String]);
    }

    return IYouMeVoiceEngine::getInstance()->queryUsersVideoInfo(vecUsers);
}

-(YouMeErrorCode_t) setUsersVideoInfo:(NSMutableArray*)userArray resolutionArray:(NSMutableArray*)resolutionArray
{
    std::vector<IYouMeVoiceEngine::userVideoInfo> videoInfoList;
    NSUInteger size = [userArray count];
    for (NSUInteger i = 0; i < size; ++i) {
        IYouMeVoiceEngine::userVideoInfo userinfo;
        NSString* userid = [userArray objectAtIndex:i];
        NSString* resolutionType = [resolutionArray objectAtIndex:i];
        userinfo.userId = [userid UTF8String];
        userinfo.resolutionType =  [resolutionType intValue];
        videoInfoList.push_back(userinfo);
    }
    
    return IYouMeVoiceEngine::getInstance()->setUsersVideoInfo(videoInfoList);
}

- (YouMeErrorCode_t) openBeautify:(bool) open {
    return IYouMeVoiceEngine::getInstance()->openBeautify( open );
}

- (YouMeErrorCode_t) beautifyChanged:(float) param{
    return IYouMeVoiceEngine::getInstance()->beautifyChanged( param );
}

//- (YouMeErrorCode_t) stretchFace:(bool) stretch {
//    return IYouMeVoiceEngine::getInstance()->stretchFace( stretch );
//}

- (YouMeErrorCode_t) maskVideoByUserId:(NSString*) strUserId  mask:(bool) mask;
{
    return IYouMeVoiceEngine::getInstance()->maskVideoByUserId( [strUserId UTF8String] ,  mask );
}

- (bool)inputVideoFrame:(void *)data Len:(int)len Width:(int)width Height:(int)height Fmt:(int)fmt Rotation:(int)rotation Mirror:(int)mirror Timestamp:(uint64_t)timestamp {
    int ret = IYouMeVoiceEngine::getInstance()->inputVideoFrame(data, len, width, height, fmt, rotation, mirror, timestamp);
    if( ret == 0 ){
        return true;
    }
    else{
        return false;
    }
}

- (void)stopInputVideoFrame {
    IYouMeVoiceEngine::getInstance()->stopInputVideoFrame();
}

- (void)stopInputVideoFrameForShare {
    IYouMeVoiceEngine::getInstance()->stopInputVideoFrameForShare();
}

- (bool)inputAudioFrame:(void *)data Len:(int)len Timestamp:(uint64_t)timestamp {
    int ret = IYouMeVoiceEngine::getInstance()->inputAudioFrame(data, len, timestamp);
    if( ret == 0 ){
        return true;
    }
    else{
        return false;
    }
}

- (bool)inputAudioFrame:(void *)data Len:(int)len Timestamp:(uint64_t)timestamp ChannelNum:(int)channelnum bInterleaved:(bool)binterleaved {
    int ret = IYouMeVoiceEngine::getInstance()->inputAudioFrame(data, len, timestamp, channelnum, binterleaved);
    if( ret == 0 ){
        return true;
    }
    else{
        return false;
    }
}

- (bool)inputAudioFrameForMixStreamId:(int)streamId data:(void*)data length:(int)len frameInfo:(YMAudioFrameInfo_t)frameInfo timestamp:(uint64_t)timestamp {
    int ret = IYouMeVoiceEngine::getInstance()->inputAudioFrameForMix(streamId, data, len, frameInfo, timestamp);
    if( ret == 0 ){
        return true;
    }
    else{
        return false;
    }
}

- (bool)inputPixelBuffer:(CVPixelBufferRef)PixelBufferRef Width:(int)width Height:(int)height Fmt:(int)fmt Rotation:(int)rotation Mirror:(int)mirror Timestamp:(uint64_t)timestamp
{

    int ret = IYouMeVoiceEngine::getInstance()->inputVideoFrameForIOS((void*)PixelBufferRef, width, height, fmt, rotation, mirror, timestamp);
    if( ret == 0 ){
        return true;
    }
    else{
        return false;
    }
}

#if TARGET_OS_IPHONE
- (bool)inputPixelBufferShare:(RPSampleBufferType)sampleBufferType withBuffer:(CMSampleBufferRef)sampleBuffer
{
    bool isOK = YOUME_SUCCESS;
    
    switch (sampleBufferType) {
        // Handle video sample buffer
        case RPSampleBufferTypeVideo:
        {
             int rotation = 0;
             int width = 0;
             int height = 0;
             CVPixelBufferRef pixelBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
             if (@available(iOS 11.0, *)) {
                 CVPixelBufferLockBaseAddress(pixelBuf, kCVPixelBufferLock_ReadOnly);
                 NSNumber *orientation = (NSNumber *)CMGetAttachment(sampleBuffer, (__bridge CFStringRef)RPVideoSampleOrientationKey, NULL);
                 if (orientation.intValue == 8) {
                     rotation = 90;
                 } else if (orientation.intValue == 6) {
                     rotation = 270;
                 } else {
                     rotation = 0;
                 }
                
                 UInt64 recordTime = [[NSDate date] timeIntervalSince1970] * 1000;
                 width = (int)CVPixelBufferGetWidth(pixelBuf);
                 height = (int)CVPixelBufferGetHeight(pixelBuf);
                 isOK = IYouMeVoiceEngine::getInstance()->inputVideoFrameForIOSShare(pixelBuf, width, height, 0, rotation, 0, recordTime);
                 orientation = nil;
                 CVPixelBufferUnlockBaseAddress(pixelBuf, kCVPixelBufferLock_ReadOnly);
             }
            
            break;
        }
            
        case RPSampleBufferTypeAudioApp:
        {
             CFRetain(sampleBuffer);
             // 从samplebuffer中提取数据
             // 从samplebuffer中获取blockbuffer
             CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
             UInt64 recordTime = [[NSDate date] timeIntervalSince1970] * 1000;
             size_t  pcmLength = 0;
             char * pcmData = NULL;
            
             // 获取blockbuffer中的pcm数据的指针和长度
             OSStatus status =  CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, &pcmLength, &pcmData);
             if (status != noErr) {
                 NSLog(@"从block众获取pcm数据失败");
                 CFRelease(sampleBuffer);
                 return false;
             } else {
                 AudioStreamBasicDescription audioStreamBasicDescription = *CMAudioFormatDescriptionGetStreamBasicDescription((CMAudioFormatDescriptionRef)CMSampleBufferGetFormatDescription(sampleBuffer));
                 AudioFormatFlags flags = audioStreamBasicDescription.mFormatFlags;

                 int bytesPerFrame = audioStreamBasicDescription.mBytesPerFrame;
                 int channelsPerFrame = audioStreamBasicDescription.mChannelsPerFrame;
                 int sampleRate = audioStreamBasicDescription.mSampleRate;

                 bool isFloat = flags & kAudioFormatFlagIsFloat;
                 bool isBigEndian = flags & kAudioFormatFlagIsBigEndian;
                 bool isSignedInteger = flags & kAudioFormatFlagIsSignedInteger;
                 bool isPacked = flags & kAudioFormatFlagIsPacked;

                 bool isAlignedHigh = flags & kAudioFormatFlagIsAlignedHigh;
                 bool isNonInterleaved = flags & kAudioFormatFlagIsNonInterleaved;
                 bool isNonMixable = flags & kAudioFormatFlagIsNonMixable;

                 YMAudioFrameInfo_t audioMixFrameInfo;
                 audioMixFrameInfo.bytesPerFrame = bytesPerFrame;
                 audioMixFrameInfo.channels = channelsPerFrame;
                 audioMixFrameInfo.isBigEndian = isBigEndian;
                 audioMixFrameInfo.isFloat = isFloat;
                 audioMixFrameInfo.isNonInterleaved = isNonInterleaved;
                 audioMixFrameInfo.isSignedInteger = isSignedInteger;
                 audioMixFrameInfo.sampleRate = sampleRate;
                 audioMixFrameInfo.timestamp = 0;

                 [[YMVoiceService getInstance] inputAudioFrameForMixStreamId:1 data:pcmData length:pcmLength frameInfo:audioMixFrameInfo timestamp:recordTime];
                 CFRelease(sampleBuffer);
             }
            break;
        }
    
        case RPSampleBufferTypeAudioMic:
        {
             CFRetain(sampleBuffer);
             // 从samplebuffer中提取数据
             // 从samplebuffer中获取blockbuffer
             CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
             UInt64 recordTime = [[NSDate date] timeIntervalSince1970] * 1000;
             size_t  pcmLength = 0;
             char * pcmData = NULL;
            
             // 获取blockbuffer中的pcm数据的指针和长度
             OSStatus status =  CMBlockBufferGetDataPointer(blockBuffer, 0, NULL, &pcmLength, &pcmData);
             if (status != noErr) {
                 NSLog(@"从block众获取pcm数据失败");
                 CFRelease(sampleBuffer);
                 return false;
             } else {
                 AudioStreamBasicDescription audioStreamBasicDescription = *CMAudioFormatDescriptionGetStreamBasicDescription((CMAudioFormatDescriptionRef)CMSampleBufferGetFormatDescription(sampleBuffer));
                 AudioFormatFlags flags = audioStreamBasicDescription.mFormatFlags;

                 int bytesPerFrame = audioStreamBasicDescription.mBytesPerFrame;
                 int channelsPerFrame = audioStreamBasicDescription.mChannelsPerFrame;
                 int sampleRate = audioStreamBasicDescription.mSampleRate;

                 bool isFloat = flags & kAudioFormatFlagIsFloat;
                 bool isBigEndian = flags & kAudioFormatFlagIsBigEndian;
                 bool isSignedInteger = flags & kAudioFormatFlagIsSignedInteger;
                 bool isPacked = flags & kAudioFormatFlagIsPacked;

                 bool isAlignedHigh = flags & kAudioFormatFlagIsAlignedHigh;
                 bool isNonInterleaved = flags & kAudioFormatFlagIsNonInterleaved;
                 bool isNonMixable = flags & kAudioFormatFlagIsNonMixable;

                 YMAudioFrameInfo_t audioMixFrameInfo;
                 audioMixFrameInfo.bytesPerFrame = bytesPerFrame;
                 audioMixFrameInfo.channels = channelsPerFrame;
                 audioMixFrameInfo.isBigEndian = isBigEndian;
                 audioMixFrameInfo.isFloat = isFloat;
                 audioMixFrameInfo.isNonInterleaved = isNonInterleaved;
                 audioMixFrameInfo.isSignedInteger = isSignedInteger;
                 audioMixFrameInfo.sampleRate = sampleRate;
                 audioMixFrameInfo.timestamp = 0;
                 
                 [[YMVoiceService getInstance] inputAudioFrameForMixStreamId:2 data:pcmData length:pcmLength frameInfo:audioMixFrameInfo timestamp:recordTime];
                 CFRelease(sampleBuffer);
             }
            break;
        }
                        
        default:
        {
            NSLog(@"unmatch type");
            break;
        }
    }
    
    return true;
}
#endif

- (void)setMixVideoWidth:(int)width Height:(int)height {
    IYouMeVoiceEngine::getInstance()->setMixVideoSize(width, height);
}

- (void)addMixOverlayVideoUserId:(NSString*)userId PosX:(int)x PosY:(int)y PosZ:(int)z Width:(int)width Height:(int)height {
    //std::string user = [userId UTF8String];
    IYouMeVoiceEngine::getInstance()->addMixOverlayVideo([userId UTF8String], x, y, z, width, height);
}

- (void)removeMixOverlayVideoUserId:(NSString*)userId {
    //std::string user = [userId UTF8String];
    IYouMeVoiceEngine::getInstance()->removeMixOverlayVideo([userId UTF8String]);
}

- (void)removeAllOverlayVideo{
    IYouMeVoiceEngine::getInstance()->removeAllOverlayVideo();
}

-(YouMeErrorCode_t) setAudioMixerTrackSamplerate:(int)sampleRate {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerTrackSamplerate(sampleRate);
}

-(YouMeErrorCode_t) setAudioMixerTrackVolume:(int)volume {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerTrackVolume(volume);
}

-(YouMeErrorCode_t) setAudioMixerInputVolume:(int)volume {
    return YouMeEngineAudioMixerUtils::getInstance()->setAudioMixerInputVolume(volume);
}

-(YouMeErrorCode_t) pushAudioMixerTrackwithBuffer:(void*)pBuf nSizeInByte:(int)nSizeInByte nChannelNUm:(int)nChannelNUm nSampleRate:(int) nSampleRate nBytesPerSample:(int)nBytesPerSample bFloat:(bool) bFloat timestamp:(uint64_t) timestamp {
    return YouMeEngineAudioMixerUtils::getInstance()->pushAudioMixerTrack(pBuf, nSizeInByte, nChannelNUm, nSampleRate, nBytesPerSample, bFloat, timestamp);
}

-(YouMeErrorCode_t) inputAudioToMixwithIndexId:(NSString*)indexId Buffer:(void*)pBuf nSizeInByte:(int)nSizeInByte nChannelNUm:(int)nChannelNUm nSampleRate:(int) nSampleRate nBytesPerSample:(int)nBytesPerSample bFloat:(bool) bFloat timestamp:(uint64_t) timestamp {
    std::string indexIdstr = [indexId UTF8String];
    return YouMeEngineAudioMixerUtils::getInstance()->inputAudioToMix(indexIdstr, pBuf, nSizeInByte, nChannelNUm, nSampleRate, nBytesPerSample, bFloat, timestamp);
}

- (int)enableLocalVideoRender: (bool)enabled
{
    return IYouMeVoiceEngine::getInstance()->enableLocalVideoRender( enabled );
    
}
- (int)enableLocalVideoSend: (bool)enabled
{
    return IYouMeVoiceEngine::getInstance()->enableLocalVideoSend( enabled );
}

- (int)muteAllRemoteVideoStreams:(BOOL)mute
{
    return IYouMeVoiceEngine::getInstance()->muteAllRemoteVideoStreams( mute );
}

- (int)setDefaultMuteAllRemoteVideoStreams:(BOOL)mute
{
    return IYouMeVoiceEngine::getInstance()->setDefaultMuteAllRemoteVideoStreams( mute );
}

- (int)muteRemoteVideoStream:(NSString*)uid
                        mute:(BOOL)mute
{
    std::string user = [uid UTF8String];
    return IYouMeVoiceEngine::getInstance()->muteRemoteVideoStream(user , mute );
}

-(YouMeErrorCode_t)  startPush:(NSString*) pushUrl
{
    return IYouMeVoiceEngine::getInstance()->startPush( [pushUrl UTF8String] );
}
-(YouMeErrorCode_t)  stopPush
{
    return IYouMeVoiceEngine::getInstance()->stopPush();
}

-(YouMeErrorCode_t)  setPushMix:(NSString*) pushUrl  width:(int) width  height:(int) height
{
    return IYouMeVoiceEngine::getInstance()->setPushMix( [pushUrl UTF8String], width, height );
}
-(YouMeErrorCode_t)  clearPushMix
{
    return IYouMeVoiceEngine::getInstance()->clearPushMix( );
}
-(YouMeErrorCode_t)  addPushMixUser:(NSString*) userId x:(int)x y:(int)y z:(int)z width:(int)width height:(int)height 
{
    return IYouMeVoiceEngine::getInstance()->addPushMixUser( [userId UTF8String], x, y, z,width, height );
}
-(YouMeErrorCode_t)  removePushMixUser: (NSString*) userId
{
    return IYouMeVoiceEngine::getInstance()->removePushMixUser( [userId UTF8String] );
}

#if TARGET_OS_IPHONE
#pragma mark Video camera control
- (BOOL)isCameraZoomSupported
{
    return IYouMeVoiceEngine::getInstance()->isCameraZoomSupported();
}
- (CGFloat)setCameraZoomFactor:(CGFloat)zoomFactor
{
    return IYouMeVoiceEngine::getInstance()->setCameraZoomFactor( zoomFactor );
}

- (BOOL)isCameraFocusPositionInPreviewSupported
{
    return IYouMeVoiceEngine::getInstance()->isCameraFocusPositionInPreviewSupported( );
}
- (BOOL)setCameraFocusPositionInPreview:(CGPoint)position
{
    return IYouMeVoiceEngine::getInstance()->setCameraFocusPositionInPreview(  position.x, position.y );
}

- (BOOL)isCameraTorchSupported
{
    return IYouMeVoiceEngine::getInstance()->isCameraTorchSupported(  );
}
- (BOOL)setCameraTorchOn:(BOOL)isOn
{
    return IYouMeVoiceEngine::getInstance()->setCameraTorchOn( isOn );
}

- (BOOL)isCameraAutoFocusFaceModeSupported
{
    return IYouMeVoiceEngine::getInstance()->isCameraAutoFocusFaceModeSupported( );
}
- (BOOL)setCameraAutoFocusFaceModeEnabled:(BOOL)enable
{
    return IYouMeVoiceEngine::getInstance()->setCameraAutoFocusFaceModeEnabled( enable );
}
#endif

- (int)enableMainQueueDispatch:(BOOL)enabled
{
    m_enableMainQueueDispatch = enabled;
    return 0;
}


- (void)onVideoFrameCallback: (NSString*)userId data:(void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp{
    
    int videoWidth = width;
    int videoHeight = height;
    void* tmpBuffer = malloc(len);
    memcpy(tmpBuffer, data, len);
    std::string strUserId = [userId UTF8String];
    [self youme_dispatch_async:^{
        //std::unique_lock<std::mutex> viewLock(viewMutex);
        std::map<std::string, OpenGLESView*>::iterator iter = mapVideoView.find(strUserId);
        int count = mapVideoView.count(strUserId);
        for (int i = 0; i < count; i++, iter++)
        {
            OpenGLESView* glView = iter->second;
            [glView displayYUV420pData:tmpBuffer width:videoWidth height:videoHeight];
        }
        free(tmpBuffer);
    }];
    
}

- (void)onVideoFrameMixedCallback: (void*) data len:(int)len width:(int)width height:(int)height fmt:(int)fmt timestamp:(uint64_t)timestamp{
    int videoWidth = width;
    int videoHeight = height;
    void* tmpBuffer = malloc(len);
    memcpy(tmpBuffer, data, len);
    std::string strUserId = localUserId;
    [self youme_dispatch_async:^{
        //std::unique_lock<std::mutex> viewLock(viewMutex);
        std::map<std::string, OpenGLESView*>::iterator iter = mapVideoView.find(strUserId);
        int count = mapVideoView.count(strUserId);
        for (int i = 0; i < count; i++, iter++)
        {
            OpenGLESView* glView = iter->second;
            [glView displayYUV420pData:tmpBuffer width:videoWidth height:videoHeight];
        }
        if(localVideoView){
            [localVideoView displayYUV420pData:tmpBuffer width:videoWidth height:videoHeight];
        }
        free(tmpBuffer);
    }];
}

- (void)onVideoFrameCallbackForGLES:(NSString*)userId  pixelBuffer:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp{
    std::string strUserId = [userId UTF8String];
    CVPixelBufferRef  pixelBufferRef = CVPixelBufferRetain(pixelBuffer);
    
    [self youme_dispatch_async:^{
        NSMutableArray<OpenGLESView*>* views ;
    {
        std::unique_lock<std::mutex> viewLock(viewMutex);
        std::map<std::string, OpenGLESView*>::iterator iter = mapVideoView.find(strUserId);
        int count = mapVideoView.count(strUserId);
        views = [[NSMutableArray alloc] initWithCapacity:count];
        for (int i = 0; i < count; i++, iter++)
        {
            [views addObject: (OpenGLESView*)iter->second];
        }
    }
        for (int i = 0; i < [views count]; i++)
        {
            OpenGLESView* glView = views[i];
            if(glView != nullptr) [glView displayPixelBuffer:pixelBufferRef];
        }
    
        CVPixelBufferRelease(pixelBufferRef);
    }];
}

- (void)onVideoFrameMixedCallbackForGLES:(CVPixelBufferRef)pixelBuffer timestamp:(uint64_t)timestamp{
    std::string strUserId = localUserId;
    CVPixelBufferRef  pixelBufferRef = CVPixelBufferRetain(pixelBuffer);
    
    [self youme_dispatch_async:^{
    NSMutableArray<OpenGLESView*>* views ;
        {
        std::unique_lock<std::mutex> viewLock(viewMutex);
        std::map<std::string, OpenGLESView*>::iterator iter = mapVideoView.find(strUserId);
        int count = mapVideoView.count(strUserId);
        views = [[NSMutableArray alloc] initWithCapacity:count];
        for (int i = 0; i < count; i++, iter++)
        {
            [views addObject: (OpenGLESView*)iter->second];
        }
        if(localVideoView){
            [localVideoView displayPixelBuffer:pixelBufferRef];
        }
    }
        for (int i = 0; i < [views count]; i++)
        {
            OpenGLESView* glView = views[i];
            if(glView != nullptr)  [glView displayPixelBuffer:pixelBufferRef];
        }
    
        CVPixelBufferRelease(pixelBufferRef);
    }];
}

- (YouMeErrorCode_t)inputCustomData:(const void *)data Len:(int)len Timestamp:(uint64_t)timestamp {
    return IYouMeVoiceEngine::getInstance()->inputCustomData(data, len, timestamp);
}

-(YouMeErrorCode_t) screenRotationChange{
#if TARGET_OS_IPHONE
    auto orientation =  [[UIApplication sharedApplication] statusBarOrientation];
    switch (orientation) {
            // UIInterfaceOrientationPortraitUpsideDown, UIDeviceOrientationPortraitUpsideDown
        case UIInterfaceOrientationPortraitUpsideDown:
            return IYouMeVoiceEngine::getInstance()->resetResolutionForPortrait();
        case UIInterfaceOrientationLandscapeRight:
            return IYouMeVoiceEngine::getInstance()->switchResolutionForLandscape();
        case UIInterfaceOrientationLandscapeLeft:
           return IYouMeVoiceEngine::getInstance()->switchResolutionForLandscape();
        case UIInterfaceOrientationPortrait:
            return IYouMeVoiceEngine::getInstance()->resetResolutionForPortrait();
            break;
        default:
            break;
    }
#endif
    return YOUME_ERROR_NOT_PROCESS;
}

-(YouMeErrorCode_t) translateText:(unsigned int*) requestID  text:(NSString*)text destLangCode:(YouMeLanguageCode_t)destLangCode srcLangCode:(YouMeLanguageCode_t)srcLangCode
{
    return IYouMeVoiceEngine::getInstance()->translateText( requestID, [text UTF8String], destLangCode, srcLangCode );
}
@end


