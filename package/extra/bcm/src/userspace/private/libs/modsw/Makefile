#***********************************************************************
#
#  Copyright (c) 2012  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

#
# In most cases, you only need to modify this first section.
#
LIB = libmodsw.so

FULL_SOURCE_OBJS = modsw_common.o modsw_ee.o

BINARY_ONLY_OBJS = modsw_bee.o

OBJS = $(FULL_SOURCE_OBJS) $(BINARY_ONLY_OBJS)

all dynamic install: conditional_build

clean: generic_clean
	rm -f $(INSTALL_DIR)/lib/private/$(LIB)

HAVESRC := $(wildcard modsw_bee.c)


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
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)

ALLOWED_LIB_DIRS := /lib:/lib/public

LIB_PATHS = $(patsubst %,-L$(INSTALL_DIR)%,$(subst :, ,$(ALLOWED_LIB_DIRS)))

LIBS   = -L$(BCM_FSBUILD_DIR)/private/lib -lbeeputil

CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include

# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors -Wno-switch


ifneq ($(strip $(BUILD_LIBMODSW)),)
COND_BUILD_LIBMODSW := 1
endif

ifeq ($(strip $(COND_BUILD_LIBMODSW)),1)
conditional_build: generic_private_lib_install

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive $(LIB_PATHS) $(LIBS)


#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

ifeq ($(strip $(HAVESRC)),)

modsw_bee.o: modsw_bee.o.$(PROFILE_ARCH).save
	cat $< > $@

-include $(FULL_SOURCE_OBJS:.o=.d)

else
-include $(OBJS:.o=.d)
endif
	
else
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif
