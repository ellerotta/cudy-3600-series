#***********************************************************************
#
#    Copyright (c) 2006 Broadcom Corporation
#    All Rights Reserved
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#
LIB = libcms_cli.so

OBJS = cli.o

all install: conditional_build



# BRCM_SUPPORTS_MULTIARCH_BUILD

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
include $(BUILD_DIR)/make.voice
endif

ifneq ($(strip $(BRCM_XDSL_DISTPOINT_USE_G9991)),)
CFLAGS += "-DG9991"
endif

ifneq ($(strip $(BRCM_BUILD_PWWRAP)),)
CFLAGS += "-DPWWRAP"
endif

ifneq ($(strip $(BUILD_CLI_CMD)), )
OBJS += cli_cmd.o cli_cmdwan.o \
       cli_cmddebug.o \
       cli_util.o \
       cli_osgid.o cli2_cmd.o cli2_util.o \
       cli_qos.o cli_intfGroup.o cli2_intfGroup.o

ifneq ($(strip $(BUILD_DM_TR104_V2)),)
    OBJS += cli2_voice.o cli2_dect.o
else
    OBJS += cli_voice.o cli_dect.o
endif
endif

ifneq ($(strip $(CMS_BYPASS_LOGIN)),)
CFLAGS += -DCMS_BYPASS_LOGIN
endif

CFLAGS += $(SSP_TYP_COMPILER_OPTS)
BCM_LD_FLAGS += -L$(INSTALL_DIR)/lib $(SSP_TYP_LIBS) -L$(INSTALL_DIR)/lib/public  -L$(CURR_DIR)/../wlcsm

OBJS := $(addprefix objs/$(PROFILE_ARCH)/,$(OBJS))
LIB := $(addprefix objs/$(PROFILE_ARCH)/,$(LIB))

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
                         -I$(BUILD_DIR)/userspace/private/libs/wlcsm/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR) \
                         -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)

ifneq ($(strip $(BUILD_OMCI)),)
ALLOWED_INCLUDE_PATHS += -I$(BCM_FSBUILD_DIR)/private/include
endif

# treat all warnings as errors
# temporarily disabling -  stack larger-than error (there is a bug in the 4.6 compiler with this)
#CUSTOM_CFLAGS += -Werror -Wfatal-errors



clean: generic_clean
	echo "objs is " $(OBJS)
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)
	rm -rf objs

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
objs/$(PROFILE_ARCH):
	mkdir -p objs/$(PROFILE_ARCH)

objs/$(PROFILE_ARCH)/%.o: %.c | objs/$(PROFILE_ARCH)
	$(CC) -MMD $(CPPFLAGS) $(CFLAGS)  -c -o $@ $<

$(notdir $(LIB)) : $(LIB)

PHONY: $(notdir $(LIB))

ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_CMS_CLI := 1
endif
ifneq ($(strip $(BUILD_BRCM_BDK)),)
COND_BUILD_CMS_CLI := 1
endif

ifeq ($(strip $(COND_BUILD_CMS_CLI)),1)

conditional_build: generic_private_lib_install

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive $(PMD_LIB_RPATH) $(LIBS)

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
