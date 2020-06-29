//
//  screen_capture.cpp
//  youme_voice_engine
//
//  Created by pinky on 2020/5/25.
//  Copyright © 2020 Youme. All rights reserved.
//

#include "tsk_time.h"
#include "screen_capture.hpp"

#include <CoreGraphics/CGDisplayStream.h>
#include <Cocoa/Cocoa.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreImage/CoreImage.h>
#include <sstream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include "YouMeCommon/XCondWait.h"

// The amount of pixels to add to the actual window bounds to take into
// account of the border/shadow effects.
static const int kBorderEffectSize = 50;
static const int kBorderEffectSizeMove = 150;   //移动的时候，因为和全屏画面的获取有时间差，所以容易超出边框，得给一个大边框

//mac电脑配置管理
class MacDisplayConfig
{
public:
    CGDirectDisplayID m_id;
    // Bounds of the desktop excluding monitors with DPI settings different from
    // the main monitor. In Density-Independent Pixels (DIPs).
    CGRect bounds;
    // Same as bounds, but expressed in physical pixels.
    CGRect pixel_bounds;
    // Scale factor from DIPs to physical pixels.
    float dip_to_pixel_scale = 1.0f;
};

class MacDesktopConfig
{
public:
    static MacDesktopConfig* GetCurrent();
    // Bounds of the desktop excluding monitors with DPI settings different from
    // the main monitor. In Density-Independent Pixels (DIPs).
    CGRect bounds;
    // Same as bounds, but expressed in physical pixels.
    CGRect pixel_bounds;
    // Scale factor from DIPs to physical pixels.
    float dip_to_pixel_scale = 1.0f;
    
    std::vector<MacDisplayConfig> displays;
};

MacDisplayConfig GetConfigurationForScreen(NSScreen* screen) {
    MacDisplayConfig display_config;

    // Fetch the NSScreenNumber, which is also the CGDirectDisplayID.
    NSDictionary* device_description = [screen deviceDescription];
    display_config.m_id = static_cast<CGDirectDisplayID>(
          [[device_description objectForKey:@"NSScreenNumber"] intValue]);

    // Determine the display's logical & physical dimensions.
    NSRect ns_bounds = [screen frame];
    display_config.bounds = CGRectMake( ns_bounds.origin.x, ns_bounds.origin.y, ns_bounds.size.width, ns_bounds.size.height );

    // If the host is running Mac OS X 10.7+ or later, query the scaling factor
    // between logical and physical (aka "backing") pixels, otherwise assume 1:1.
    if ([screen respondsToSelector:@selector(backingScaleFactor)] &&
            [screen respondsToSelector:@selector(convertRectToBacking:)]) {
        display_config.dip_to_pixel_scale = [screen backingScaleFactor];
        NSRect ns_pixel_bounds = [screen convertRectToBacking: ns_bounds];
        display_config.pixel_bounds = CGRectMake( ns_pixel_bounds.origin.x, ns_pixel_bounds.origin.y, ns_pixel_bounds.size.width, ns_pixel_bounds.size.height );;
    } else {
        display_config.pixel_bounds = display_config.bounds;
    }

  // Determine if the display is built-in or external.
//  display_config.is_builtin = CGDisplayIsBuiltin(display_config.id);

  return display_config;
}


void InvertRectYOrigin(const CGRect& bounds,
                       CGRect* rect) {
    *rect = CGRectMake( rect->origin.x, ( (bounds.origin.y + bounds.size.height) - (rect->origin.y + rect->size.height) ) ,  rect->size.width, rect->size.height);
}

