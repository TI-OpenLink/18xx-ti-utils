LOCAL_PATH:= $(call my-dir)

#
# asi
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	sdio_access.c \
	device_mode.c \
	log.c \
	asi.c \

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../ \
	external/libnl-headers
LOCAL_CFLAGS += -DCONFIG_LIBNL20
LOCAL_MODULE_TAGS := debug
LOCAL_STATIC_LIBRARIES := libnl_2
LOCAL_MODULE := asi
include $(BUILD_EXECUTABLE)