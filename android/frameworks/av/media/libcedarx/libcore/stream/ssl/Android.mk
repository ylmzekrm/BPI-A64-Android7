LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LIB_ROOT=$(LOCAL_PATH)/../..
include $(LIB_ROOT)/../config.mk
include $(LIB_ROOT)/stream/config.mk

LOCAL_SRC_FILES = \
		$(notdir $(wildcard $(LOCAL_PATH)/*.c))

LOCAL_C_INCLUDES:= \
    $(LIB_ROOT)/base/include \
    $(LIB_ROOT)/stream/include \
	$(LIB_ROOT)/include/     \
    $(LIB_ROOT)/../

android_version = $(shell echo $(PLATFORM_VERSION) | cut -c 1)
ifeq ($(android_version), 6)
LOCAL_C_INCLUDES += \
    $(TOP)/external/boringssl/src/include
else ifeq ($(android_version), 7)
LOCAL_C_INCLUDES += \
	$(TOP)/external/boringssl/src/include
else
LOCAL_C_INCLUDES += \
	$(TOP)/external/openssl/include
endif

LOCAL_CFLAGS += $(CDX_CFLAGS)

LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE:= libcdx_ssl_stream

ifeq ($(TARGET_ARCH),arm)
    LOCAL_CFLAGS += -Wno-psabi
endif

include $(BUILD_STATIC_LIBRARY)