//为了获取dip_to_pixel_scale
MacDesktopConfig* MacDesktopConfig::GetCurrent()
{
    MacDesktopConfig* desktop_config = new MacDesktopConfig();
    NSArray* screens = [NSScreen screens];
    assert(screens);
    
    // Iterator over the monitors, adding the primary monitor and monitors whose
    // DPI match that of the primary monitor.
    for (NSUInteger i = 0; i < [screens count]; ++i) {
        MacDisplayConfig display_config = GetConfigurationForScreen([screens objectAtIndex: i]);

        if (i == 0)
            desktop_config->dip_to_pixel_scale = display_config.dip_to_pixel_scale;

          // Cocoa uses bottom-up coordinates, so if the caller wants top-down then
          // we need to invert the positions of secondary monitors relative to the
          // primary one (the primary monitor's position is (0,0) in both systems).
//          if (i > 0 && origin == TopLeftOrigin) {
        if( i> 0 )
        {
            InvertRectYOrigin(desktop_config->displays[0].bounds,
                              &display_config.bounds);
            // |display_bounds| is density dependent, so we need to convert the
            // primay monitor's position into the secondary monitor's density context.
            float scaling_factor = display_config.dip_to_pixel_scale /
                desktop_config->displays[0].dip_to_pixel_scale;
        CGRect primary_bounds = CGRectMake(desktop_config->displays[0].pixel_bounds.origin.x* scaling_factor,
                                               desktop_config->displays[0].pixel_bounds.origin.y * scaling_factor,
                                           desktop_config->displays[0].pixel_bounds.size.width * scaling_factor ,
                                           desktop_config->displays[0].pixel_bounds.size.height * scaling_factor );
            InvertRectYOrigin(primary_bounds, &display_config.pixel_bounds);
        }
        
    // Add the display to the configuration.
       desktop_config->displays.push_back(display_config);

       // Update the desktop bounds to account for this display, unless the current
       // display uses different DPI settings.
//           if (display_config.dip_to_pixel_scale == desktop_config.dip_to_pixel_scale) {
//             desktop_config.bounds.UnionWith(display_config.bounds);
//             desktop_config.pixel_bounds.UnionWith(display_config.pixel_bounds);
//           }
     }

     return desktop_config;
}

struct ExclusiveWndInfo
{
    CGWindowID wid;
    CGRect lastRect;    //记录上此的大ExclusiveWndInfo小位置，以确定扩展的大小
    CGRect relativeRect;    //相对于屏幕的大小位置
    CGImageRef img;       //获取到的排除后的画面
    
    ExclusiveWndInfo( int windowid )
    {
        wid = windowid;
        lastRect = CGRectMake(0, 0, 0, 0);
        relativeRect = CGRectMake(0, 0, 0, 0);
        img = nil;
    }
};


CIContext* context = nil;
//typedef struct {
class AVFContext{
public:
    int             frame_rate;
    int             width;
    int             height;
    ScreenCb        videoCb;
    int             frames_captured;
    
    MacDesktopConfig* deskConfig;
    int displayIndex = 0;
    CGDirectDisplayID displayID = 0;
    
//    CGWindowID exclusiveWnd = 0;
//        CGRect exclusiveWndLastRect ={0};
//    std::vector<CGWindowID> vecExclusiveWnd;
    std::map<CGWindowID, ExclusiveWndInfo*> mapExclusiveWndInfo;
    bool refreshedWndList = false ;

    
    CGDisplayStreamRef stream_ref = nil;
    
    dispatch_queue_t dq = nil ;
    
    bool m_bStarted = false ;
//    CIContext* context = nil;
    
    CVPixelBufferRef last_frame = nil ;    //自适应帧率，可能很久不来一帧，把原来的一帧保存下来，超时还没来，就拿这个发一次
    uint64_t last_ts = 0;
    
    int frame_rate_min = 5;             //不能小于5帧，不然太少了
    std::thread  check_ts_thread;       //最小帧率检查，帧数太小不行啊
    youmecommon::CXCondWait m_frameCond;
    
    std::mutex m_dateMutex;
    
    NSData* data = nil ;
    
