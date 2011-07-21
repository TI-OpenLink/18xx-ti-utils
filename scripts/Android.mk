LOCAL_PATH := $(call my-dir)

local_target_dir := $(TARGET_OUT)/etc/wifi

include $(CLEAR_VARS)
LOCAL_MODULE := wl12xx-tool.sh
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_MODULE_CLASS := ETC
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

