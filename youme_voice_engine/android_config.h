#include "config.h"

#ifdef HAVE_NEON
#include <arm_neon.h>
#endif

#define AUDIO_OPENSLES_UNDER_ANDROID 1
/* Define if <ifaddrs.h> header exist */
#define HAVE_IFADDRS_H 0

#define HAVE_GETIFADDRS 0
/* Define if <netpacket/packet.h> header exist */
#define HAVE_NETPACKET_PACKET_H 1
/* Define if <net/if_types> header exist */
#define HAVE_NET_IF_TYPES_H 1

#define VIDEO_UNDER_ANDROID 1
