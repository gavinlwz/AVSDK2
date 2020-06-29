// #include <obs-module.h>
// #include <util/darray.h>
// #include <util/threading.h>
// #include <util/platform.h>

#import "tsk_cond.h"
#import "tsk_time.h"

#import <CoreGraphics/CGWindow.h>
#import <Cocoa/Cocoa.h>
#import <pthread.h>

#import <Cocoa/Cocoa.h>
#import "BordWindow.h"
#import <CoreGraphics/CGWindow.h>
#import <Foundation/Foundation.h>
#import <CoreFoundation/CoreFoundation.h>
#import <ApplicationServices/ApplicationServices.h>


#import "window_utils.h"

void * g_context = NULL;

void setWindowShareContext( void* context)
{
    g_context = context;
}

class BorderManager
{
public:
    BordWindow* m_bord;
    YMWindowController* m_bordWindowC;
    AXObserverRef m_observer_ref;
    
    AXUIElementRef m_appRef;
    AXUIElementRef m_windowRef;
    CGWindowID m_windowId;
    NSTimer* m_timer;
    
public:
    void startBorder( CGWindowID windowID , int ownerID);
    void stopBorder();
    
    void showBorder( bool bForceShow );
    void hideBorder( bool bReShow );
    
    //定时更新接口
    void updateBorder();
    void putWindowFront();
    
    int getScreenHeight();
    NSRect getBordWindowFrame( CGPoint pointOri, CGSize sizeOri );
    bool IsWindowMinid( AXUIElementRef windowRef  );
    bool IsWindowOnScreen();
    
    void AddNotification(NSString* notification,  AXUIElementRef element );
    void RemoveNotification(NSString* notification,  AXUIElementRef element);
};

BorderManager* g_borderManager = NULL ;


struct window_capture {
	// obs_source_t *source;

	struct cocoa_window window;

	//CGRect              bounds;
	//CGWindowListOption  window_option;
	CGWindowImageOption image_option;

	CGColorSpaceRef color_space;

    uint8_t* video_buf;
    size_t   video_buf_size;

	pthread_t  capture_thread;
	tsk_cond_handle_t* capture_event;

	VideoScreenCb videoCb;
	int      frame_rate;
	uint64_t next_frame_time;	// next time to get video frame
	int pthread_exit_flag;	// pthread exit flag, default false
};

static CGImageRef get_image(struct window_capture *wc)
{
	NSArray *arr = (__bridge NSArray*)CGWindowListCreate(
			kCGWindowListOptionIncludingWindow,
			wc->window.window_id);
	//[arr autorelease];

	if (arr.count)
		return CGWindowListCreateImage(CGRectNull,
				kCGWindowListOptionIncludingWindow,
				wc->window.window_id, wc->image_option);

	if (!find_window(&wc->window, NULL, false))
		return NULL;

	return CGWindowListCreateImage(CGRectNull,
			kCGWindowListOptionIncludingWindow,
			wc->window.window_id, wc->image_option);
}

