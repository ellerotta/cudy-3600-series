
libpcap: conditional_build 

LIBPCAP_TAR := libpcap-1.10.4.tar.gz
APP = libpcap

# BRCM_SUPPORTS_MULTIARCH_BUILD

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
ifeq "$(BCM_LD_FLAGS)" "-m32"
BCM_HOST := i386-linux
else
BCM_HOST := $(TOOLCHAIN_PREFIX)
endif
else
BCM_HOST := $(TOOLCHAIN_PREFIX)
endif

export LINUX_VER_STR TOOLCHAIN_PREFIX


ifneq ($(strip $(BUILD_LIBPCAP)),)
conditional_build: all
else
conditional_build:
	@echo "skipping libpcap (not configured)"
endif


$(APP)/configure: $(LIBPCAP_TAR)
	mkdir -p $(APP); cd $(APP); tar --strip-components=1 -xzf ../$(LIBPCAP_TAR); patch -p1 -b -s < ../remove_rpath.patch
	@echo "libpcap is untarred"

check_config: objs/$(PROFILE_ARCH)/Makefile

objs/$(PROFILE_ARCH)/Makefile: $(APP)/configure
	mkdir -p objs/$(PROFILE_ARCH)
	cd objs/$(PROFILE_ARCH) ; export PKG_CONFIG=$(TOOLCHAIN_TOP)/$(TOOLCHAIN_USR_DIR)/bin/pkg-config; ac_cv_linux_vers=$(LINUX_VER_STR)  ../../$(APP)/configure --host=$(BCM_HOST) --target=$(TOOLCHAIN_PREFIX) --with-pcap=linux --prefix=$(BCM_FSBUILD_DIR)/public/ --without-dag --without-libnl --without-dpdk --enable-remote

all: check_config
	mkdir -p $(INSTALL_DIR)/lib/public/
	cd objs/$(PROFILE_ARCH) ; $(MAKE) CFLAGS="$(CFLAGS)" LDFLAGS="$(BCM_LD_FLAGS)"
	cd objs/$(PROFILE_ARCH) ; $(MAKE) install
	mkdir -p  $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	mkdir -p  $(INSTALL_DIR)/sbin/
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libpcap.so* $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	$(INSTALL) $(BCM_FSBUILD_DIR)/public/sbin/rpcapd $(INSTALL_DIR)/sbin/

clean:
	rm -f $(INSTALL_DIR)/lib/public/libpcap.so*
	-[ ! -e objs/$(PROFILE_ARCH)/Makefile ] || $(MAKE) -C objs/$(PROFILE_ARCH) uninstall 
	rm -rf objs
	rm -rf $(APP)

bcm_dorel_distclean: distclean

distclean:
	rm -rf $(APP) objs

shell:
	bash -i

