LOCAL_PATH := $(call my-dir)

# Setup Bluetooth local make variables for handling configuration
ifneq ($(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR),)
  bluetooth_C_INCLUDES := $(BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR)
  bluetooth_CFLAGS += -DHAS_BDROID_BUILDCFG
else
  bluetooth_C_INCLUDES :=
  bluetooth_CFLAGS += -DHAS_NO_BDROID_BUILDCFG
endif

ifeq ($(BOARD_HAVE_BLUETOOTH_RTK),true)
$(error "RTK BLUETOOTH")
# RealTek Bluetooth private configuration table
  bdroid_CFLAGS := -Wno-unused-parameter
  rtkbt_bdroid_C_INCLUDES += $(LOCAL_PATH)/bta/hh
  rtkbt_bdroid_C_INCLUDES += $(LOCAL_PATH)/bta/dm
  rtkbt_bdroid_CFLAGS += -DBLUETOOTH_RTK
  rtkbt_bdroid_CFLAGS += -DBLUETOOTH_RTK_API
  rtkbt_bdroid_CFLAGS += -DBLUETOOTH_RTK_COEX
  bluetooth_C_INCLUDES += $(rtkbt_bdroid_C_INCLUDES)
  bluetooth_CFLAGS += $(rtkbt_bdroid_CFLAGS)
endif

ifneq ($(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED),)
  bluetooth_CFLAGS += -DHCILP_INCLUDED=$(BOARD_BLUETOOTH_BDROID_HCILP_INCLUDED)
endif

ifneq ($(TARGET_BUILD_VARIANT),user)
bluetooth_CFLAGS += -DBLUEDROID_DEBUG
endif

bluetooth_CFLAGS += -DEXPORT_SYMBOL="__attribute__((visibility(\"default\")))"

#
# Common C/C++ compiler flags.
#
# -Wno-gnu-variable-sized-type-not-at-end is needed, because struct BT_HDR
#  is defined as a variable-size header in a struct.
# -Wno-typedef-redefinition is needed because of the way the struct typedef
#  is done in osi/include header files. This issue can be obsoleted by
#  switching to C11 or C++.
# -Wno-unused-parameter is needed, because there are too many unused
#  parameters in all the code.
#
ifeq ($(BOARD_HAVE_BLUETOOTH_RTK),true)
bluetooth_CFLAGS += \
  -fvisibility=hidden \
  -Wall \
  -Wextra \
  -Wno-gnu-variable-sized-type-not-at-end \
  -Wno-typedef-redefinition \
  -Wno-unused-parameter \
  -Wunused-but-set-variable \
  -UNDEBUG \
  -DLOG_NDEBUG=1
else
bluetooth_CFLAGS += \
  -fvisibility=hidden \
  -Wall \
  -Wextra \
  -Werror \
  -Wno-gnu-variable-sized-type-not-at-end \
  -Wno-typedef-redefinition \
  -Wno-unused-parameter \
  -UNDEBUG \
  -DLOG_NDEBUG=1
endif

bluetooth_CONLYFLAGS += -std=c99
bluetooth_CPPFLAGS :=

include $(call all-subdir-makefiles)

# Cleanup our locals
bluetooth_C_INCLUDES :=
bluetooth_CFLAGS :=
bluetooth_CONLYFLAGS :=
bluetooth_CPPFLAGS :=