CVPixelBufferRef pixelBufferFromCGImage(CGImageRef image ){
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey,
                             [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey,
//                             [NSNumber numberWithBool:YES], kCVPixelBufferIOSurfacePropertiesKey,
//                             [NSNumber numberWithBool:YES], kCVPixelBufferMetalCompatibilityKey,
                             nil];

    CVPixelBufferRef pxbuffer = NULL;

    CGFloat frameWidth = CGImageGetWidth(image);
    CGFloat frameHeight = CGImageGetHeight(image);
    size_t bytesPerRow = CGImageGetBytesPerRow(image);
    
    CFDataRef  dataFromImageDataProvider = CGDataProviderCopyData(CGImageGetDataProvider(image));
    GLubyte  *imageData = (GLubyte *)CFDataGetBytePtr(dataFromImageDataProvider);
        
    CVPixelBufferCreateWithBytes(kCFAllocatorDefault,
                                 frameWidth,
                                 frameHeight,
                                 kCVPixelFormatType_32BGRA,
                                 imageData,
                                 bytesPerRow,
                                 NULL,
                                 NULL,
                                 (__bridge CFDictionaryRef)options,
                                 &pxbuffer);
      
    CFRelease(dataFromImageDataProvider);

//    CVReturn status = CVPixelBufferCreate(kCFAllocatorDefault,
//                                          frameWidth,
//                                          frameHeight,
//                                          kCVPixelFormatType_32BGRA,
//                                          (__bridge CFDictionaryRef) options,
//                                          &pxbuffer);
//
////    NSParameterAssert(status == kCVReturnSuccess && pxbuffer != NULL);
//
//    CVPixelBufferLockBaseAddress(pxbuffer, 0);
//    void *pxdata = CVPixelBufferGetBaseAddress(pxbuffer);
////    NSParameterAssert(pxdata != NULL);
//
//    CGColorSpaceRef rgbColorSpace = CGColorSpaceCreateDeviceRGB();
//
//    CGContextRef context = CGBitmapContextCreate(pxdata,
//                                                 frameWidth,
//                                                 frameHeight,
//                                                 8,
//                                                 CVPixelBufferGetBytesPerRow(pxbuffer),
//                                                 rgbColorSpace,
//                                                 kCGBitmapByteOrder32Host | kCGImageAlphaPremultipliedFirst);
//                                                 //kCGImageAlphaNoneSkipFirst);
////    NSParameterAssert(context);
//    CGContextConcatCTM(context, CGAffineTransformIdentity);
//    CGContextDrawImage(context, CGRectMake(0,
//                                           0,
//                                           frameWidth,
//                                           frameHeight),
//                       image);
//    CGColorSpaceRelease(rgbColorSpace);
//    CGContextRelease(context);
//
//    CVPixelBufferUnlockBaseAddress(pxbuffer, 0);

    return pxbuffer;
}

static inline void capture_frame(struct window_capture *wc)
{
	// get video frame timestamp
	uint64_t   ts  = tsk_gettimeofday_ms();
	CGImageRef img = get_image(wc);
	if (!img)
		return;

	size_t width  = CGImageGetWidth(img);
	size_t height = CGImageGetHeight(img);

	CGRect rect = {{0.0, 0.0}, {width*1.0, height*1.0}};
	//da_reserve(wc->buffer, width * height * 4);
    
//    wc->video_buf_size = width * height * 4;
//	wc->video_buf = (uint8_t*)malloc(wc->video_buf_size);
//    uint8_t *data = wc->video_buf;

    //wc->color_space,
//	CGContextRef cg_context = CGBitmapContextCreate(data, width, height,
//			8, width * 4,wc->color_space ,
//			kCGBitmapByteOrder32Host |
//			kCGImageAlphaPremultipliedFirst);
//	CGContextSetBlendMode(cg_context, kCGBlendModeCopy);
//	CGContextDrawImage(cg_context, rect, img);
//	CGContextRelease(cg_context);
//	CGImageRelease(img);

	// below is callback video raw data to caller 
	uint32_t size = width * height * 4;
    CVPixelBufferRef buff = pixelBufferFromCGImage( img  );
    
    wc->videoCb((char*)buff, size, width, height, ts);
    
    CGImageRelease(img);
    CVPixelBufferRelease(buff);
	
    if (wc->video_buf) {
        free(wc->video_buf);
        wc->video_buf = NULL;
    }
	
	#if 0 
	struct obs_source_frame frame = {
		.format      = VIDEO_FORMAT_BGRA,
		.width       = width,
		.height      = height,
		.data[0]     = data,
		.linesize[0] = width * 4,
		.timestamp   = ts,
	};

	obs_source_output_video(wc->source, &frame);
	#endif
}

static void *capture_thread(void *data)
{
	struct window_capture *wc = (struct window_capture *)data;
    
	wc->next_frame_time = os_gettime_ns()/1000;	//us

	int64_t delay = 0;
	uint64_t now_time = 0;
	uint64_t avg_delay = 1000/(wc->frame_rate * 1.0)*1000;	// us
    wc->image_option |= (kCGWindowImageNominalResolution | kCGWindowImageBoundsIgnoreFraming ) ; // 采用最低分辨率
    //kCGWindowImageBoundsIgnoreFraming
    
	while (!wc->pthread_exit_flag) {
//        tsk_cond_wait(wc->capture_event, -1);

		@autoreleasepool {
			capture_frame(wc);
		}

		now_time = os_gettime_ns()/1000; // us
		wc->next_frame_time += avg_delay;
		delay = wc->next_frame_time - now_time;

		if (delay > 0) {
			usleep(delay);
		} else {
			// nothing
		}
        
        dispatch_async(dispatch_get_main_queue(), ^(void){
            if(g_borderManager) g_borderManager->updateBorder();
        });
	}

	return NULL;
}