    CGColorSpaceRef colorSpace = nil;
    CFDictionaryRef pixBuffOptionsDictionary = nil;
    
    
};
//} AVFContext;
void clearExclusiveWnd( AVFContext *ctx );
void InitContext( AVFContext* ctx)
{
    ctx->frame_rate = 15;
    ctx->width = 1920;
    ctx->height = 1080;
    
    ctx->videoCb = nullptr;
    ctx->frames_captured = 0;
    
    ctx->deskConfig = nullptr;
    ctx->displayIndex  = 0;
    ctx->displayID = 0;
    ctx->stream_ref = nil;
    ctx->dq = nil;
    ctx->m_bStarted = false;
    ctx->last_frame = nil;
    ctx->frame_rate_min = 5;
    context = nil;  //直接作为成员不行
    ctx->colorSpace = nil;
    ctx->pixBuffOptionsDictionary = nil;
    
    clearExclusiveWnd( ctx );
    ctx->refreshedWndList = false;

}

void clearContext( AVFContext* ctx )
{
    if( ctx->deskConfig )
    {
        delete ctx->deskConfig;
        ctx->deskConfig = nullptr;
    }
    
    if( ctx->stream_ref )
    {
        CFRelease( ctx->stream_ref );
        ctx->stream_ref  = nil;
    }
    
    if( ctx->last_frame )
    {
        CFRelease( ctx->last_frame );
        ctx->last_frame = nil;
    }
    
    if( ctx->colorSpace )
    {
        CFRelease( ctx->colorSpace );
        ctx->colorSpace = nil;
    }
    
    if( ctx->pixBuffOptionsDictionary )
    {
        CFRelease( ctx->pixBuffOptionsDictionary );
        ctx->pixBuffOptionsDictionary = nil;
    }
    
    context = nil;
    ctx->dq = nil;
    ctx->videoCb = nil;
    ctx->m_frameCond.Reset();

}


void onFrame( void* data , IOSurfaceRef frame,CGDisplayStreamUpdateRef ref );
int  configure( void *data )
{
    if( !data )
    {
        return -1;
    }
    AVFContext *ctx = (AVFContext *)data;
    if( ctx->dq == nil )
    {
        ctx->dq = dispatch_queue_create("screen capture", DISPATCH_QUEUE_SERIAL);
    }
    
    if( context == nil )
    {
        NSMutableDictionary *option = [[NSMutableDictionary alloc] init];
        [option setObject: [NSNull null] forKey: kCIContextWorkingColorSpace];
        [option setObject: @"NO" forKey: kCIContextUseSoftwareRenderer];
        context = [CIContext contextWithOptions:nil];
    }
    
    ctx->colorSpace = CGColorSpaceCreateDeviceRGB();
    const void *keys[] = {
       kCVPixelBufferMetalCompatibilityKey,
       kCVPixelBufferIOSurfacePropertiesKey,
       kCVPixelBufferBytesPerRowAlignmentKey,
    };
    const void *values[] = {
        (__bridge const void *)([NSNumber numberWithBool:YES]),
        (__bridge const void *)(@{}),
        (__bridge const void *)(@(32)),
    };
    // 创建pixelbuffer属性
    ctx->pixBuffOptionsDictionary = CFDictionaryCreate(NULL, keys, values, 3, NULL, NULL);
    
    uint32_t pixel_format = 'BGRA';
       
    int outWidth = ctx->deskConfig->displays[ ctx->displayIndex ].bounds.size.width;
    int outHeight = ctx->deskConfig->displays[ ctx->displayIndex ].bounds.size.height;
    ctx->stream_ref = CGDisplayStreamCreateWithDispatchQueue( ctx->displayID,
                                                    outWidth,
                                                    outHeight,
                                                    pixel_format,
                                                    (__bridge CFDictionaryRef)(@{(__bridge NSString *)kCGDisplayStreamShowCursor : @YES,
                                                    (__bridge NSString*)kCGDisplayStreamMinimumFrameTime: @(1.0f/ctx->frame_rate)}),
                                                    ctx->dq,
                                                    ^(CGDisplayStreamFrameStatus status,    /* kCGDisplayStreamFrameComplete, *FrameIdle, *FrameBlank, *Stopped */
                                                    uint64_t time,                        /* Mach absolute time when the event occurred. */
                                                    IOSurfaceRef frame,                   /* opaque pixel buffer, can be backed by GL, CL, etc.. This may be NULL in some cases. See the docs if you want to keep access to this. */
                                                    CGDisplayStreamUpdateRef ref)
                                                    {
        if( kCGDisplayStreamFrameStatusFrameComplete == status
        && NULL != frame )
        {
            std::lock_guard<std::mutex> lock( ctx->m_dateMutex );
            if( ctx && ctx->m_bStarted != false )
            {
                onFrame(  data , frame, ref );
            }
        }
    }
    );
        
    if( NULL == ctx->stream_ref )
    {
        printf("Error: create failed\n");
        return -4;
    }
        
    return 0;
}

