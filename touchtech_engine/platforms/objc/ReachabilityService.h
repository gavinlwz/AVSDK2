


#import <Foundation/Foundation.h>
#import "INgnNetworkService.h"

@interface YouMeTalkReachabilityServiceIOS : NSObject

+ (YouMeTalkReachabilityServiceIOS *)getInstance;
+ (void)destroy;

- (id)init;
- (void)start:(INgnNetworkChangCallback *)networkChangeCallback;
- (void)uninit;

@end
