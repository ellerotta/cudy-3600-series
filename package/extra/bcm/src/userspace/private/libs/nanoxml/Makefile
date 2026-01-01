#***********************************************************************
#
#  Copyright (c) 2006-2007  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

#
# TODO: split into Bcmbuild.mk, Makefile, and Makefile.fullsrc, and build in obj subdir
#
LIB = libnanoxml.so

OBJS = nanoxml.o

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
                         -I$(BUILD_DIR)/userspace/private/include


ifneq ($(strip $(BUILD_BRCM_CMS)),)
COND_BUILD_NANOXML := 1
endif
ifneq ($(strip $(BUILD_DISTRIBUTED_MDM)),)
COND_BUILD_NANOXML := 1
endif

ifeq ($(strip $(COND_BUILD_NANOXML)),1)

conditional_build: generic_private_lib_install

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,-soname,$(notdir $@) -o $@ $(OBJS)

else

conditional_build:
	@echo "Skipping $(LIB) (not configured)"

endif


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