void *InitScreenMedia( int screenid  )
{
    AVFContext *ctx               = nil;
    ctx = new AVFContext();
    InitContext( ctx );
    
    ctx->deskConfig = MacDesktopConfig::GetCurrent();
    ctx->displayIndex = screenid;
    if( screenid >= ctx->deskConfig->displays.size() )
    {
        ctx->displayIndex = 0;
    }
    
    ctx->displayID = ctx->deskConfig->displays[ ctx->displayIndex ].m_id;
    
    return ctx;
}

void DestroyScreenMedia(void *data)
{
    AVFContext *ctx = (AVFContext *)data;

    ctx->videoCb         = NULL;

    if (ctx) {
        delete ctx;
        ctx = NULL;
    }
}

void clearExclusiveWnd( AVFContext *ctx )
{
    if( !ctx )
        return;
    
    for( auto iter = ctx->mapExclusiveWndInfo.begin(); iter != ctx->mapExclusiveWndInfo.end(); ++iter )
    {
        delete iter->second;
        iter->second = nil;
    }
    
    ctx->mapExclusiveWndInfo.clear();
}

int SetScreenExclusiveWnd( void * data, int wnd )
{
    if (!data) {
        return -1;
    }
    AVFContext *ctx = (AVFContext *)data;
 
    std::lock_guard<std::mutex> lock( ctx->m_dateMutex );
    if( wnd == 0 )
    {
        clearExclusiveWnd( ctx );
        ctx->refreshedWndList = true;
    }
    else{
        auto iter = ctx->mapExclusiveWndInfo.find( wnd );
        if( iter != ctx->mapExclusiveWndInfo.end() )
        {
            return 0;
        }
        else{
            ctx->mapExclusiveWndInfo[ wnd ] = new ExclusiveWndInfo( wnd );
            ctx->refreshedWndList = true;
        }
    }
    
    return 0;
}

int SetScreenCaptureFramerate(void *data, int fps)
{
    if (!data) {
        return -1;
    }

    // fps: up to 60
    // default frame rate is 15
    if (fps < 1 || fps > 60) {
        fps = 15;
    }

    AVFContext *ctx = (AVFContext *)data;
    ctx->frame_rate = fps;

    return 0;
}

void SetScreenCaptureVideoCallback(void *data, ScreenCb cb)
{
    if (!data) {
        return;
    }

    AVFContext *ctx = (AVFContext *)data;
    ctx->videoCb = cb;
}

