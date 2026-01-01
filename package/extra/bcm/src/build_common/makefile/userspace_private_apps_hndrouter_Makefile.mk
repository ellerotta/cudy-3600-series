######################################################################
# + SDK/userspace/private/apps/hndrouter/Makefile
#

# Purpose:
#	export some -D to USR_CFLAGS, CFLAGS in main/components/router/Makefile will add it.
#

ifneq ($(strip $(BUILD_MCAST_PROXY)),)
ifneq ($(strip $(BUILD_IPV6)),)
USR_CFLAGS += -DBCM_NBUFF_WLMCAST_IPV6
endif
endif

USR_CFLAGS += -DCHIP_$(BRCM_CHIP)
USR_CFLAGS += -DWL_DEFAULT_NUM_SSID=$(BRCM_DEFAULT_NUM_MBSS)

ifneq ($(strip $(BUILD_RDKWIFI)),)
USR_CFLAGS += -I$(BCM_TARGET_PATH)/userspace/private/libs/wlcsm/wifi_all/cmwifi/libs/wlcsm/include
USR_CFLAGS += -DWL_MAX_NUM_SSID=$(BRCM_DEFAULT_NUM_MBSS)
ifneq ($(strip $(PHASE2_SEPARATE_RC)),)
USR_CFLAGS += -DPHASE2_SEPARATE_RC
endif
RDK_GENERIC_INSTALL=$(shell grep "^.*wifi_generic_install.*:" ${WIRELESS_IMPL_PATH}/cmwifi/cmwifi.mk)
RDK_GENERIC_APPS=$(shell grep "^.*wifi_generic_apps.*:" ${WIRELESS_IMPL_PATH}/cmwifi/cmwifi.mk)
USR_CFLAGS += -DBUILD_RDKWIFI
endif

ifeq ($(strip $(BRCM_USER_SSP)),y)
USR_CFLAGS  += $(SSP_MIN_COMPILER_OPTS)
USR_LDFLAGS += $(SSP_LIBS)
endif

ifneq ($(strip $(BCA_CPEROUTER)),)
export USE_EXTERNAL_HTTPD=y
USR_CFLAGS += -DBCA_CPEROUTER  -DNO_NVRAM_GUI -DUSE_EXTERNAL_HTTPD -I$(BCM_TARGET_PATH)/userspace/private/libs/wlcsm/include
ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
USR_CFLAGS += -DBCA_SUPPORT_UNFWLCFG
# TODO: Workaround for building wbd2 successfully with openssl v3.
# It should be removed once the wbds is migrating to the new openssl APIs.
USR_CFLAGS += -Wno-error=deprecated-declarations
endif
ifeq ($(strip $(BRCM_IKOS)),y)
# for IKOS APP, only need to have wl and/or dhd command,enable dhd if needed. 
IKOS_WLAPPS := wlexe
#IKOS_WLAPPS += dhd
IKOS_WLAPPS_INSTALL:= $(foreach app, $(IKOS_WLAPPS),$(app)-install)
IKOS_WLAPPS_CLEAN:= $(foreach app, $(IKOS_WLAPPS),$(app)-clean)
endif

ifneq ($(strip $(BUILD_WLDATAELD)),)
ifeq ($(findstring _$(strip $(BCM_WLIMPL))_,_81_87_),)
USR_CFLAGS += -I$(BCM_TARGET_PATH)/userspace/private/apps/wlan/wbd_dataelms
USR_CFLAGS += -DWBD_DATAELMS
export BCA_CPEROUTER_WLDATAELD_LDFLAGS = -lwbddataelms
endif
endif

endif

#ifeq ($(strip $(BCA_CPEROUTER_LIBNL)),y)
#USR_CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include/libnl3
#export PKG_CONFIG_PATH := $(BCM_FSBUILD_DIR)/public/lib/pkgconfig:$(PKG_CONFIG_PATH)
#endif


export USR_CFLAGS

#
# - SDK/userspace/private/apps/hndrouter/Makefile
######################################################################