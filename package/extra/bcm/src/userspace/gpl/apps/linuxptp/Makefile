#***********************************************************************
#
#  Copyright (c) 2017  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

all dynamic install: conditional_build

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

# Actually, this app does not include anything from the fs.build dirs, but I
# am leaving a good example here in case we need to include from there in
# the future.  Note this is a GPL app, so must NOT include any headers from
# private/include and must NOT link with any Broadcom proprietary libs.
EXTRA_CFLAGS += -I${BCM_FSBUILD_DIR}/public/include -I${BCM_FSBUILD_DIR}/bcmdrivers/include
export EXTRA_CFLAGS

KBUILD_OUTPUT:=$(TOOLCHAIN_INCLUDE_DIR)/../../
export KBUILD_OUTPUT

APP = linuxptp-3.1.1

.PHONY: check_untar_patch_configure conditional_build 

check_untar_patch_configure: 
	@if [ ! -e $(APP) ]; then \
	    echo "Untarring original $(APP) source"; \
	    (tar xkzf $(APP).tgz 2> /dev/null || true); \
	fi

conditional_build: check_untar_patch_configure
	$(MAKE) -C $(APP)/ ptp4l
	install -m 755 $(APP)/ptp4l $(INSTALL_DIR)/bin
	install -m 755 brcm.cfg $(INSTALL_DIR)/bin


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