void FrameCbThread( AVFContext* ctx )
{
    int max_frame_wait = 1000 /ctx->frame_rate_min;
    int waitTime = max_frame_wait;
    while(1)
    {
        youmecommon::WaitResult ret = ctx->m_frameCond.WaitTime( waitTime );
        {
            if( ctx->m_bStarted == false )
            {
                break;
            }
            
            std::lock_guard<std::mutex> lock( ctx->m_dateMutex );
            if( ctx->videoCb )
            {
                CVPixelBufferLockBaseAddress(ctx->last_frame, kCVPixelBufferLock_ReadOnly);
                    
                uint64_t ts = tsk_gettimeofday_ms();
                int width = (int)CVPixelBufferGetWidth( ctx->last_frame );
                int height = (int)CVPixelBufferGetHeight( ctx->last_frame );

                ctx->videoCb( (char*)ctx->last_frame, 0, width, height, ts );
                CVPixelBufferUnlockBaseAddress(ctx->last_frame, kCVPixelBufferLock_ReadOnly);
            }
        }
    };
}
int StartScreenStream(void * data)
{
    AVFContext *ctx = (AVFContext *)data;
    if( !ctx )
    {
        return -1;
    }
    
    std::lock_guard<std::mutex> lock( ctx->m_dateMutex );
    if( ctx->m_bStarted )
    {
        StopScreenStream( data );
    }
    
    int ret = configure( ctx );
    if( ret != 0 )
    {
        return ret;
    }
    
    CGError err = CGDisplayStreamStart( ctx->stream_ref );
    
    if( kCGErrorSuccess != err )
    {
        printf("Error: failed to start\n");
        return -1;
    }
    
    ctx->m_bStarted = true;
    ctx->check_ts_thread = std::thread( FrameCbThread, ctx);
    
    return 0;
}

int StopScreenStream(void * data)
{
    AVFContext *ctx = (AVFContext *)data;
    if( !ctx )
    {
        return -1;
    }
    
    ctx->m_bStarted = false;
    
    if (ctx->check_ts_thread.joinable()) {
       ctx->m_frameCond.SetSignal();
       if (ctx->check_ts_thread.get_id() == std::this_thread::get_id()) {
           ctx->check_ts_thread.detach();
       }
       else {
           ctx->check_ts_thread.join();
       }
    }
    
    std::lock_guard<std::mutex> lock( ctx->m_dateMutex );
    CGError err = CGDisplayStreamStop( ctx->stream_ref);
    
    if (kCGErrorSuccess != err) {
      printf("Error: failed to stop the display stream capturer. CGDisplayStreamStart failed: %d .\n", err);
    }
    
    clearContext( ctx );

    return 0;
}

int GetWindowId(CFDictionaryRef window) {
  CFNumberRef window_id = reinterpret_cast<CFNumberRef>(
      CFDictionaryGetValue(window, kCGWindowNumber));
  if (!window_id) {
    return 0;
  }

  // Note: WindowId is 64-bit on 64-bit system, but CGWindowID is always 32-bit.
  // CFNumberGetValue() fills only top 32 bits, so we should use CGWindowID to
  // receive the window id.
  CGWindowID wid;
  if (!CFNumberGetValue(window_id, kCFNumberIntType, &wid)) {
    return 0;
  }

  return wid;
}

CFArrayRef CreateWindowListWithExclusion( const std::map<CGWindowID, ExclusiveWndInfo*>& mapExclusiveList )
{
    if( mapExclusiveList.empty() )
    {
        return nullptr;
    }

    CFArrayRef all_windows =
    CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID);
    if( !all_windows ) return nullptr;

    int count =  CFArrayGetCount( all_windows);
    CFMutableArrayRef returned_array = CFArrayCreateMutable( nullptr,  count , nullptr);

    bool bfound = false;
    for( CFIndex i = 0 ; i < count ; ++i )
    {
        CFDictionaryRef window = reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(all_windows, i));
        CGWindowID wid = GetWindowId(window);
        
        if( mapExclusiveList.find( wid ) != mapExclusiveList.end() )
        {
            bfound = true;
            continue;
        }

        CFArrayAppendValue(returned_array, reinterpret_cast<void*>(wid));
    }

    CFRelease( all_windows );
    //没找到，那么就不用找他的画面了，所以返回空
    if(!bfound )
    {
        CFRelease( returned_array );
        return nullptr;
    }
    return returned_array;
}

