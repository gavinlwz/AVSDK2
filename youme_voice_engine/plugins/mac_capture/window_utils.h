#import <CoreGraphics/CGWindow.h>
#import <Cocoa/Cocoa.h>

#import "window_capture.h"
//#include <util/threading.h>
//#include <obs-module.h>

#define WINDOW_NAME   ((__bridge NSString*)kCGWindowName)
#define WINDOW_NUMBER ((__bridge NSString*)kCGWindowNumber)
#define WINDOW_BOUNDES ((__bridge NSString*)kCGWindowBounds)
#define OWNER_NAME    ((__bridge NSString*)kCGWindowOwnerName)
#define OWNER_PID     ((__bridge NSNumber*)kCGWindowOwnerPID)

typedef struct obs_source_s {
	//todo

}obs_source_t;

struct cocoa_window {
	CGWindowID      window_id;
    int app_id;

	pthread_mutex_t name_lock;
	NSString        *owner_name;
	NSString        *window_name;

	uint64_t next_search_time;
    
    cocoa_window(){
        window_id = 0;
        app_id = 0;
        owner_name = @"";
        window_name = @"";
        next_search_time = 0;
    }
};
typedef struct cocoa_window *cocoa_window_t;

NSArray *enumerate_windows(void);

uint64_t os_gettime_ns(void);

bool find_window(cocoa_window_t cw, obs_data_t *settings, bool force);

void init_window(cocoa_window_t cw, obs_data_t *settings);

void destroy_window(cocoa_window_t cw);

void update_window(cocoa_window_t cw, obs_data_t *settings);

void window_defaults(obs_data_t *settings);

//void add_window_properties(obs_properties_t *props);
//
//void show_window_properties(obs_properties_t *props, bool show);
