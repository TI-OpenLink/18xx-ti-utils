LOCAL_PATH:= $(call my-dir)

# wlconf
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        main.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)

LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := wlconf

include $(BUILD_EXECUTABLE)
