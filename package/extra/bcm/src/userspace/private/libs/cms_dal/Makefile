#***********************************************************************
#
#  Copyright (c) 2006-20012  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#
LIB = libcms_dal.so

OBJS = dal_main.o dal2_main.o \
       dal_wan.o \
       dal_lan.o dal2_lan.o dal2_bridging.o \
       dal_auth.o dal_route.o \
       dal_virtualserver.o dal_parentalcntrl.o dal_urlfilter.o \
       dal_sec.o dal2_sec.o  \
       dal_qos.o dal2_qos.o \
       dal_pmap.o  dal_cert.o dal_ethswitch.o dal_ddns.o dal_network.o dal_iptunnel.o \
       dal_pmirror.o dal2_pmirror.o \
       dal_diag.o dal_dnsproxy.o \
       dal_dns.o dal2_dns.o dal2_dns6.o \
       dal_eth.o dal2_eth.o dal_dsl.o dal2_dsl.o \
       dal_porttriggering.o dal_ipsec.o dal_pwrmngt.o \
       dal_multicast.o dal_l2tpac.o dal2_l2tpac.o dal2_l2tpns.o dal_storageservice.o dal_pptpac.o dal2_pptpns.o dal_autodetection.o \
       dal_modsw_ee.o \
       dal_gponwan.o dal_eponwan.o dal_bmu.o \
       dal_wifiwan.o dal2_wifiwan.o \
       dal2_wan.o dal2_wan6.o dal_lan6.o dal2_lan6.o \
       dal2_ip.o dal2_ip6.o dal2_ppp.o dal2_ethernet.o dal2_nat.o dal2_route.o dal2_ipsec.o \
       dal2_dhcp.o dal2_iptunnel.o dal2_diag.o dal2_virtualserver.o dal2_porttriggering.o \
       dal2_pcp.o dal_stun.o dal_ethlag.o dal2_vxlan.o dal2_gre.o

all install: conditional_build 

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)



#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BRCM_VOICE_SUPPORT)),)
#ifneq ($(strip $(BRCM_VOICE_APP_SIP)),)
include $(BUILD_DIR)/make.voice

OBJS += dal_voice_common.o

ifneq ($(strip $(BUILD_DM_TR104_V2)),)
    OBJS += dal2_voice.o dal2_dect.o
else
    OBJS += dal_voice.o dal_dect.o
endif

#
# need CMS_DMP_xxx defination
#
CUSTOM_CFLAGS += $(CMS_DMP_FLAGS)

endif

ifeq ($(strip $(BUILD_SPDSVC)), y)
OBJS += dal_spdsvc.o
endif

ifneq ($(strip $(BUILD_OPENVSWITCH)),)
OBJS += dal_openVS.o dal2_openVS.o
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
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core/linux \
                         -I$(BUILD_DIR)/userspace/private/libs/cms_core/linux/device2 \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)


ifneq ($(strip $(WIRELESS)),)
ALLOWED_INCLUDE_PATHS += -I$(INC_BCMDRIVER_PATH)/include
endif

# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors # NEW_FORBID_WARNINGS


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_CMS_DAL := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_CMS_DAL := 1
endif

ifeq ($(strip $(COND_BUILD_CMS_DAL)),1)

conditional_build: generic_private_lib_install

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,-soname,$(notdir $@) -o $@ $(OBJS)

else

conditional_build:
	@echo "skipping $(LIB) (not configured)"

endif


#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)