static inline void *window_capture_create_internal(obs_data_t *settings)
{
    int len = sizeof(struct window_capture);
	struct window_capture *wc = (struct window_capture *)malloc(len);
    if (wc) {
        memset(wc, 0, len);
    } else {
        return nil;
    }
	// wc->source = source;

	wc->color_space = CGColorSpaceCreateDeviceRGB();

	//da_init(wc->buffer);
    wc->video_buf = NULL;
    wc->video_buf_size = 0;
    wc->frame_rate = 15;
    
	init_window(&wc->window, settings);

	// 预览是否显示窗口阴影
	// wc->image_option = obs_data_get_bool(settings, "show_shadow") ?
	// 	kCGWindowImageDefault : kCGWindowImageBoundsIgnoreFraming;
	wc->image_option = kCGWindowImageDefault;
	
	wc->videoCb = NULL;

	wc->capture_event = tsk_cond_create(false, false);
	wc->pthread_exit_flag = 0;	//exit thread

	//pthread_create(&wc->capture_thread, NULL, capture_thread, wc);

	return wc;
}

void *InitWindowMedia(obs_data_t *settings)
{
    return window_capture_create_internal(settings);
}

void DestroyWindowMedia(void *data)
{
	struct window_capture *wc = (struct window_capture *)data;

	tsk_cond_set(wc->capture_event);

	// wc->pthread_exit_flag = 1;
	
	pthread_join(wc->capture_thread, NULL);

	CGColorSpaceRelease(wc->color_space);

	//da_free(wc->buffer);
    if (wc->video_buf) {
        free(wc->video_buf);
    }
    
    wc->video_buf = NULL;
    wc->video_buf_size = 0;
    wc->frame_rate = 0;

	tsk_cond_destroy(&wc->capture_event);
	// os_event_destroy(wc->stop_event);

	destroy_window(&wc->window);

	free(wc);
    wc = NULL;
}

int StartWindowStream(void * data)
{
	if (!data) {
		return -1;
	}

	struct window_capture *wc = (struct window_capture *)data;

	pthread_create(&wc->capture_thread, NULL, capture_thread, wc);
    
    if(!g_borderManager )
    {
        g_borderManager = new BorderManager();
    }
    
    if( g_borderManager )
    {
        g_borderManager->startBorder( wc->window.window_id , wc->window.app_id);
    }

	return 0;
}

int StopWindowStream(void * data)
{
	if (!data) {
		return -1;
	}

	struct window_capture *wc = (struct window_capture *)data;

	wc->pthread_exit_flag = 1;
    
    if( g_borderManager )
     {
         g_borderManager->stopBorder();
     }

	return 0;
}

int SetWindowCaptureFramerate(void *data, int fps)
{
	// sanity check
	if (!data) {
		return -1;
	}

	if (fps < 1 || fps > 30) {
		fps = 15;	// default frame rate is 15
	}

	struct window_capture *wc = (struct window_capture *)data;
	wc->frame_rate = fps;
    
    return 0;
}

void SetWindowCaptureVideoCallback(void *data, VideoScreenCb cb)
{
	if (!data) {
		return;
	}

	struct window_capture *wc = (struct window_capture *)data;
	wc->videoCb = cb;
}

#include <cstring>
int getStringLength(NSString* strtemp)
{
    int strlength = 0;
    const char * pStr = strtemp.UTF8String;
    if(pStr==NULL)
    {
        return 0;
    }
    
    char *p = (char*)pStr;
    
    while(0x0 != *(p++))
    {
        strlength++;
    }
    
    return strlength;
}

