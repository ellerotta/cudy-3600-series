######################################################################
# + package shared codes path
#
SHARED_CODE_ROOT_PATH:=$(STAGING_DIR)/usr/share/bcm_shared_code

bcm_components_awd_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_components_awd
bcm_components_bcmcrypto_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_components_bcmcrypto
bcm_components_math_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_components_math
bcm_components_proto_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_components_proto
bcm_components_router_bcmdrv_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_main_components_router_bcmdrv
bcm_components_shared_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_main_components_shared
bcm_components_wioctl_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_components_wioctl

bcm_mainsys_src_include_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_src_include
bcm_mainsys_src_makefiles_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_src_makefiles
bcm_mainsys_src_shared_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_mainsys_src_shared

bcm_wl_shared_path:=$(SHARED_CODE_ROOT_PATH)/bcmdrivers_broadcom_net_wl_shared

# hnd_wl(wl.ko) shared codes for other packages
bcm_hnd_wl_share_sys_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_main_src_wl_sys
bcm_hnd_wl_share_config_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_main_src_wl_config
bcm_hnd_wl_share_ppr_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_main_src_wl_ppr

# wlexe(wl) shared codes for other packages
bcm_wl_exe_share_exe_path:=$(SHARED_CODE_ROOT_PATH)/impl$(CONFIG_BCM_WLIMPL)_sys_src_wl_exe
#
# - package shared codes path
######################################################################

######################################################################
#

# needed by kernel module build
export LINUX_DIR
# needed by wireless driver: obj-$(CONFIG_BCM_WLAN)
export CONFIG_BCM_WLAN
# needed by kernel module code
export src=.
#
# - SLP specific
######################################################################
