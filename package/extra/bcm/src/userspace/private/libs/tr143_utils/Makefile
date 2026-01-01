#***********************************************************************
#
#  Copyright (c) 2013  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

#
# TODO: split into Bcmbuild.mk, Makefile, and Makefile.fullsrc, and build in obj subdir
#

LIB = libtr143_utils.so

OBJS := common.o protocol.o ftp.o http.o  

all dynamic install: conditional_build 

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)
	rm -f $(BCM_FSBUILD_DIR)/private/include/tr143_defs.h


ifeq ($(BCM_MODULAR_BUILD),)
# Old way: infer location of make.common based on pwd.
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
else
# New Modular Build way: EXT_BUILD_DIR must be set.
# Also point BUILD_DIR to EXT_BUILD_DIR
BUILD_DIR := $(EXT_BUILD_DIR)
include $(EXT_BUILD_DIR)/make.common
endif

#
# Private apps and libs are allowed to include header files from the
# private and public directories
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I . \
                         -I$(BCM_FSBUILD_DIR)/kernel/$(USER_ARCH)/include \
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)

ifneq ($(strip $(BUILD_SPDTEST)),)
	ALLOWED_INCLUDE_PATHS += -I$(BUILD_DIR)/bcmdrivers/opensource/include/bcm963xx
	BCM_LD_FLAGS += -L$(INSTALL_DIR)/lib -lbcm_spdt -lbdmf -lrt
	CFLAGS += -DBRCM_SPDTEST
endif

ifeq ($(strip $(BCM_SPEEDYGET)),y)
	CFLAGS += -DCONFIG_BCM_SPEEDYGET
endif

ifeq ($(PROFILE_KERNEL_VER), LINUX_4_19_0)
ifeq ($(strip $(DESKTOP_LINUX)),)
CFLAGS += -DHAVE_TCP_DISCARD
endif
endif


ifneq ($(strip $(BUILD_TR69_TR143)),)
ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_TR143_UTILS := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_TR143_UTILS := 1
endif
endif

ifeq ($(strip $(COND_BUILD_TR143_UTILS)),1)

conditional_build: $(BCM_FSBUILD_DIR)/private/include/tr143_defs.h sanity_check generic_private_lib_install

$(BCM_FSBUILD_DIR)/private/include/tr143_defs.h: $(LIB)
	mkdir -p $(dir $@)
	$(INSTALL_HEADERS_WITH_CP) $(notdir $@) $(dir $@)

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive

else

conditional_build: sanity_check
	@echo "Skipping $(LIB) (not configured)"

endif


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