void EnumerateWindowInfo(obs_data_t* info, int *count)
{
	int window_count = 0;
	NSArray *arr = enumerate_windows();
    for (NSDictionary *dict in arr) {
        NSString *owner = (NSString*)dict[OWNER_NAME];
        NSString *window  = (NSString*)dict[WINDOW_NAME];
        NSNumber *windowid    = (NSNumber*)dict[WINDOW_NUMBER];
        NSDictionary *windowBundes    = (NSDictionary*)dict[WINDOW_BOUNDES];
        if(window == nil || [@"" isEqualToString:window] || (windowBundes && ([(NSNumber*)windowBundes[@"Width"] intValue] < 64 || [(NSNumber*)windowBundes[@"Height"] intValue]< 64))){
            continue;
        }
        //NSNumber *wid   = (NSNumber*)dict[WINDOW_NUMBER];
        
//        printf("EnumerateWindowInfo: %s\n", [NSString stringWithFormat:@"[%@] [%@] [%@]", owner, window, windowid].UTF8String);
        
        memcpy(info->owner_name, owner.UTF8String, getStringLength(owner));
        memcpy(info->window_name, window.UTF8String, getStringLength(window));
        info->window_id = windowid.intValue;
        info++;
    	
    	window_count++;
    }

    *count = window_count;
    // printf("EnumerateWindowInfo: window count:%d\n", window_count);
}

#if 0 // todo by mark
void GetWindowDefaultSetting(obs_data_t *settings)
{
	// obs_data_set_default_bool(settings, "show_shadow", false);
	window_defaults(settings);
}

static obs_properties_t *window_capture_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	add_window_properties(props);

	obs_properties_add_bool(props, "show_shadow",
			obs_module_text("WindowCapture.ShowShadow"));

	return props;
}

static inline void window_capture_update_internal(struct window_capture *wc,
		obs_data_t *settings)
{
	wc->image_option = obs_data_get_bool(settings, "show_shadow") ?
		kCGWindowImageDefault : kCGWindowImageBoundsIgnoreFraming;

	update_window(&wc->window, settings);

	if (wc->window.window_name.length) {
		blog(LOG_INFO, "[window-capture: '%s'] update settings:\n"
				"\twindow: %s\n"
				"\towner:  %s",
				obs_source_get_name(wc->source),
				[wc->window.window_name UTF8String],
				[wc->window.owner_name UTF8String]);
	}
}


static void window_capture_update(void *data, obs_data_t *settings)
{
	@autoreleasepool {
		window_capture_update_internal(data, settings);
	}
}
#endif
#if 0
static const char *window_capture_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WindowCapture");
}

static inline void window_capture_tick_internal(struct window_capture *wc,
		float seconds)
{
	UNUSED_PARAMETER(seconds);
	os_event_signal(wc->capture_event);
}

static void window_capture_tick(void *data, float seconds)
{
	struct window_capture *wc = data;

	if (!obs_source_showing(wc->source))
		return;

	@autoreleasepool {
		return window_capture_tick_internal(data, seconds);
	}
}

struct obs_source_info window_capture_info = {
	.id             = "window_capture",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.get_name       = window_capture_getname,

	.create         = window_capture_create,
	.destroy        = window_capture_destroy,

	.output_flags   = OBS_SOURCE_ASYNC_VIDEO,
	.video_tick     = window_capture_tick,

	.get_defaults   = window_capture_defaults,
	.get_properties = window_capture_properties,
	.update         = window_capture_update,
};

#endif

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, CGWindowID* out);

static void OnEventReceived(
    AXObserverRef observer_ref,
    AXUIElementRef element,
    CFStringRef notification,
                            void *refcon);

//共享窗口的绿色边框宽度
#define BORDER_WIDTH 4


