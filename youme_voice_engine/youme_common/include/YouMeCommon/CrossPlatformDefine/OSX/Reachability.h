
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>


typedef enum : NSInteger
{
    NotReachable = 0,
    ReachableViaWiFi,
    ReachableViaWWAN
} NetworkStatus;


extern NSString *kYouMeReachabilityChangedNotification;


@interface YouMeReachability : NSObject

{
    BOOL _alwaysReturnLocalWiFiStatus; // default is NO
    SCNetworkReachabilityRef _reachabilityRef;
}
/*!
 * Use to check the reachability of a given host name.
 */
+ (instancetype)reachabilityWithHostName;

/*!
 * Use to check the reachability of a given IP address.
 */
+ (instancetype)reachabilityWithAddress:(const struct sockaddr_in *)hostAddress;

/*!
 * Checks whether the default route is available. Should be used by applications that do not connect
 * to a particular host.
 */
+ (instancetype)reachabilityForInternetConnection;

/*!
 * Checks whether a local WiFi connection is available.
 */
+ (instancetype)reachabilityForLocalWiFi;

/*!
 * Start listening for reachability notifications on the current run loop.
 */
- (BOOL)startNotifier;
- (void)stopNotifier;

- (NetworkStatus)currentReachabilityStatus;

/*!
 * WWAN may be available, but not active until a connection has been established. WiFi may require a
 * connection for VPN on Demand.
 */
- (BOOL)connectionRequired;

@end
