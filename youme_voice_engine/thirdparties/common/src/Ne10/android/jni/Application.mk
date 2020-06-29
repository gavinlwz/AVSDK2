APP_ABI := armeabi-v7a
NDK_TOOLCHAIN_VERSION := 4.9
ANDROID_API_LEVEL 	:= android-9
APP_PROJECT_PATH  	:= $(LOCAL_PATH)
APP_MODULES 	  	:= ne10

ifeq ($(NDK_DEBUG),1)
  APP_OPTIM := debug
else
  APP_CPPFLAGS += -DNDEBUG
  APP_OPTIM := release
endif
