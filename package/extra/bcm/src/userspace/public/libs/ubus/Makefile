LIB = ubus-2021-06-30-4fc532c8
CMAKE_BUILD_DIR = ubus-build

all dynamic install: conditional_build

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

BINDIR=$(INSTALL_DIR)/bin
SBINDIR=$(INSTALL_DIR)/sbin

clean:
	-[ ! -e $(LIB) ] || $(MAKE) -C $(CMAKE_BUILD_DIR) clean;
	rm -rf $(BINDIR)/ubus*
	rm -rf $(SBINDIR)/ubus*
	rm -rf $(INSTALL_DIR)/lib/libubus*
	rm -rf $(BCM_FSBUILD_DIR)/public/lib/libubus*
	rm -rf $(BCM_FSBUILD_DIR)/public/bin/ubus
	rm -rf $(BCM_FSBUILD_DIR)/public/sbin/ubusd
	rm -rf $(CURR_DIR)/$(CMAKE_BUILD_DIR)
	rm -rf $(LIB)
	rm -f $(INSTALL_DIR)/etc/init.d/ubus.sh
	rm -f $(INSTALL_DIR)/etc/rc3.d/S62ubus


# The next line is a hint to to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

CFLAGS=-I$(BCM_FSBUILD_DIR)/public/include -I$(BCM_FSBUILD_DIR)/public/include/json-c -I$(BCM_FSBUILD_DIR)/public/include/libubox
ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS := $(BCM_LD_FLAGS)
CFLAGS+=$(BCM_BLT32_FLAGS)
endif

ifeq ($(strip $(BUILD_LIBJSONC)),y)
LDFLAGS+=-L$(INSTALL_DIR)/lib -ljson-c
endif

ifeq ($(strip $(BUILD_LIBUBOX)),y)
LDFLAGS+=-L$(INSTALL_DIR)/lib -lubox -lblobmsg_json -ljson_script
endif

ifeq ($(strip $(BUILD_OPENSYNC_GW)),y)
LUAPATH := $(INSTALL_DIR)/usr/lib/lua
CMAKE_LUA_FLAGS := -DBUILD_LUA=on -DLUAPATH=$(LUAPATH)
else
CMAKE_LUA_FLAGS := -DBUILD_LUA=off
endif

.PHONY: build check_untar_cmake sanity_check conditional_build

ifneq ($(strip $(BUILD_UBUS)),)
conditional_build: build
else ifneq ($(strip $(BCM_COND_HAVE_UBUS)),)
conditional_build:
	@echo "$(LIB) will be provided by build helper (not built by native BDK build system)"
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif

check_untar_cmake: sanity_check
	@echo $(TOOLCHAIN_PREFIX)
	echo "Untarring original $(LIB) source and configure";
	(tar xfJ $(LIB).tar.xz 2> /dev/null || true);
	for i in bcm_patches/[0-9]*.patch ; do echo "Applying $$i" ; patch -p1 -b -s -d$(LIB) < $$i ; done
	mkdir -p $(CURR_DIR)/$(CMAKE_BUILD_DIR)
	export PKG_CONFIG_LIBDIR=$(BCM_FSBUILD_DIR)/public/lib
	export PKG_CONFIG_PATH=$(BCM_FSBUILD_DIR)/public/lib/pkgconfig
	cd $(CURR_DIR)/$(CMAKE_BUILD_DIR); \
	CFLAGS="$(CFLAGS)" \
	cmake $(CMAKE_LUA_FLAGS) -DENABLE_SYSTEMD=off \
	-DCMAKE_RANLIB=$(RANLIB) \
	-DCMAKE_STRIP=$(STRIP) \
	-DCMAKE_NM:FILEPATH=$(NM) \
	-DCMAKE_OBJCOPY:FILEPATH=$(OBJCOPY) \
	-DCMAKE_OBJDUMP=$(OBJDUMP) \
	-DCMAKE_LINKER:FILEPATH="$(LD)" \
	-DCMAKE_AR:FILEPATH=$(AR) \
	-DCMAKE_FIND_ROOT_PATH:PATH="$(BCM_FSBUILD_DIR);$(BCM_FSBUILD_DIR)/public" \
	-DCMAKE_PREFIX_PATH:PATH="$(BCM_FSBUILD_DIR)/public;$(BCM_FSBUILD_DIR)/public/include;$(BCM_FSBUILD_DIR)/public/lib;$(BCM_FSBUILD_DIR)/public/lib/libubox" \
	-DCMAKE_LIBRARY_PATH:PATH="$(BCM_FSBUILD_DIR)/public/lib" \
	-DCMAKE_INSTALL_PREFIX:PATH=$(BCM_FSBUILD_DIR)/public \
	$(CURR_DIR)/$(LIB)


build: check_untar_cmake
	@echo "Building $(LIB)"
	cd $(CURR_DIR)/$(CMAKE_BUILD_DIR); $(MAKE)
	cd $(CURR_DIR)/$(CMAKE_BUILD_DIR); $(MAKE) install
	#libubus.so ubusd ubus
	mkdir -p $(INSTALL_DIR)/etc/init.d
	mkdir -p $(INSTALL_DIR)/etc/rc3.d
	mkdir -p $(SBINDIR)
	mkdir -p $(BINDIR)
	chmod 755 $(BCM_FSBUILD_DIR)/public/lib/libubus.so;
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libubus.* $(INSTALL_DIR)/lib/
	cp -d $(BCM_FSBUILD_DIR)/public/sbin/ubusd $(SBINDIR)/
	cp -d $(BCM_FSBUILD_DIR)/public/bin/ubus $(BINDIR)/
	echo "install BRCM ubus script"
	install -m 0755 scripts/ubus.sh $(INSTALL_DIR)/etc/init.d
	(cd $(INSTALL_DIR)/etc/rc3.d; rm -f S62ubus; ln -s ../init.d/ubus.sh S62ubus)