void OnEventReceived(
AXObserverRef observer_ref,
AXUIElementRef element,
CFStringRef notification,
                        void *refcon)
{
//    NSLog(@"Pinky:receiveEvent:%@", notification );
    
    NSString* note = (__bridge NSString *)notification;
    
    BorderManager* this_ptr = static_cast<BorderManager*>( refcon );
    if(!this_ptr)
    {
        return ;
    }
    
    if([note isEqualToString: NSAccessibilityWindowMiniaturizedNotification])
    {
        [this_ptr->m_bord miniaturize: nil] ;
        this_ptr->hideBorder( false );
    }
    else if( [note isEqualToString: NSAccessibilityApplicationHiddenNotification] )
    {
        this_ptr->hideBorder( false );
    }
    else if([note isEqualToString: NSAccessibilityWindowMovedNotification] ||
    [note isEqualToString: NSAccessibilityWindowResizedNotification])
    {
        this_ptr->hideBorder( true );

    }
    else if([note isEqualToString:NSAccessibilityApplicationActivatedNotification ] ||
            [note isEqualToString:NSAccessibilityFocusedWindowChangedNotification ]
            ){
        this_ptr->showBorder( false );
    }
    else if( [note isEqualToString:NSAccessibilityApplicationDeactivatedNotification ] )
    {
        if( this_ptr->IsWindowMinid( this_ptr->m_windowRef ))
        {
            this_ptr->hideBorder( false );
        }
        else{
            this_ptr->showBorder( false );
        }
    }
    else if ([note isEqualToString: NSAccessibilityWindowDeminiaturizedNotification ] )
    {
        [this_ptr->m_bord deminiaturize: nil] ;
        this_ptr->showBorder( true  );
    }
    else if( [note isEqualToString:NSAccessibilityApplicationShownNotification ] )
    {
        this_ptr->showBorder( true  );
    }
    else if( [note isEqualToString:NSAccessibilityUIElementDestroyedNotification  ])
    {
        //窗口都删除了，就停止了
        this_ptr->stopBorder();
    }
    else if( [note isEqualToString: NSAccessibilityWindowCreatedNotification ])
    {
        this_ptr->showBorder( true );
    }
}

void BorderManager::startBorder( CGWindowID windowID , int ownerID )
{
    if( m_windowId != 0 )
    {
        stopBorder();
    }
    
    m_appRef = AXUIElementCreateApplication(ownerID);
    
    CFArrayRef windowList;
    AXError err = AXUIElementCopyAttributeValue(m_appRef, kAXWindowsAttribute, (CFTypeRef *)&windowList);
    NSArray *array = (__bridge NSArray *)windowList;
    if ((!windowList) || CFArrayGetCount(windowList)<1)
        return;
    
    AXUIElementRef windowRef = NULL;
    CGWindowID widfind = 0;
    for(  int i = 0 ; i < array.count; i++ )
    {
        windowRef = (AXUIElementRef) CFArrayGetValueAtIndex( windowList, i);
        
        CGWindowID wid = 0;
        _AXUIElementGetWindow( windowRef, &wid );
        if( wid == windowID )
        {
            widfind = wid;
            m_windowRef = windowRef;
            m_windowId = widfind;
            break;
        }
    }
    
    if( widfind <= 0 )
    {
        NSLog(@"Pinky：didnot find the window:%d", windowID );
        return;
    }
    
    
    
    m_bord =  [[BordWindow alloc ] initWithContentRect:NSMakeRect(0,0,100,100) styleMask:(NSUInteger)NSResizableWindowMask backing:(NSBackingStoreType)NSBackingStoreRetained defer:(BOOL)true];
    m_bordWindowC = [[YMWindowController alloc] initWithWindow: m_bord];
    [m_bord setBorderSize: BORDER_WIDTH];
    [m_bord setCollectionBehavior: NSWindowCollectionBehaviorDefault |NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorTransient |NSWindowCollectionBehaviorFullScreenDisallowsTiling];
    
    if( g_context)
    {
        [m_bordWindowC showWindow:(__bridge NSWindowController*)g_context];
    }
    
    //注册监听事件
    err = AXObserverCreate( ownerID, OnEventReceived, &m_observer_ref );
    CFRunLoopAddSource(
           [[NSRunLoop currentRunLoop] getCFRunLoop],
           AXObserverGetRunLoopSource(m_observer_ref),
           kCFRunLoopDefaultMode);
    
    AddNotification( NSAccessibilityMainWindowChangedNotification, m_appRef );
    AddNotification( NSAccessibilityFocusedWindowChangedNotification, m_appRef );
    AddNotification( NSAccessibilityApplicationActivatedNotification, m_appRef );
    AddNotification( NSAccessibilityApplicationDeactivatedNotification, m_appRef );
    AddNotification( NSAccessibilityApplicationHiddenNotification, m_appRef );
    AddNotification( NSAccessibilityApplicationShownNotification, m_appRef );
    
    AddNotification( NSAccessibilityWindowMovedNotification, m_windowRef );
    AddNotification( NSAccessibilityWindowResizedNotification, m_windowRef );
    AddNotification( NSAccessibilityWindowMiniaturizedNotification, m_windowRef );
    AddNotification( NSAccessibilityWindowDeminiaturizedNotification, m_windowRef );
    AddNotification( NSAccessibilityWindowCreatedNotification, m_windowRef );
    
    AddNotification( NSAccessibilityUIElementDestroyedNotification, m_windowRef );
    AddNotification( NSAccessibilityCreatedNotification, m_windowRef );
    
    
    
    
    putWindowFront();
    showBorder( true );
}

