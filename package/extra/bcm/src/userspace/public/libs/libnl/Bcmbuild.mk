libnl: conditional_build

LIBNL_VER := 3.5.0
LIBNL_TAR := libnl-$(LIBNL_VER).tar.gz
LIBNL_DIR := libnl
LIBNL_PATCH = libnl-$(LIBNL_VER).patch

# BRCM_SUPPORTS_MULTIARCH_BUILD

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX


ifneq ($(strip $(BUILD_LIBNL)),)
conditional_build: all
else
conditional_build: ;
endif

libnl/configure: $(LIBNL_TAR)
	@mkdir -p libnl
	@cd libnl && tar --strip-components=1 -xkzf ../$(LIBNL_TAR)
	@-patch -p1 -b -s -d$(LIBNL_DIR) < $(LIBNL_PATCH)
	@touch -c $@

objs/$(PROFILE_ARCH)/Makefile: libnl/configure
	mkdir -p objs/$(PROFILE_ARCH)
	cd objs/$(PROFILE_ARCH) ; \
		../../libnl/configure \
			--host=$(TOOLCHAIN_PREFIX) \
			--prefix=$(BCM_FSBUILD_DIR)/public/ \
			--disable-static \
			--disable-cli \
			;

all: objs/$(PROFILE_ARCH)/Makefile
	@mkdir -p $(INSTALL_DIR)/lib/
	@cd objs/$(PROFILE_ARCH) ; $(MAKE)
	@cd objs/$(PROFILE_ARCH) ; $(MAKE) install
	@cd objs/$(PROFILE_ARCH) ; cp -u lib/defs.h $(BCM_FSBUILD_DIR)/public/include/libnl3/
	@mkdir -p  $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	@cp -du $(BCM_FSBUILD_DIR)/public/lib/libnl*.so* $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)

clean:
	@rm -f $(INSTALL_DIR)/lib/libnl*.so*
	[ ! -e objs/$(PROFILE_ARCH)/Makefile ] || make -C objs/$(PROFILE_ARCH) uninstall clean
	rm -rf objs libnl

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i

