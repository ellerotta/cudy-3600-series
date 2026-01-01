APP := dhcpv6

ifeq ($(strip $(DESKTOP_LINUX)),y)
LDFLAGS += $(BCM_LD_FLAGS)
TOOLCHAIN_PREFIX=$(ARCH)-none-linux
endif

THIS_PLATFORM=$(shell uname -m)-linux-gnu

check_untar_configure:
	if [ ! -e $(APP)/$@ ]; then \
		rm -rf $(APP) ; \
		echo "Untarring source and overrides..." ; \
		mkdir -p $(APP) ; \
		(tar --strip-components=1 -xjf $(APP).tar.bz2 -C $(APP) 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		for i in patches/[0-9]*.patch ; do patch -p1 -b -s -d$(APP) < $$i ; done ; \
		cd $(APP); \
		./configure --build=$(THIS_PLATFORM) --host=$(TOOLCHAIN_PREFIX) --prefix=$(BCM_FSBUILD_DIR) \
		--sysconfdir=/etc --with-localdbdir=/var \
		ac_cv_func_setpgrp_void=yes ; \
		touch $@ ; \
	fi

build: check_untar_configure
	# avoid running bison twice in parallel
	$(MAKE) -C $(APP) cfparse.c
	$(MAKE) -C $(APP)

install: build
	$(MAKE) -C $(APP) install


clean:
	@if [ -e $(APP)/Makefile ]; then \
		cd $(APP); $(MAKE) uninstall ; \
	fi
	rm -rf $(APP)

# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