bool BorderManager::IsWindowMinid( AXUIElementRef windowRef  )
{
    CFBooleanRef isMini;
    AXError err = AXUIElementCopyAttributeValue(m_windowRef, kAXMinimizedAttribute, (CFTypeRef *)&isMini);
    
    if( isMini && CFBooleanGetValue(isMini) )
    {
        return true;
    }
    else{
        return false;
    }
}

bool BorderManager::IsWindowOnScreen()
{
    CGWindowID ids[1];
    ids[0] = m_windowId;
    
    CFArrayRef window_id_array =
        CFArrayCreate(nullptr, reinterpret_cast<const void**>(&ids), 1, nullptr);
    CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
    
    CFBooleanRef isOnScreen = nil;
    
    if (CFArrayGetCount(window_array) > 0) {
        CFDictionaryRef window =
          reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, 0));
        isOnScreen = reinterpret_cast<CFBooleanRef>(CFDictionaryGetValue(window, kCGWindowIsOnscreen ));
    }
    
    CFRelease( window_id_array );
    CFRelease( window_array );
    return isOnScreen;
}

void BorderManager::AddNotification(NSString* notification,  AXUIElementRef element )
{
    AXError err = AXObserverAddNotification( m_observer_ref, element, static_cast<CFStringRef>(notification),  this);
    if( err != 0 )
    {
        NSLog(@"AddNotification Err:%@", notification );
    }
}

void BorderManager::RemoveNotification(NSString* notification,  AXUIElementRef element)
{
    AXError err = AXObserverRemoveNotification( m_observer_ref, element, static_cast<CFStringRef>(notification));
    if( err != 0 )
    {
        NSLog(@"RemoveNotification Err:%@", notification );
    }
}

void BorderManager::stopBorder()
{
    if( m_windowId == 0 )
    {
        return ;
    }
    hideBorder( false );
    
    RemoveNotification( NSAccessibilityMainWindowChangedNotification, m_appRef );
    RemoveNotification( NSAccessibilityFocusedWindowChangedNotification, m_appRef );
    RemoveNotification( NSAccessibilityApplicationActivatedNotification, m_appRef );
    RemoveNotification( NSAccessibilityApplicationDeactivatedNotification, m_appRef );
    RemoveNotification( NSAccessibilityApplicationHiddenNotification, m_appRef );
    RemoveNotification( NSAccessibilityApplicationShownNotification, m_appRef );
    
    RemoveNotification( NSAccessibilityWindowMovedNotification, m_windowRef );
    RemoveNotification( NSAccessibilityWindowResizedNotification, m_windowRef );
    RemoveNotification( NSAccessibilityWindowMiniaturizedNotification, m_windowRef );
    RemoveNotification( NSAccessibilityWindowDeminiaturizedNotification, m_windowRef );
    
    RemoveNotification( NSAccessibilityUIElementDestroyedNotification, m_windowRef );
    RemoveNotification( NSAccessibilityCreatedNotification, m_windowRef );
    
    m_windowId = 0;
    
    m_bord = nil;
    m_bordWindowC = nil;
}

