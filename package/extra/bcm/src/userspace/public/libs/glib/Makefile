
LIB = glib-2.76.6
VERSION_MAJOR = 2.0

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

PREFIX = $(BCM_FSBUILD_DIR)/public

GBINDIR=$(INSTALL_DIR)/usr/bin
GLIBDIR=$(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
GSHAREDIR=$(INSTALL_DIR)/share

INSTALL_LIBS_PREFIX := libglib libgmodule libgthread libgobject libgio
INSTALL_BINS := glib-gettextize gtester gtester-report gobject-query glib-genmarshal glib-mkenums \
                gdbus-codegen gio-querymodules glib-compile-schemas glib-compile-resources gsettings \
                gdbus gapplication gresource

all dynamic install: conditional_build

include $(BUILD_DIR)/make.common

ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS := $(BCM_LD_FLAGS)
endif

MESON=$(HOSTTOOLS_DIR)/meson/meson
NINJA=$(HOSTTOOLS_DIR)/ninja/ninja
CROSS_CONF=$(CURR_DIR)/cross.conf
PREFIX = $(BCM_FSBUILD_DIR)/public

GLIB_MESON_OPTIONS=-Dprefix=$(PREFIX)/ -Dlibmount=disabled -Druntime_dir=/var/run -Dc_args='-I$(BCM_FSBUILD_DIR)/public/include -I$(BCM_FSBUILD_DIR)/public/usr/include' -Dc_link_args='-L$(BCM_FSBUILD_DIR)/public/lib -L$(BCM_FSBUILD_DIR)/gpl/lib -L$(INSTALL_DIR)/lib'

ifneq ($(strip $(BUILD_GLIB)),)

ifeq ($(strip $(DESKTOP_LINUX)),y)
conditional_build: desktop_build
else
conditional_build: build
endif

else
conditional_build: 
	@echo "skipping $(LIB) (not configured)"
endif

check_config:$(LIB)/meson.build $(CROSS_CONF)

$(LIB)/meson.build:
	(tar xfJ $(LIB).tar.xz 2> /dev/null || true); \
	touch -c $(LIB)/meson.build

$(CROSS_CONF):
	@echo "Generate cross conf file..."
	@echo "[binaries]" > $(CROSS_CONF)
	@echo "c = '$(CC)'" >> $(CROSS_CONF)
	@echo "cpp = '$(CXX)'" >> $(CROSS_CONF)
	@echo "ar = '$(AR)'" >> $(CROSS_CONF)
	@echo "strip = '$(STRIP)'" >> $(CROSS_CONF)
	@echo "pkgconfig = '/usr/bin/pkg-config'" >> $(CROSS_CONF)
	@echo -e '\n' >> $(CROSS_CONF)
	@echo "[host_machine]" >> $(CROSS_CONF)
	@echo "system = 'linux'" >> $(CROSS_CONF)
	@echo "cpu_family = '$(USER_ARCH)'" >> $(CROSS_CONF)
	@echo "cpu = '$(KERNEL_ARCH)'" >> $(CROSS_CONF)
	@echo "endian = 'little'" >> $(CROSS_CONF)
	touch -c $(CROSS_CONF)

Meson:check_config
	export PKG_CONFIG_LIBDIR=$(PREFIX)/lib:$(PREFIX)/lib/lib; \
	export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig:$(PREFIX)/lib/lib/pkgconfig; \
	export CFLAGS="$(BCM_BLT32_FLAGS) -g"\ -O2\ -Wno-strict-aliasing\ -Wno-error=unused-parameter; \
	cd $(LIB); \
	export PATH=$(PATH):$(HOSTTOOLS_DIR)/ninja;\
	$(MESON) setup --cross-file $(CROSS_CONF) $(GLIB_MESON_OPTIONS) _build	 

build:Meson
	$(NINJA) -C $(LIB)/_build
	$(NINJA) -C $(LIB)/_build install
	
	mkdir -p $(GLIBDIR)
	for i in $(INSTALL_LIBS_PREFIX) ; do \
	    cp -d $(PREFIX)/lib/$$i-$(VERSION_MAJOR).so* $(GLIBDIR) ; \
	done
	mkdir -p $(GBINDIR)
	for i in $(INSTALL_BINS) ; do \
	    cp -d $(PREFIX)/bin/$$i $(GBINDIR) ; \
	done  

desktop_build:
	(tar xfJ $(LIB).tar.xz 2> /dev/null || true); \
	touch -c $(LIB)/meson.build \
	export PKG_CONFIG_LIBDIR=$(PREFIX)/lib:$(PREFIX)/lib/lib; \
	export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig:$(PREFIX)/lib/lib/pkgconfig; \
	export CFLAGS="$(BCM_BLT32_FLAGS) -g"\ -O2\ -Wno-strict-aliasing\ -Wno-error=unused-parameter; \
	cd $(LIB); \
	export PATH=$(PATH):$(HOSTTOOLS_DIR)/ninja;\
	$(MESON) setup -Dprefix=$(PREFIX)/ -Dlibdir=lib/ -Dpkg_config_path='$(BCM_FSBUILD_DIR)/public/lib/pkgconfig:$(BCM_FSBUILD_DIR)/public/lib/lib/pkgconfig' -Dtests=false -Dselinux=disabled -Dlibelf=disabled  -Dlibmount=disabled _build
	
	$(NINJA) -C $(LIB)/_build
	$(NINJA) -C $(LIB)/_build install
	
	mkdir -p $(GLIBDIR)
	for i in $(INSTALL_LIBS_PREFIX) ; do \
	    cp -d $(PREFIX)/lib/$$i-$(VERSION_MAJOR).so* $(GLIBDIR) ; \
	done
	mkdir -p $(GBINDIR)
	for i in $(INSTALL_BINS) ; do \
	    cp -d $(PREFIX)/bin/$$i $(GBINDIR) ; \
	done

clean:
	rm -rf meson-info; rm -rf meson-logs; rm -rf meson-private
	rm -rf $(LIB)
	rm -rf $(CROSS_CONF)
	
	for i in $(INSTALL_BINS) ; do \
	    rm -rf $(GBINDIR)/$$i ; \
	done
	for i in $(INSTALL_LIBS_PREFIX) ; do \
	    rm -rf $(GLIBDIR)/$$i-$(VERSION_MAJOR).so* ; \
	done

# The next line is a hint to to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

.PHONY: build check_untar_configure_patch sanity_check conditional_build

