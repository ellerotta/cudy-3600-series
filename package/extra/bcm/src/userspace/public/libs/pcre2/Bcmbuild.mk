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

PREFIX = $(BCM_FSBUILD_DIR)/public
LIB_INSTALL_DIR := $(BCM_FSBUILD_DIR)/public/lib

APP = pcre2-10.37
LIB = libpcre2

# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)

.PHONY: conditional_build 

ifneq ($(strip $(BUILD_LIBPCRE2)),)
conditional_build: 
	$(MAKE) -f Makefile install
	mkdir -p $(FINAL_LIB_INSTALL_DIR)
	cp -d $(LIB_INSTALL_DIR)/$(LIB)*.so* $(FINAL_LIB_INSTALL_DIR)
else
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)*.so*
	$(MAKE) -f Makefile clean

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
bcm_dorel_distclean: distclean

