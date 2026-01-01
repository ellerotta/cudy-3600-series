#Linux Makefile
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
WL_SRC_BASE := /main/src
LIBWLCSM_D := libwlcsm.so
LIBWLCSM_S := libwlcsm.a

all dynamic: conditional_build

include $(BCM_TARGET_PATH)/make.common
include $(BCM_TARGET_PATH)/make.wlan

SRCBASE=$(WIRELESS_DRIVER_PATH)
WLCSM_EXT_BASE=$(BRCMDRIVERS_DIR)/opensource/char/bcm_knvram/impl1
KERNEL_NVRAM_FILE_NAME := kernel_nvram.setting

ifeq ($(strip $(BUILD_BRCM_CMS)),)
BUILD_NO_CMS=y
endif
ifeq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
BUILD_MNGR_CHANNEL = $(BUILD_TR181_WLMNGR)
else
CFLAGS+=-DUNF_CFG
endif

ifneq ($(strip $(RDK_BUILD)),)
CFLAGS+=-DBCA_CPEROUTER_RDK
endif

ifneq ($(strip $(CONFIG_MTD_NAND)),)
	CFLAGS += -DNAND_SYS -DKERNEL_NVRAM_FILE_NAME='"/data/.$(KERNEL_NVRAM_FILE_NAME)"'
endif

ifneq ($(strip $(BUILD_RDKWIFI)),)
# Need to generate libwlcsm.so from RDK side
CFLAGS += -DBUILD_RDKWIFI
RDK_GENERIC_INSTALL=$(shell grep "^wifi_generic_install:" ${WIRELESS_IMPL_PATH}/cmwifi/cmwifi.mk)
endif

INCLUDE += -I$(INC_INSTALL_KERNEL_HDR_PATH)/include -I$(WLCSM_EXT_BASE)/include -I./include
INCLUDE += -I$(HNDAPPS_DIR)/../wlioctl/include -I$(HNDAPPS_DIR)/../proto/include
INCLUDE += -I. -I.. -I$(SRCBASE)/include -I$(SRCBASE)/common/include -I$(SRCBASE)/shared -I$(SRCBASE)/router/shared -I$(SRCBASE)/wl/exe
INCLUDE += -I$(SRCBASE)/common/include/devctrl_if
INCLUDE += -I$(SRCBASE)/security/secfrw/include -I$(SRCBASE)/supp/include -I$(SRCBASE)/../components/bcmwifi/include -I$(SRCBASE)/shared/bcmwifi/include
INCLUDE += -I$(BUILD_DIR)/userspace/public/include/linux
INCLUDE += -I$(BUILD_DIR)/userspace/public/include

CFLAGS += $(INCLUDE)
CFLAGS += -Wall
ifneq ($(strip $(BUILD_MNGR_CHANNEL)),)
CFLAGS += -DWITH_MNGR_CHANNEL
endif

ifneq ($(strip $(BCA_CPEROUTER)),)
CFLAGS += -DBCA_CPEROUTER
ifneq ($(strip $(BUILD_BCM_WLAN_DPDCTL)),)
CFLAGS += -DBUILD_BCM_WLAN_DPDCTL
endif
endif

ifneq ($(strip $(WLCSM_DEBUG_TRACE)),)
CFLAGS += -DWLCSM_DEBUG -g
else
CFLAGS += -s -Os
endif
CFLAGS += -DDSLCPE -Werror
LDFLAGS += -L. -L$(INSTALL_DIR)/lib

vpath %.c $(CURR_DIR)/src

WLCSM_FILES = $(foreach x, $(shell find ./src -name \*.c) ,$(notdir $(x)))
WLCSM_OBJS := $(foreach x, $(WLCSM_FILES),$(x:.c=.o))

_BUILD_STATIC_LIB_= $(shell ( [ ! -z $(BUILD_RDKWIFI) ] || [ -z $(BUILD_BRCM_UNFWLCFG) ] ||  [ ! -z $(WLCSM_DEBUG_TRACE) ]  ;  echo $$?))
_BUILD_DYNC_LIB_= $(shell ( [ -z $(BUILD_RDKWIFI) ]  &&  ( [ ! -z $(WLCSM_DEBUG_TRACE) ]   || [ -z $(BUILD_BRCM_UNFWLCFG) ] );  echo $$?))

WLCSM_OBJS := $(foreach x, $(WLCSM_FILES),$(x:.c=.o))
OBJS += $(WLCSM_OBJS)

rdkwifi_lib: $(LIBWLCSM_S)
	install $(LIBWLCSM_S) ./wifi_all/cmwifi/libs/wlcsm
	$(MAKE) -C ./wifi_all/cmwifi/libs/wlcsm BCA_CPEROUTER_RDKWIFI=1 BUILD_DIR=$(BUILD_DIR)  BSP_WLCSM_OBJS="$(WLCSM_OBJS)" WLSRCBASE="$(SRCBASE)"
