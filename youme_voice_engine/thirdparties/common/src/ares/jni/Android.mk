LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
ifeq ($(NEON),1)
LOCAL_ARM_NEON := true
LOCAL_CFLAGS += -DHAVE_NEON
LOCAL_CPPFLAGS += -DHAVE_NEON
endif
endif

LOCAL_MODULE := ares

LOCAL_CPP_EXTENSION := .cc  .cpp


LOCAL_CFLAGS := \
-DHAVE_CONFIG_H -DANDROID\
-DCARES_STATICLIB\
-Os \
-fvisibility=hidden\

LOCAL_CPPFLAGS +=\
-DHAVE_CONFIG_H\
-DANDROID\
-DCARES_STATICLIB\
-Os \
-std=c++11\
-fvisibility=hidden\

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../..\

LOCAL_SRC_FILES := \
$(LOCAL_PATH)/../ares_init.cpp \
$(LOCAL_PATH)/../ares__close_sockets.cpp \
$(LOCAL_PATH)/../ares__get_hostent.cpp \
$(LOCAL_PATH)/../ares__read_line.cpp \
$(LOCAL_PATH)/../ares__timeval.cpp \
$(LOCAL_PATH)/../ares_cancel.cpp \
$(LOCAL_PATH)/../ares_create_query.cpp \
$(LOCAL_PATH)/../ares_data.cpp \
$(LOCAL_PATH)/../ares_destroy.cpp \
$(LOCAL_PATH)/../ares_expand_name.cpp \
$(LOCAL_PATH)/../ares_expand_string.cpp \
$(LOCAL_PATH)/../ares_fds.cpp \
$(LOCAL_PATH)/../ares_free_hostent.cpp \
$(LOCAL_PATH)/../ares_free_string.cpp \
$(LOCAL_PATH)/../ares_getenv.cpp \
$(LOCAL_PATH)/../ares_gethostbyaddr.cpp \
$(LOCAL_PATH)/../ares_gethostbyname.cpp \
$(LOCAL_PATH)/../ares_getnameinfo.cpp \
$(LOCAL_PATH)/../ares_getsock.cpp \
$(LOCAL_PATH)/../ares_library_init.cpp \
$(LOCAL_PATH)/../ares_llist.cpp \
$(LOCAL_PATH)/../ares_mkquery.cpp \
$(LOCAL_PATH)/../ares_nowarn.cpp \
$(LOCAL_PATH)/../ares_options.cpp \
$(LOCAL_PATH)/../ares_parse_a_reply.cpp \
$(LOCAL_PATH)/../ares_parse_aaaa_reply.cpp \
$(LOCAL_PATH)/../ares_parse_mx_reply.cpp \
$(LOCAL_PATH)/../ares_parse_naptr_reply.cpp \
$(LOCAL_PATH)/../ares_parse_ns_reply.cpp \
$(LOCAL_PATH)/../ares_parse_ptr_reply.cpp \
$(LOCAL_PATH)/../ares_parse_soa_reply.cpp \
$(LOCAL_PATH)/../ares_parse_srv_reply.cpp \
$(LOCAL_PATH)/../ares_parse_txt_reply.cpp \
$(LOCAL_PATH)/../ares_platform.cpp \
$(LOCAL_PATH)/../ares_process.cpp \
$(LOCAL_PATH)/../ares_query.cpp \
$(LOCAL_PATH)/../ares_search.cpp \
$(LOCAL_PATH)/../ares_send.cpp \
$(LOCAL_PATH)/../ares_strcasecmp.cpp \
$(LOCAL_PATH)/../ares_strdup.cpp \
$(LOCAL_PATH)/../ares_strerror.cpp \
$(LOCAL_PATH)/../ares_timeout.cpp \
$(LOCAL_PATH)/../ares_version.cpp \
$(LOCAL_PATH)/../ares_writev.cpp \
$(LOCAL_PATH)/../bitncmp.cpp \
$(LOCAL_PATH)/../inet_net_pton.cpp \
$(LOCAL_PATH)/../inet_ntop.cpp \

include $(BUILD_STATIC_LIBRARY)