// Returns the bounds of |window| in physical pixels, enlarged by a small amount
// on four edges to take account of the border/shadow effects.
CGRect GetExcludedWindowPixelBounds( CGWindowID window,  float dip_to_pixel_scale )
{
    CGRect rect;
    CGWindowID ids[1];
    ids[0] = window;
    
    CFArrayRef window_id_array =
        CFArrayCreate(nullptr, reinterpret_cast<const void**>(&ids), 1, nullptr);
    CFArrayRef window_array = CGWindowListCreateDescriptionFromArray(window_id_array);
    
    CFBooleanRef isOnScreen = nil;
    
    if (CFArrayGetCount(window_array) > 0) {
      CFDictionaryRef window =
          reinterpret_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(window_array, 0));
        
        
    isOnScreen = reinterpret_cast<CFBooleanRef>(CFDictionaryGetValue(window, kCGWindowIsOnscreen ));
        
    
      CFDictionaryRef bounds_ref =
          reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(window, kCGWindowBounds));
      CGRectMakeWithDictionaryRepresentation(bounds_ref, &rect);
    }
    CFRelease(window_id_array);
    CFRelease(window_array);
    
    if( isOnScreen && CFBooleanGetValue(isOnScreen) )
    {
        return rect;
    }
    else{
        return CGRectMake(0,0,0,0 );
    }
    
    
    // |rect| is in DIP, so convert to physical pixels.
//    return ScaleAndRoundCGRect(rect, dip_to_pixel_scale);
}

CGRect intersectRect( CGRect rect1, CGRect rect2 )
{
    CGFloat left, top, right , bottom;
    left = std::max( rect1.origin.x, rect2.origin.x);
    top = std::max( rect1.origin.y, rect2.origin.y );
    right = std::min( rect1.origin.x + rect1.size.width,  rect2.origin.x+ rect2.size.width );
    bottom = std::min( rect1.origin.y + rect1.size.height,  rect2.origin.y+ rect2.size.height);
    
    if( right <= left || bottom <= top )
    {
        return CGRectMake(0, 0, 0, 0);
    }
    
    return CGRectMake( left , top,  right - left ,  bottom - top );
}

// Copy pixels in the |rect| from |src_place| to |dest_plane|. |rect| should be
// relative to the origin of |src_plane| and |dest_plane|.
void CopyRect(const uint8_t* src_plane,
              int src_plane_stride,
              uint8_t* dest_plane,
              int dest_plane_stride,
              int bytes_per_pixel,
              const CGRect& rect) {
  // Get the address of the starting point.
    const int src_y_offset = src_plane_stride * rect.origin.y;
    const int dest_y_offset = dest_plane_stride * rect.origin.y;
    const int x_offset = bytes_per_pixel * rect.origin.x;
    src_plane += src_y_offset + x_offset;
    dest_plane += dest_y_offset + x_offset;

  // Copy pixels in the rectangle line by line.
    const int bytes_per_line = bytes_per_pixel * rect.size.width;
    const int height = rect.size.height;
    for (int i = 0; i < height; ++i) {
        memcpy(dest_plane, src_plane, bytes_per_line);
        src_plane += src_plane_stride;
        dest_plane += dest_plane_stride;
    }
}


CVPixelBufferRef pixelBufferFromNSData( AVFContext* ctx, NSData* data, int width, int height, size_t bytesPerRow )
{
    NSDictionary *options = [NSDictionary dictionaryWithObjectsAndKeys:
                                 [NSNumber numberWithBool:YES], kCVPixelBufferCGImageCompatibilityKey,
                                 [NSNumber numberWithBool:YES], kCVPixelBufferCGBitmapContextCompatibilityKey,
    //                             [NSNumber numberWithBool:YES], kCVPixelBufferIOSurfacePropertiesKey,
    //                             [NSNumber numberWithBool:YES], kCVPixelBufferMetalCompatibilityKey,
                                 nil];
    CVPixelBufferRef pxbuffer = NULL;
    CVPixelBufferCreateWithBytes(kCFAllocatorDefault,
            width,
            height,
            kCVPixelFormatType_32BGRA,
            (void *)[data bytes],
            bytesPerRow,
            NULL,
            NULL,
            (__bridge CFDictionaryRef)options,
            &pxbuffer);
    
    return pxbuffer;
}

