LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
        main.c \
	crc32.c

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)

LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := wlconf

include $(BUILD_EXECUTABLE)
