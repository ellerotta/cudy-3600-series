
libtirpc: conditional_build 

# BRCM_SUPPORTS_MULTIARCH_BUILD

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

export LINUX_VER_STR TOOLCHAIN_PREFIX

ifeq ($(strip $(DESKTOP_LINUX)),y)
CFLAGS = $(BCM_LD_FLAGS)
LDFLAGS = $(BCM_LD_FLAGS)
export CFLAGS LDFLAGS
ifeq "$(BCM_LD_FLAGS)" "-m32"
BCM_HOST := i386-linux
else
BCM_HOST := x86_64-linux
endif
else
BCM_HOST := $(TOOLCHAIN_PREFIX)
endif

#ifneq ($(strip $(BUILD_LIBTIRPC)),)
conditional_build: all
#else
#conditional_build:
#	@echo "skipping libtirpc (not configured)"
#endif

LIBNAME=libtirpc-1.3.4
$(LIBNAME)/configure: $(LIBNAME).tar.bz2
	tar -xjf $(LIBNAME).tar.bz2
	touch -c $(LIBNAME)/configure
	@echo "$(LIBNAME) is untarred"

check_config: objs/$(PROFILE_ARCH)/Makefile

objs/$(PROFILE_ARCH)/Makefile: $(LIBNAME)/configure
	mkdir -p objs/$(PROFILE_ARCH)
	cd objs/$(PROFILE_ARCH) ; ac_cv_linux_vers=$(LINUX_VER_STR)  ../../$(LIBNAME)/configure --host=$(BCM_HOST) --prefix=$(BCM_FSBUILD_DIR)/public/ --disable-gssapi

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cd objs/$(PROFILE_ARCH) ; $(MAKE)
	cd objs/$(PROFILE_ARCH) ; $(MAKE) install
	mkdir -p  $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libtirpc.so* $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	#cp -d $(BCM_FSBUILD_DIR)/public/lib/libtirpc.a $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	cp -d $(BCM_FSBUILD_DIR)/public/etc/netconfig $(INSTALL_DIR)/etc
	#rm -f $(BCM_FSBUILD_DIR)/public/include/rpc
	#ln -s $(BCM_FSBUILD_DIR)/public/include/tirpc/rpc $(BCM_FSBUILD_DIR)/public/include/rpc

clean:
	rm -f $(INSTALL_DIR)/lib/public/libtirpc.so*
	rm -f $(INSTALL_DIR)/lib/public/libtirpc.a
	#rm -f $(BCM_FSBUILD_DIR)/public/include/rpc
	-[ ! -e objs/$(PROFILE_ARCH)/Makefile ] || $(MAKE) -C objs/$(PROFILE_ARCH) uninstall 
	rm -rf objs
	-rm -rf $(LIBNAME)

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i

