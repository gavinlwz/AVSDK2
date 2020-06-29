


#import <Foundation/Foundation.h>
#import "INgnNetworkService.h"

@interface HeadsetMonitor : NSObject

+ (HeadsetMonitor *)getInstance;
+ (void)destroy;

- (id)init;
- (void)start;
@end
