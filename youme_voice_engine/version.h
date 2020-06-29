//
//  version.h
//  youme_voice_engine
//
//  Created by wanglei on 15/10/22.
//  Copyright © 2015年 youme. All rights reserved.
//

#ifndef version_h
#define version_h

// 版本号定义，前三个版本号根据需要手动更新，最后一个版本号 BUILD_NUMBER 由编译脚本在每次编译开始前自动更新
// 主版本号和次版本号相同的SDK，具有完全的兼容性，可直接替换升级
#define MAIN_VER       3  //主版本号，当有大的功能提升时，该版本号加1
#define MINOR_VER      4  //次版本号，当API有改动时，该版本号加1
#define RELEASE_NUMBER 0  //发布版本号，每次正式提测准备对外发布的版本时，该版本号加1为奇数。测试过程发现严重问题，
                          //修正后回归测试时，该版本号不变。测试过程没发现问题或只发现小问题，该版本号加1为偶数。

#define BUILD_NUMBER 886

#define SDK_VERSION "%d.%d.%d.%d"

/**
 * MAIN_VER         - 4 bite
 * MINOR_VER        - 6 bite
 * RELEASE_NUMBER   - 8 bite
 * BUILD_NUMBER     - 14 bite

 |  4 |   6  |   8    |      14      |
 |----|------|--------|--------------|

 * 最大支持版本 15.63.255.16383
 */
#define SDK_NUMBER  ( (((MAIN_VER) << 28) & 0xF0000000) | (((MINOR_VER) << 22) & 0x0FC00000) | (((RELEASE_NUMBER) << 14) & 0x003FC000) | ((BUILD_NUMBER) & 0x00003FFF) )

// 每次拉branch或者tag, 这个名字必须改为跟branch或tag对应的名字, 否则就会存在这种情况:
// branch/tag编出来的包版本号跟trunk或其他branch/tag一样，但实际上对应的代码却不一样.
// 命名规则：branch： "branch-xxxx"， xxxx为branch名字的后缀， 如branch名字为 client2.0-dev-0, 则取名 "branch-dev-0"
//         tag: "tag-xxxx", 跟branch类似。
#define BRANCH_TAG_NAME    "video-trunk"

// 是否支持用FFMPEG库播放媒体文件
#define FFMPEG_SUPPORT 1
#ifdef WIN32
#if !defined(WIN_FFMPEG_SUPPORT)
#undef FFMPEG_SUPPORT
#endif
#endif

#ifndef WIN32
#define MIN_VERSION 0
#endif

#endif /* version_h */
