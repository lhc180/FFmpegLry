LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_STATIC_LIBRARIES := libavformat libavcodec libavutil libpostproc libswscale
LOCAL_MODULE := ffmpeg
LOCAL_SRC_FILES := ../ffmpegEntry/FFmpegEntry.c
LOCAL_LDLIBS += -llog -ljnigraphics -lz -ldl -lgcc
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))
