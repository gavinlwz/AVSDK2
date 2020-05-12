APP_STL := gnustl_static
NDK_TOOLCHAIN_VERSION := 4.9
APP_PROJECT_PATH  	:= $(LOCAL_PATH)
APP_MODULES 	  	:= youme_voice_engine

APP_CPPFLAGS := -frtti -DANDROID -std=c++11 -fsigned-char
APP_LDFLAGS := -latomic -Wl,--exclude-libs,ALL

ifeq ($(NDK_DEBUG),1)
APP_OPTIM := debug
else
APP_CPPFLAGS += -DNDEBUG -DYOUME_VIDEO
APP_OPTIM := release
endif

