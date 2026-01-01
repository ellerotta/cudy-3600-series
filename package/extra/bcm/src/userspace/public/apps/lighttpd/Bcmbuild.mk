lighttpd: conditional_build
.PHONY: lighttpd

TAR := lighttpd-1.4.73.tar.gz

# BRCM_SUPPORTS_MULTIARCH_BUILD

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

CFLAGS += -I$(BCM_FSBUILD_DIR)/public/include \
		-I$(BCM_FSBUILD_DIR)/public/lib/include
LDFLAGS += -L$(BCM_FSBUILD_DIR)/public/lib \
		-L$(BCM_FSBUILD_DIR)/public/lib/lib

export LINUX_VER_STR TOOLCHAIN_PREFIX

ifneq ($(BUILD_LIGHTTPD),)
conditional_build: all install
else
conditional_build: ;
endif

lighttpd/configure: $(TAR)
	@mkdir -p lighttpd
	@cd lighttpd && tar --strip-components=1 -xkzf ../$(TAR)
	@touch -c $@

objs/$(PROFILE_ARCH)/Makefile: lighttpd/configure
	mkdir -p objs/$(PROFILE_ARCH)
	cd objs/$(PROFILE_ARCH) ; \
	../../lighttpd/configure \
		--host=$(TOOLCHAIN_PREFIX) \
		--prefix=/usr \
		--without-pcre \
		--without-pcre2 \
		--without-bzip2 \
		CFLAGS='$(CFLAGS)' LDFLAGS='$(LDFLAGS)' \
		;

install: all
	mkdir -p objs/$(PROFILE_ARCH)/install
	cd objs/$(PROFILE_ARCH) ; $(MAKE) install DESTDIR=`pwd`/install
	mkdir -p $(INSTALL_DIR)/usr/sbin
	cp -f objs/$(PROFILE_ARCH)/install/usr/sbin/* $(INSTALL_DIR)/usr/sbin/
	mkdir -p $(INSTALL_DIR)/usr/lib
	cp -f objs/$(PROFILE_ARCH)/install/usr/lib/* $(INSTALL_DIR)/usr/lib/
	mkdir -p $(INSTALL_DIR)/etc/lighttpd
	install -p -t $(INSTALL_DIR)/etc/lighttpd lighttpd.conf
	mkdir -p $(INSTALL_DIR)/etc/init.d
	install -p -m 755 scripts/lighttpd.sh $(INSTALL_DIR)/etc/init.d/lighttpd
	mkdir -p $(INSTALL_DIR)/etc/rc3.d
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f S80lighttpd; ln -s ../init.d/lighttpd S80lighttpd)

all: objs/$(PROFILE_ARCH)/Makefile
	@cd objs/$(PROFILE_ARCH) ; $(MAKE)

clean:
	@-[ ! -e objs/$(PROFILE_ARCH)/Makefile ] || make -C objs/$(PROFILE_ARCH) clean
	@rm -f $(INSTALL_DIR)/etc/init.d/lighttpd
	@rm -f $(INSTALL_DIR)/etc/rc3.d/S80lighttpd
	@rm -rf $(INSTALL_DIR)/etc/lighttpd
	@rm -rf objs lighttpd

bcm_dorel_distclean: distclean

distclean: clean

shell:
	bash -i

