LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfilter


# jni
LOCAL_SRC_FILES := imagefilter.cpp text_search.cpp

LOCAL_LDLIBS += \
  -lGLESv1_CM \
  -ljnigraphics \
  -llog \
    
#common
LOCAL_SHARED_LIBRARIES:= liblept
LOCAL_PRELINK_MODULE:= false
LOCAL_DISABLE_FORMAT_STRING_CHECKS:=true

include $(BUILD_SHARED_LIBRARY)
