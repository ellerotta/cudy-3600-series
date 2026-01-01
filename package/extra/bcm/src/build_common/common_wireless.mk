ROUTER_PROFILE=$(CONFIG_BCM_CHIP_ID)
# for getting buildroot.config <== correspond to BCM_SDK/target/$(profile)/$(profile)
export BCM_BUILDROOT_CONFIG_DIR := $(TOPDIR)
# for getting kernel's .config <== correspond to BCM_SDK/kernel/linux-*.*/.config
export BCM_KERNEL_DIR := $(LINUX_DIR)
# for getting router .config <== correspond to BCM_SDK/bcmdrivers/broadcom/net/wl/impl03/main/components/router/.config

export BCM_ROUTER_CONFIG_DIR := $(BCM_TARGET_PATH)/config/$(ROUTER_PROFILE)
# for getting common_platform_bottom.mk <== correspond to BCM_SDK/make.common
export BCM_BUILD_COMMON_DIR := $(BCM_TARGET_PATH)/build_common


BCM_MAKEFILE_PATH:=$(BCM_BUILD_COMMON_DIR)/makefile


# csp specifc
include $(BCM_MAKEFILE_PATH)/csp.mk

# get bcm config
include $(BCM_MAKEFILE_PATH)/csp_get_translated_bcmcfg.mk

# make.common: include somes parts of make.common which affect wireless package's CFLAGS
include $(BCM_TARGET_PATH)/make.common

include $(BCM_TARGET_PATH)/make.wlan

include $(BCM_MAKEFILE_PATH)/make_common/make_common.wireless

# include some makefiles which affect wireless package's CFLAGS
include $(BCM_MAKEFILE_PATH)/bcmdrivers_Makefile.mk
include $(BCM_MAKEFILE_PATH)/userspace_private_apps_hndrouter_Makefile.mk

# export CFLAGS for wireless package
-include $(BCM_ROUTER_CONFIG_DIR)/.config
include $(BCM_MAKEFILE_PATH)/bcmdrivers_broadcom_net_wl_implX_main_components_router_Makefile.mk