CGImageRef CreateExcludedWindowRegionImage(const CGRect& pixel_bounds,
                                           float dip_to_pixel_scale,
                                           CFArrayRef window_list )
{
    CGImageRef img =  CGWindowListCreateImageFromArray(pixel_bounds, window_list, kCGWindowImageBoundsIgnoreFraming | kCGWindowImageNominalResolution );
    int imgWidth = CGImageGetWidth(img);
    int imgHeight = CGImageGetHeight(img);
    return img;
}

CVPixelBufferRef getResultPix( AVFContext *ctx , IOSurfaceRef frame,CGDisplayStreamUpdateRef ref)
{
    CGFloat width = 0;
    CGFloat height = 0 ;
        
    const uint8_t* display_base_address = 0;
    int src_bytes_per_row = 0;
    CGImageRef excluded_image = nil;
       
    CGRect displayRect;
    
    CGRect rectRelativeToDisplay;
    CVPixelBufferRef pix = nil;
    int bytePerPix = 4;

    CFArrayRef window_list = nil;
    
    int exclusiveImgCount = 0;
    if( !ctx->mapExclusiveWndInfo.empty() )
    {
        displayRect = ctx->deskConfig->displays[ ctx->displayIndex ].bounds;
        window_list = CreateWindowListWithExclusion( ctx->mapExclusiveWndInfo );
        if( window_list != nil )
        {
            CGRect wndRect;
            for( auto iter = ctx->mapExclusiveWndInfo.begin();
                iter != ctx->mapExclusiveWndInfo.end(); ++iter )
            {
                wndRect = GetExcludedWindowPixelBounds( iter->first,  1  );
                if( wndRect.size.width != 0 && wndRect.size.height != 0 )
                {
                    int border = kBorderEffectSize;
                    if( wndRect.origin.x != iter->second->lastRect.origin.x ||
                       wndRect.origin.y != iter->second->lastRect.origin.y ||
                       wndRect.size.width != iter->second->lastRect.size.width ||
                       wndRect.size.height != iter->second->lastRect.size.height )
                    {
                        border = kBorderEffectSizeMove;
                        iter->second->lastRect = wndRect;
                    }
                   
                    wndRect.origin.x -= border;
                    wndRect.origin.y -= border;
                    wndRect.size.width += 2 * border;
                    wndRect.size.height += 2 * border;
                   
                    //如果超出屏幕的部分，不要拷贝哟
                    wndRect = intersectRect( wndRect, displayRect );
                      
                    if( wndRect.size.width != 0 && wndRect.size.height != 0 )
                    {
                        iter->second->img = CreateExcludedWindowRegionImage( wndRect, 1, window_list );
                        //找出window相对于display的坐标
                        iter->second->relativeRect = wndRect;
                        iter->second->relativeRect.origin.x -= displayRect.origin.x;
                        iter->second->relativeRect.origin.y -= displayRect.origin.y;
                        
                        exclusiveImgCount++;
                    }
                    else{
                        iter->second->img = nil;
                    }
                }
                else{
                    iter->second->img = nil;
                    iter->second->lastRect = CGRectMake(0,0,0,0);
                }
            }
            
            CFRelease( window_list );
        }
    }
        
    IOSurfaceLock( frame, kIOSurfaceLockReadOnly, NULL );
    uint8* plane= (uint8_t*)IOSurfaceGetBaseAddress(frame);
    int stride = IOSurfaceGetBytesPerRow(frame);
    int size = IOSurfaceGetAllocSize( frame );
    width = IOSurfaceGetWidth( frame );
    height = IOSurfaceGetHeight( frame );
      
    
    int need_size = (int)(height * stride);
    if( ctx->data == nil || [ctx->data length] < need_size )
    {
        ctx->data = [[NSData alloc] initWithBytes:(const void*)plane length:need_size ];
        ctx->refreshedWndList = false;
    }
    else{
        //如果排除的窗口列表变动过，则可能需要重新加载全画面
        if( ctx->refreshedWndList )
        {
            memcpy( (uint8_t*)ctx->data.bytes, plane, need_size);
            ctx->refreshedWndList = false;
        }
        else{
            //仅更新变动部分
            size_t count = 0;
            const CGRect* rects =
                CGDisplayStreamUpdateGetRects(ref, kCGDisplayStreamUpdateDirtyRects, &count);

            const uint8_t* display_base_address = plane;
            int src_bytes_per_row = stride;

            for( int i = 0 ; i < count ; i++ )
            {
                CGRect rect = *(rects+i);
                CopyRect( display_base_address, src_bytes_per_row,
                         (uint8_t*)[ctx->data bytes],
                         stride,
                         bytePerPix,
                         rect );
            }
        }
    }

    IOSurfaceUnlock(frame, kIOSurfaceLockReadOnly, NULL);
    
    if( exclusiveImgCount != 0  )
    {
        for( auto iter = ctx->mapExclusiveWndInfo.begin();
            iter != ctx->mapExclusiveWndInfo.end(); ++iter )
        {
            if( iter->second->img == nil )
            {
                continue;
            }
            
            CGImageRef excluded_image = iter->second->img;
            iter->second->img = nil;
            
            CGDataProviderRef provider = CGImageGetDataProvider(excluded_image);
            CFDataRef excluded_image_data =  CGDataProviderCopyData(provider);
            display_base_address = CFDataGetBytePtr(excluded_image_data);
            src_bytes_per_row = CGImageGetBytesPerRow(excluded_image);

            //获取的图片可能小于坐标size
            int imgWidth = CGImageGetWidth(excluded_image);
            int imgHeight = CGImageGetHeight(excluded_image);
            
            CGRect rect_to_copy = CGRectMake(0,0, imgWidth, imgHeight );

            if( CGImageGetBitsPerPixel( excluded_image)/8 == 4 )
            {
                uint8_t* begin = (uint8_t*)[ctx->data bytes];
             
             
                uint8_t* destStartPos = begin +
                int( iter->second->relativeRect.origin.y * stride)+ int(iter->second->relativeRect.origin.x) * bytePerPix;
                //进行拷贝啦
                CopyRect( display_base_address,
                         src_bytes_per_row,
                         destStartPos,
                         stride,
                         bytePerPix,//DesktopFrame::kBytesPerPixel,
                         rect_to_copy
                         );
            }
            CFRelease( excluded_image );
            CFRelease( excluded_image_data );
        }
    }
    
    pix = pixelBufferFromNSData( ctx , ctx->data, width , height ,  stride );

    return pix;
}


void onFrame( void* data , IOSurfaceRef frame,CGDisplayStreamUpdateRef ref )
{
    if( data == nil )
    {
        return;
    }
    
    AVFContext *ctx = (AVFContext *)data;
   
    CGFloat width = 0;
    CGFloat height = 0 ;
    CGRect displayRect = ctx->deskConfig->displays[ ctx->displayIndex ].bounds;
    width = displayRect.size.width;
    height = displayRect.size.height;
    
//    int start = tsk_gettimeofday_ms();

    CVPixelBufferRef newPixcelBuffer  = getResultPix( ctx , frame, ref );
    
//    int end4 = tsk_gettimeofday_ms();
//
//    static int total_time = 0;
//    static int total_count = 0;
//    total_time += end4 -start;
//    total_count ++;
//
//    if( total_count % 100 == 0  )
//    {
//        printf("Pinky:avg cost:%d\n", total_time / total_count );
//        total_time = 0 ;
//        total_count = 0;
//    }
    
    {
        if( ctx->last_frame )
        {
            CFRelease( ctx->last_frame );
        }

        ctx->last_frame = newPixcelBuffer;
        ctx->m_frameCond.SetSignal();
    }
}


