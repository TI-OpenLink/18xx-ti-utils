LOCAL_PATH:= $(call my-dir)

# Copy ini files

etc_target_dir := $(TARGET_OUT)/etc/wifi/wlconf/official_inis

include $(CLEAR_VARS)
LOCAL_SRC_FILES := WL8_System_parameters.ini
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