int BorderManager::getScreenHeight()
{
//    NSArray *screenArray = [NSScreen screens];
//    NSScreen *mainScreen = [NSScreen mainScreen];
//    unsigned screenCount = [screenArray count];
//    unsigned index  = 0;
//
//    int screenHeight = 0;
//    for (index; index < screenCount; index++)
//    {
//      NSScreen *screen = [screenArray objectAtIndex: index];
//      NSRect screenRect = [screen frame];
//      NSString *mString = ((mainScreen == screen) ? @"Main" : @"not-main");
//
//      NSLog(@"Screen #%d (%@) Frame: %@", index, mString, NSStringFromRect(screenRect));
//        screenHeight = screenRect.size.height + screenRect.origin.y;
//    }
    
    int screenHeight = [[m_bord screen] frame].size.height;
    
    return screenHeight;
}
#define EXTEND_SIZE  0
NSRect BorderManager::getBordWindowFrame( CGPoint pointOri, CGSize sizeOri )
{
    int border = BORDER_WIDTH;
    int screenHight = getScreenHeight() ;
    int x = pointOri.x - border - EXTEND_SIZE;
//    int y = screenHight - pointOri.y - sizeOri.height - border + 23  - EXTEND_SIZE ;//23是个魔数，测试得出来的，UIScreen获取的分辨率比系统显示的少23
    int y = screenHight - pointOri.y - sizeOri.height - border - EXTEND_SIZE;
    int width = sizeOri.width + 2 * border  + 2 * EXTEND_SIZE;
    int height = sizeOri.height + 2 * border + 2* EXTEND_SIZE ;
    
    
    NSRect rect = NSMakeRect( x, y, width, height );
    return rect;
}

void BorderManager::updateBorder()
{
    if( !IsWindowOnScreen() )
    {
        hideBorder( false );
    }
    else{
        if( !m_timer )  //没有timer，说明没处在临时隐藏状态
        {
            showBorder( true );
        }
        else{
            showBorder( false );
        }
    }
}

void BorderManager::showBorder( bool bForceShow )
{
//    NSLog(@"Pinky:showBorder:%d", bForceShow);
    if( m_bord == NULL )
    {
        return ;
    }
    AXValueRef position;
    CGPoint point;
    AXUIElementCopyAttributeValue(m_windowRef, kAXPositionAttribute, (CFTypeRef *)&position);
    AXValueGetValue(position, (AXValueType)kAXValueCGPointType, &point);
    
    AXValueRef sizeRef;
    CGSize size;
    AXUIElementCopyAttributeValue(m_windowRef, kAXSizeAttribute, (CFTypeRef *)&sizeRef);
    AXValueGetValue(sizeRef, (AXValueType)kAXValueCGSizeType, &size);
        
    NSRect rect  = getBordWindowFrame( point, size );
    
    bool bShow = [m_bord isVisible];
    
   
    [m_bord orderWindow:NSWindowAbove   relativeTo:(NSInteger)m_windowId];
    
    if( bForceShow )
    {
        bShow = true;
    }
    
    [m_bord setFrame: rect  display: bShow ];
    [m_bord setIsVisible: bShow];
    
//    NSLog(@"Pinky,window.frame,(%f, %f), size:(%f, %f )", m_bord.frame.origin.x, m_bord.frame.origin.y, m_bord.frame.size.width, m_bord.frame.size.height );
    
}

//bReShow是否过一会要自动显示，move,resize得不到结束的回调
void BorderManager::hideBorder( bool bReShow )
{
//    NSLog(@"Pinky:hideBorder:%d", bReShow);
    if( m_bord )
   {
       [m_bord setIsVisible: false ];
       
       if( m_timer )
       {
           [m_timer invalidate];
           m_timer = nil;
       }
       if( bReShow )
       {
           m_timer = [NSTimer timerWithTimeInterval:0.2 repeats:false block:^(NSTimer * _Nonnull timer) {
               this->showBorder( true );
               m_timer = nil;
           }];
           
            [[NSRunLoop currentRunLoop] addTimer:m_timer forMode:NSDefaultRunLoopMode];
       }
   }
}

void BorderManager::putWindowFront()
{
    AXError err;
    err = AXUIElementSetAttributeValue(m_windowRef, kAXMainAttribute, kCFBooleanTrue);
    err = AXUIElementSetAttributeValue(m_windowRef, kAXFocusedApplicationAttribute, kCFBooleanTrue);
    err = AXUIElementSetAttributeValue(m_windowRef, kAXFocusedAttribute, kCFBooleanTrue);
    err = AXUIElementSetAttributeValue(m_appRef, kAXFocusedWindowAttribute, kCFBooleanTrue);
    err = AXUIElementSetAttributeValue(m_appRef, kAXFrontmostAttribute, kCFBooleanTrue);
    err = AXUIElementSetAttributeValue(m_windowRef, kAXFocusedAttribute, kCFBooleanTrue );
    err = AXUIElementSetAttributeValue(m_windowRef,  kAXMinimizedAttribute,kCFBooleanFalse);
}





