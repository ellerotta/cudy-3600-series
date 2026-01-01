LIB = libsndfile-1.1.0

all install: conditional_build 

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_PIPEWIRE)),)
  conditional_build: 
	  @echo "Making $(LIB)"
	  $(MAKE) -f Makefile install
	  mkdir -p $(BCM_FSINSTALL_DIR)/lib
	  cp -d $(BCM_FSBUILD_DIR)/public/lib/libsndfile.so* $(BCM_FSINSTALL_DIR)/lib/
else
  conditional_build:
	  @echo "$(LIB) not configured. Skipping."
endif

clean:
	$(MAKE) -f Makefile clean

distclean: clean

bcm_dorel_distclean: distclean
