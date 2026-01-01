LIB := cjson

all install: conditional_build


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


ARCH=$(PROFILE_ARCH)
ALLOWED_INCLUDE_PATHS := -I. \
                         -I$(BCM_FSBUILD_DIR)/public/include

PREFIX := $(INSTALL_DIR)/usr
LUA_VERSION := ""
# Final location of LIB for system image.  Only the BRCM build system needs to
# know about this.
FINAL_LIB_INSTALL_DIR := $(INSTALL_DIR)/usr/lib/lua

export ARCH CFLAGS PREFIX LUA_VERSION

ifneq ($(strip $(BUILD_LUACJSON)),)
conditional_build:
	$(MAKE) -f Makefile install CFLAGS="$(CFLAGS)" 
else
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif

clean:
	-$(MAKE) -f Makefile clean

distclean: clean
	-$(MAKE) -f Makefile distclean

shell:
	bash -i
