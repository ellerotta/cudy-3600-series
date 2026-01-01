all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
export CFLAGS
LDFLAGS = $(BCM_LD_FLAGS)
export LDFLAGS
endif

APP = tinyproxy-1.10.0

.PHONY: conditional_build 

conditional_build:
	@echo "Making $(APP)"
	$(MAKE) -f Makefile install
	mkdir -p $(INSTALL_DIR)/usr/sbin; \
	$(INSTALL) -m 755 $(APP)/src/tinyproxy $(INSTALL_DIR)/usr/sbin/tinyproxy
	$(STRIP) $(INSTALL_DIR)/usr/sbin/tinyproxy

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	$(MAKE) -f Makefile clean
	rm -f $(INSTALL_DIR)/usr/sbin/tinyproxy

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

