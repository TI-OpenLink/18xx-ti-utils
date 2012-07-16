LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
        main.c \
	crc32.c
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)
LOCAL_CFLAGS += -DWLCONF_DIR=\"/system/etc/wifi/wlconf/\"
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE := wlconf
include $(BUILD_EXECUTABLE)

# Copy configuration files

etc_target_dir := $(TARGET_OUT)/etc/wifi/wlconf

include $(CLEAR_VARS)
LOCAL_SRC_FILES := struct.bin
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := dictionary.txt
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := wl18xx-conf-default.bin
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := example.conf
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := example.ini
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := README
LOCAL_MODULE_TAGS := debug
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(etc_target_dir)
LOCAL_MODULE := $(LOCAL_SRC_FILES)
include $(BUILD_PREBUILT)

include $(LOCAL_PATH)/official_inis/Android.mk