ifeq ($(strip $(RDK_GENERIC_INSTALL)),)
	install -m 755 ./wifi_all/cmwifi/apps/util/script/wifi*.sh $(INSTALL_DIR)/bin
	-install -m 755 ./wifi_all/cmwifi/apps/util/script/*util.sh $(INSTALL_DIR)/bin
	ln -sf wifi_setup.sh $(INSTALL_DIR)/bin/erase
	ln -sf wifi_setup.sh $(INSTALL_DIR)/bin/rc
	-install ./wifi_all/cmwifi/release/nvram/nvram_*default_*.txt $(INSTALL_DIR)/etc/wlan/
endif

ifneq ($(strip $(BUILD_RDKWIFI)),)
# Need to generate libwlcsm.so from RDK side
ifneq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
# In this case, wlcsm.so will be generated from nvram as unfnvram is needed
LDFLAGS += -lnvram
extra_target: $(LIBWLCSM_S)
else
extra_target: rdkwifi_lib
# No unfnvram is needed, then can go ahead to build libwlcsm.so from RDK
endif
else
extra_target: $(LIBWLCSM_S)
endif

headerfile_links:
	ln -sf $(WLCSM_EXT_BASE)/include/wlcsm_linux.h  $(CURR_DIR)/include/wlcsm_linux.h
	ln -sf $(WLCSM_EXT_BASE)/include/wl_common_defs.h  $(CURR_DIR)/include/wl_common_defs.h
	ln -sf $(WLCSM_EXT_BASE)/include/wl_common_defs.h  $(BUILD_DIR)/userspace/private/include/wl_common_defs.h
ifeq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
	ln -sf $(BUILD_DIR)/userspace/private/libs/wlcsm/include/wlcsm_defs.h  $(BUILD_DIR)/userspace/private/include/wlcsm_defs.h
	ln -sf $(BUILD_DIR)/userspace/private/libs/wlcsm/include/wlcsm_lib_api.h  $(BUILD_DIR)/userspace/private/include/wlcsm_lib_api.h
endif
ifneq ($(strip $(BUILD_RDKWIFI)),)
	ln -sf $(WIRELESS_IMPL_PATH) wifi_all
	ln -sf $(BUILD_DIR)/userspace/private/libs/wlcsm/include/wlcsm_lib_wl.h  $(BUILD_DIR)/userspace/private/include/wlcsm_lib_wl.h
	ln -sf $(BUILD_DIR)/userspace/private/libs/wlcsm/include/wlcsm_lib_hspot.h  $(BUILD_DIR)/userspace/private/include/wlcsm_lib_hspot.h
	ln -sf $(WLCSM_EXT_BASE)/include/wlcsm_linux.h  $(BUILD_DIR)/userspace/private/include/wlcsm_linux.h
	ln -sf $(WLCSM_EXT_BASE)/include/wlcsm_linux.h  $(CURR_DIR)/wifi_all/cmwifi/libs/wlcsm/include/wlcsm_linux.h
	ln -sf $(WLCSM_EXT_BASE)/include/wl_common_defs.h  $(CURR_DIR)/wifi_all/cmwifi/libs/wlcsm/include/wl_common_defs.h
endif

ifeq ($(_BUILD_STATIC_LIB_),0)
$(LIBWLCSM_S): headerfile_links $(OBJS) $(WLCSM_EXT_BASE)/include/wlcsm_linux.h
	$(AR) $(ARFLAGS) $(LIBWLCSM_S) $(OBJS)
	$(RANLIB) $(LIBWLCSM_S)
else
$(LIBWLCSM_S): headerfile_links
	@echo "skipping libwlcsm.a build. Not needed "
endif

$(LIBWLCSM_D) : extra_target headerfile_links
ifeq ($(_BUILD_DYNC_LIB_),0)
	$(CC) -fPIC -shared -o $(LIBWLCSM_D) -Wl,--whole-archive $(LIBWLCSM_S) -Wl,--no-whole-archive -lpthread
	install -m 755 $(LIBWLCSM_D) $(INSTALL_DIR)/lib
else
	@echo "skipping libwlcsm.so build. Not needed "
endif

conditional_build: $(LIBWLCSM_D)

install: all
ifeq ($(_BUILD_DYNC_LIB_),0)
	install -m 755 $(LIBWLCSM_D) $(INSTALL_DIR)/lib
else
	@echo "No file to install"
endif

distclean clean:
	-rm -f *.o *.map *.d  *.d.* *.a
	-rm -f $(LIBWLCSM_D) $(LIBWLCSM_S)
	-rm -rf doc
ifneq ($(strip $(BUILD_RDKWIFI)),)
	-$(MAKE) -C ./wifi_all/cmwifi/libs/wlcsm BCA_CPEROUTER_RDKWIFI=1 BUILD_DIR=$(BUILD_DIR)  clean
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_lib_wl.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_lib_hspot.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_linux.h
	-rm -rf $(CURR_DIR)/wifi_all/cmwifi/libs/wlcsm/include/wlcsm_linux.h
	-rm -rf $(CURR_DIR)/wifi_all/cmwifi/libs/wlcsm/include/wl_common_defs.h
endif
	-rm -rf wifi_all
	-rm -rf $(CURR_DIR)/include/wlcsm_linux.h
	-rm -rf $(CURR_DIR)/include/wl_common_defs.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wl_common_defs.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_lib_api.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_defs.h
	-rm -f $(INSTALL_DIR)/lib/$(LIBWLCSM_D)
ifeq ($(strip $(BUILD_BRCM_UNFWLCFG)),)
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_defs.h
	-rm -rf $(BUILD_DIR)/userspace/private/include/wlcsm_lib_api.h
endif
#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#
%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

include $(BCM_TARGET_PATH)/make.deprules

#ifeq ($(shell [[ $(_BUILD_DYNC_LIB_) == 0 || $(_BUILD_STATIC_LIB_) ==0 ]] && echo true), true)
-include $(OBJS:.o=.d)
#endif
