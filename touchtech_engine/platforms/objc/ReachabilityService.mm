#import <CoreTelephony/CTTelephonyNetworkInfo.h>
#import "INgnNetworkService.h"
#import "ReachabilityService.h"
#import "Reachability.h"

static YouMeTalkReachabilityServiceIOS *s_YouMeTalkReachabilityServiceIOS_sharedInstance = nil;

@interface YouMeTalkReachabilityServiceIOS ()

@property (nonatomic) YouMeReachability *hostReachability;
@property (nonatomic) YouMeReachability *internetReachability;
@property (nonatomic) YouMeReachability *wifiReachability;
@property (atomic) INgnNetworkChangCallback *mCallback;

@end


@implementation YouMeTalkReachabilityServiceIOS

+ (YouMeTalkReachabilityServiceIOS *)getInstance
{
    @synchronized (self)
    {
        if (s_YouMeTalkReachabilityServiceIOS_sharedInstance == nil)
        {
            s_YouMeTalkReachabilityServiceIOS_sharedInstance = [[self alloc] init ];
        }
    }

    return s_YouMeTalkReachabilityServiceIOS_sharedInstance;
}

+ (void)destroy
{
    s_YouMeTalkReachabilityServiceIOS_sharedInstance = nil;
}
- (id)init
{
    self = [super init];
    return self;
}
- (void)uninit
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if(self.hostReachability != nil){
        [self.hostReachability stopNotifier];
    }
    
    if(self.internetReachability != nil){
        [self.internetReachability stopNotifier];
    }
    if(self.wifiReachability != nil){
        [self.wifiReachability stopNotifier];
    }
    self.mCallback=nil;
}
- (void)start:(INgnNetworkChangCallback *)networkChangeCallback
{
    if (self)
    {
        self.mCallback = networkChangeCallback;


        /*
         Observe the kNetworkReachabilityChangedNotification. When that notification is posted, the
         method YouMeTalkReachabilityChanged will be called.
         */
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (YouMeTalkReachabilityChanged:) name:kYouMeReachabilityChangedNotification object:nil];

        // Change the host name here to change the server you want to monitor.
        self.hostReachability = [YouMeReachability reachabilityWithHostName];
        [self.hostReachability startNotifier];
        [self updateInterfaceWithReachability:self.hostReachability];

        self.internetReachability = [YouMeReachability reachabilityForInternetConnection];
        [self.internetReachability startNotifier];
        [self updateInterfaceWithReachability:self.internetReachability];

        self.wifiReachability = [YouMeReachability reachabilityForLocalWiFi];
        [self.wifiReachability startNotifier];
        [self updateInterfaceWithReachability:self.wifiReachability];
    }
    return;
}


/*!
 * Called by Reachability whenever status changes.
 */
- (void)YouMeTalkReachabilityChanged:(NSNotification *)note
{
    YouMeReachability *curReach = [note object];
    if ([curReach isKindOfClass:[YouMeReachability class]]) {
        NSParameterAssert ([curReach isKindOfClass:[YouMeReachability class]]);
        [self updateInterfaceWithReachability:curReach];
    }
}


- (void)updateInterfaceWithReachability:(YouMeReachability *)reachability
{

    if (self.mCallback == nil)
    {
        return;
    }


    NetworkStatus netStatus = [reachability currentReachabilityStatus];
    BOOL connectionRequired = [reachability connectionRequired];

    switch (netStatus)
    {
    case NotReachable:
    {
//        connectionRequired = NO;
        self.mCallback->onNetWorkChanged (NETWORK_TYPE_NO);
        break;
    }

    case ReachableViaWWAN:
    {
        self.mCallback->onNetWorkChanged (NETWORK_TYPE_MOBILE);
        break;
    }
    case ReachableViaWiFi:
    {

        self.mCallback->onNetWorkChanged (NETWORK_TYPE_WIFI);
        break;
    }
    default:
        self.mCallback->onNetWorkChanged (NETWORK_TYPE_NO);
    }
}



@end
