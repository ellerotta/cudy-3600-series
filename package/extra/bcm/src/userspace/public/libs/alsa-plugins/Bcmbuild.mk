LIB = alsa-plugins-1.2.1

all install: conditional_build   

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

export PKG_CONFIG_PATH=$(BCM_FSBUILD_DIR)/public/lib/pkgconfig
export PKG_CONFIG_LIBDIR=$(BCM_FSBUILD_DIR)/public/lib
export LDFLAGS=-L$(BCM_FSBUILD_DIR)/public/lib
export CFLAGS=-I$(BCM_FSBUILD_DIR)/public/include

ifneq ($(strip $(BUILD_BCM_ASOC_AUDIO)),)
  conditional_build:
	  @echo "Making $(LIB)"
	  mkdir -p $(BCM_FSBUILD_DIR)/public/lib/alsa-lib
	  $(MAKE) -f Makefile install
	  mkdir -p $(BCM_FSINSTALL_DIR)/lib
	  mkdir -p $(BCM_FSINSTALL_DIR)/lib/alsa-lib
	  cp -d $(BCM_FSBUILD_DIR)/public/lib/alsa-lib/libasound_module_rate_samplerate*.so* $(BCM_FSINSTALL_DIR)/lib/alsa-lib
else
  conditional_build:
	  @echo "alsa-plugins not configured. Skipping."
endif

clean:
	$(MAKE) -f Makefile clean
	rm -rf $(BCM_FSINSTALL_DIR)/lib/alsa-lib/libasound_module_rate_samplerate*.so*

distclean: clean

bcm_dorel_distclean: distclean
