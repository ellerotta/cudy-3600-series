#***********************************************************************
# 
#    Copyright (c) 2012 Broadcom Corporation
#    All Rights Reserved
#
#***********************************************************************/

#
# TODO: split into Bcmbuild.mk, Makefile, and Makefile.fullsrc, and build in obj subdir
#
LIB = libcms_qdm.so

OBJS = qdm_modsw_ee.o \
       qdm_intf.o qdm2_intf.o \
       qdm_ipintf.o qdm2_ipintf.o \
       qdm_lan.o qdm2_lan.o \
       qdm2_dhcpv4.o \
       qdm_route.o qdm2_route.o \
       qdm_dns.o qdm2_dns.o \
       qdm_multicast.o qdm2_multicast.o \
       qdm_dsl.o qdm2_dsl.o \
       qdm_qos.o qdm2_qos.o \
       qdm_vlan.o qdm2_vlan.o \
       qdm_ethernet.o qdm2_ethernet.o \
       qdm_tr69c.o qdm2_tr69c.o \
       qdm_ipv6.o qdm2_ipv6.o \
       qdm_xtm.o qdm2_xtm.o \
       qdm_diag.o qdm2_diag.o \
       qdm2_xmppc.o \
       qdm_system.o qdm2_system.o \
       qdm_wifi.o qdm2_wifi.o
       
all install: conditional_build

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)



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

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
include $(BUILD_DIR)/make.voice
endif

CFLAGS += $(SSP_TYP_COMPILER_OPTS)
BCM_LD_FLAGS += -L$(INSTALL_DIR)/lib $(SSP_TYP_LIBS)

#
# Private apps and libs are allowed to include header files from the
# private and public directories
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BCM_FSBUILD_DIR)/bcmdrivers/include \
                         -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)


# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_CMS_QDM := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_CMS_QDM := 1
endif

ifeq ($(strip $(COND_BUILD_CMS_QDM)),1)

conditional_build: generic_private_lib_install

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive

else

conditional_build:
	@echo "skipping $(LIB) (not configured)"

endif


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
