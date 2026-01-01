LIB = eudev-3.2.11

all install: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_PIPEWIRE)),)
  conditional_build: 
	  @echo "Making $(LIB)"
	  $(MAKE) -f Makefile install INSTALL_DESTDIR=$(BCM_FSBUILD_DIR)/public
	  mkdir -p $(BCM_FSINSTALL_DIR)/lib
	  # The libudev code is under LGPL v2.1 license, all other
	  # components in eudev package is GPLv2.1, so be careful not
	  # included in the BCM_FSINSTALL_DIR/lib
	  cp -d $(BCM_FSBUILD_DIR)/public/lib/libudev.so* $(BCM_FSINSTALL_DIR)/lib/
else
  conditional_build:
	  @echo "$(LIB) not configured. Skipping."
endif

clean:
	$(MAKE) -f Makefile clean
	rm -f $(BCM_FSINSTALL_DIR)/lib/libudev.so* 

distclean: clean

bcm_dorel_distclean: distclean
