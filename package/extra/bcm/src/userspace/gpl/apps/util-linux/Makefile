util-linux: conditional_build 

# BRCM_SUPPORTS_MULTIARCH_BUILD

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX
export CFLAGS LDFLAGS
CFLAGS=-I$(BCM_FSBUILD_DIR)/public/include -I$(TOOLCHAIN_INCLUDE_DIR) -isystem $(TOOLCHAIN_INCLUDE_DIR) -I$(CURR_DIR)/$(APP)
LDFLAGS=-L$(BCM_FSBUILD_DIR)/public/lib -L$(BCM_FSINSTALL_DIR)/lib

COMPILE_OPTIONS=--disable-tls --without-tinfo --with-systemd=no --disable-wall --enable-libmount-force-mountinfo \
				--without-python --without-udev --disable-hwclock-gplv3 --with-bashcompletiondir=$(BCM_FSBUILD_DIR)/public/bashcompletion

INSTALL_OPTIONS=--program-suffix=".util-linux" --disable-makeinstall-setuid --disable-makeinstall-chown
APP=util-linux-2.39

ifneq ($(strip $(BUILD_SYSTEMD)),)
conditional_build: all
else
conditional_build:
	@echo "skipping util-linux (not configured)"
endif

$(APP)/configure:
	tar -xvzf $(APP).tar.gz
	patch -p1 -b -s -d$(APP) < $(APP).patch
	cd $(APP)
	touch -c $(APP)/configure
	@echo "util-linux is ready"

check_config: objs/$(PROFILE_ARCH)/Makefile

objs/$(PROFILE_ARCH)/Makefile: $(APP)/configure
	mkdir -p objs/$(PROFILE_ARCH)
	cd objs/$(PROFILE_ARCH) ; ac_cv_linux_vers=$(LINUX_VER_STR) ../../$(APP)/configure --host=$(TOOLCHAIN_PREFIX) --prefix=$(BCM_FSBUILD_DIR)/public $(COMPILE_OPTIONS) $(INSTALL_OPTIONS)

all: check_config
	$(MAKE) -C objs/$(PROFILE_ARCH)
	$(MAKE) -C objs/$(PROFILE_ARCH) install
	mkdir -p $(BCM_FSINSTALL_DIR)/lib
	mkdir -p $(BCM_FSINSTALL_DIR)/usr/sbin
	cp -av objs/$(PROFILE_ARCH)/.libs/lib*.so* $(BCM_FSINSTALL_DIR)/lib
	cp objs/$(PROFILE_ARCH)/agetty $(BCM_FSINSTALL_DIR)/usr/sbin/agetty.util-linux
	cp objs/$(PROFILE_ARCH)/sulogin $(BCM_FSINSTALL_DIR)/usr/sbin/sulogin.util-linux

clean:
	rm -rf objs
	rm -rf $(APP)

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i

