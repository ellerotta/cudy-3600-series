# Makefile to build odhcp6c

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

all dynamic install:

include $(BUILD_DIR)/make.common

export CFLAGS
export LDFLAGS

APP = odhcp6c-2015-07-29
APP_ARCHIVE = $(APP).tar.bz2
APP_BIN = odhcp6c

.PHONY: conditional_build 

ifneq ($(strip $(BUILD_ODHCP6C)),)
conditional_build:
	@echo "Making $(APP)"
	$(MAKE) -f Makefile install
	mkdir -p $(INSTALL_DIR)/usr/sbin && \
	$(INSTALL) -m 755 $(APP)/$(APP_BIN) $(INSTALL_DIR)/usr/sbin/$(APP_BIN) && \
	$(STRIP) $(INSTALL_DIR)/usr/sbin/$(APP_BIN)
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	$(MAKE) -f Makefile clean
	@if [ -e  $(INSTALL_DIR)/usr/sbin/$(APP_BIN) ]; then \
		rm -f $(INSTALL_DIR)/usr/sbin/$(APP_BIN); \
	fi;

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean
