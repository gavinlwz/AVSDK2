
#import "HeadsetMonitor.h"
#import "YouMeVoiceEngine.h"


static HeadsetMonitor *s_HeadsetMonitor_sharedInstance = nil;

@interface HeadsetMonitor ()
@property (atomic) INgnNetworkChangCallback *mCallback;

@end


@implementation HeadsetMonitor

+ (HeadsetMonitor *)getInstance
{
    @synchronized (self)
    {
        if (s_HeadsetMonitor_sharedInstance == nil)
        {
            s_HeadsetMonitor_sharedInstance = [self alloc];
        }
    }

    return s_HeadsetMonitor_sharedInstance;
}

+ (void)destroy
{
    s_HeadsetMonitor_sharedInstance = nil;
}
- (id)init
{
    self = [super init];
    return self;
}

- (void)start
{
    if (self)
    {
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (onPluginHeadset:) name:@"PluggingHeadset" object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector (onUnPluginHeadset:) name:@"unPluggingHeadset" object:nil];
    }
    return;
}


- (void)onPluginHeadset:(NSNotification *)note
{
    CYouMeVoiceEngine::getInstance ()->onHeadSetPlugin (1);
}
- (void)onUnPluginHeadset:(NSNotification *)note
{
    CYouMeVoiceEngine::getInstance ()->onHeadSetPlugin (0);
}


@end
