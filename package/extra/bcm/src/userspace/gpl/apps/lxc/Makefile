# Makefile to build LXC

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

MESON=$(HOSTTOOLS_DIR)/meson/meson
NINJA=$(HOSTTOOLS_DIR)/ninja/ninja
CROSS_CONF=$(CURR_DIR)/cross.conf

all dynamic install:

include $(BUILD_DIR)/make.common

ALLOWED_LIB_DIRS := /lib:/public/lib

CFLAGS = -I$(BUILD_DIR)/userspace/public/include
IS_GCC47_ABOVE := 1
ifeq ($(strip $(DESKTOP_LINUX)),y)
MIN_GCC_VERSION = "8"
GCC_VERSION := "`$(CROSS_COMPILE)gcc -dumpversion`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
# lxc-3.1.0 required at least gcc 4.7. To skip the lxc build for Jenkins build server.
IS_GCC47_ABOVE := $(shell expr "$(GCC_VERSION)" ">=" "4.7")
CFLAGS += $(BCM_LD_FLAGS)
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
CFLAGS += -Wno-format-truncation -Wno-format-overflow
endif
endif

ifneq ($(strip $(BUILD_LIBSECCOMP)),)
SECCOMP_ENABLE = true
SECCOMP_CFLAGS = -I$(BCM_FSBUILD_DIR)/public/include
else
SECCOMP_ENABLE = false
SECCOMP_CFLAGS =
endif

ifneq ($(strip $(BUILD_APPARMOR)),)
APPARMOR_ENABLE = true
else
APPARMOR_ENABLE = false
endif


APP = lxc-5.0.3
START_LXC = S64lxc

ifeq "$(IS_GCC47_ABOVE)" "1"
ifneq ($(strip $(BUILD_LXC)),)
conditional_build: buildLxc
else
conditional_build: 
	@echo "skipping $(APP) (not configured)"
endif
else
conditional_build: 
	@echo "skipping $(APP) (DESKTOP_LINUX with old gcc)"
endif

$(CROSS_CONF):
	@echo "Generate cross conf file..."
	@echo "[binaries]" > $(CROSS_CONF)
	@echo "c = '$(CC)'" >> $(CROSS_CONF)
	@echo "cpp = '$(CXX)'" >> $(CROSS_CONF)
	@echo "ar = '$(AR)'" >> $(CROSS_CONF)
	@echo "strip = '$(STRIP)'" >> $(CROSS_CONF)
	@echo "pkgconfig = '/usr/bin/pkg-config'" >> $(CROSS_CONF)
	@echo "cmake = '/opt/toolchains/local/bin/cmake3'" >> $(CROSS_CONF)
	@echo -e '\n' >> $(CROSS_CONF)
	@echo "[host_machine]" >> $(CROSS_CONF)
	@echo "system = 'linux'" >> $(CROSS_CONF)
	@echo "cpu_family = '$(USER_ARCH)'" >> $(CROSS_CONF)
	@echo "cpu = '$(KERNEL_ARCH)'" >> $(CROSS_CONF)
	@echo "endian = 'little'" >> $(CROSS_CONF)
	touch -c $(CROSS_CONF)

$(APP)/meson.build: sanity_check
	if [ ! -e $(APP)/meson.build ]; then \
		echo "Untarring source and overrides..." ; \
		(tar xfz $(APP).tar.gz 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		patch -p1 -b -N -s -d$(APP) < $(APP).patch ; \
	fi
	
check_config:$(APP)/meson.build $(CROSS_CONF)

Meson:check_config
	touch -c $(APP)/meson.build;
	
	cd $(APP); \
		export PATH=$(PATH):$(HOSTTOOLS_DIR)/ninja; $(MESON) setup --cross-file $(CROSS_CONF) -Dprefix=/ \
		-Dman=false -Dexamples=false -Dcommands=true -Dtools=true -Db_pie=false -Dinit-script=sysvinit -Dcapabilities=true -Dselinux=false -Dopenssl=false -Dtests=false -Dseccomp=$(SECCOMP_ENABLE) -Dapparmor=$(APPARMOR_ENABLE) \
		-Dc_args='$(CFLAGS) $(SECCOMP_CFLAGS)' -Dpkg_config_path='$(BCM_FSBUILD_DIR)/public/lib/pkgconfig:$(BCM_FSBUILD_DIR)/gpl/lib/pkgconfig' \
		-Dc_link_args='-L$(BCM_FSBUILD_DIR)/public/lib -L$(BCM_FSBUILD_DIR)/gpl/lib $(BCM_LD_FLAGS)' build

# Note: For 64-bit userspace, lxc still looks for directory /lib/lxc/rootfs
# instead of /lib64/lxc/rootfs. This directory must exist, even though it
# may be empty. It is used to temporary mount the rootfs of lxc in a private
# mount namespace only visible by the processes running in the container.
# Make the directory under $(BCM_FSINSTALL_DIR)/lib/arm so that it can be copied
# over to $ROOTFS/lib by buildFS.
#

buildLxc: Meson
	@echo "Making $(APP)"
	@cd $(APP); \
	export PATH=$(PATH):$(HOSTTOOLS_DIR)/ninja; $(MAKE) &&  \
	$(MAKE) install DESTDIR=$(BCM_FSBUILD_DIR)
	cp -p $(BCM_FSBUILD_DIR)/bin/lxc-* $(BCM_FSINSTALL_DIR)/bin/.
	mkdir -p $(BCM_FSINSTALL_DIR)/etc/init.d
	mkdir -p $(BCM_FSINSTALL_DIR)/etc/rc3.d
ifeq ($(strip $(PROFILE_ARCH)),aarch64)
	mkdir -p $(BCM_FSINSTALL_DIR)/lib/arm/lxc/rootfs
endif
	rm -f $(BCM_FSINSTALL_DIR)/run
	ln -s /var/run $(BCM_FSINSTALL_DIR)/run
	mkdir -p $(BCM_FSINSTALL_DIR)/etc/lxc
	cp -p $(BCM_FSBUILD_DIR)/etc/lxc/* $(BCM_FSINSTALL_DIR)/etc/lxc/.
ifneq ($(strip $(BUILD_APPARMOR)),)
	mkdir -p $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d/abstractions
	if [  -e $(BCM_FSBUILD_DIR)/etc/apparmor.d ]; then \
		cp -rf $(BCM_FSBUILD_DIR)/etc/apparmor.d/lxc $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d/.; \
		cp -rf $(BCM_FSBUILD_DIR)/etc/apparmor.d/abstractions/lxc $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d/abstractions/.; \
		cp -rf $(BCM_FSBUILD_DIR)/etc/apparmor.d/lxc-containers $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d/.; \
		cp -p $(BCM_FSBUILD_DIR)/etc/apparmor.d/usr.bin.lxc-start $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d/.; \
	fi
endif
	cp -rf $(BCM_FSBUILD_DIR)/libexec $(BCM_FSINSTALL_DIR)/.
	cp -P $(BCM_FSBUILD_DIR)/lib/liblxc.so* $(BCM_FSINSTALL_DIR)/lib/.
	cp -rf $(BCM_FSBUILD_DIR)/lib/lxc $(BCM_FSINSTALL_DIR)/lib/.
	cp -p $(BCM_FSBUILD_DIR)/sbin/init.lxc $(BCM_FSINSTALL_DIR)/sbin/.
	install -m 0755 lxc.sh $(BCM_FSINSTALL_DIR)/etc/init.d
	(cd $(BCM_FSINSTALL_DIR)/etc/rc3.d; rm -f $(START_LXC); ln -s ../init.d/lxc.sh $(START_LXC))



# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)
	rm -f cross.conf
	@rm -f $(BCM_FSINSTALL_DIR)/etc/init.d/lxc.sh
	@rm -f $(BCM_FSINSTALL_DIR)/etc/rc3.d/$(START_LXC)
	@rm -f $(BCM_FSINSTALL_DIR)/run
	rm -f $(BCM_FSINSTALL_DIR)/lib/liblxc.so*
	rm -rf $(BCM_FSINSTALL_DIR)/lib/lxc
	rm -rf $(BCM_FSINSTALL_DIR)/libexec
	rm -rf $(BCM_FSINSTALL_DIR)/etc/libexec
	rm -rf $(BCM_FSINSTALL_DIR)/etc/lxc
	rm -rf $(BCM_FSINSTALL_DIR)/etc/lxcapparmor.d
	rm -f $(BCM_FSINSTALL_DIR)/bin/lxc-*
	rm -f $(BCM_FSINSTALL_DIR)/sbin/init.lxc
	rm -f $(BCM_FSBUILD_DIR)/lib/liblxc.so*
	rm -rf $(BCM_FSBUILD_DIR)/lib/lxc
	rm -rf $(BCM_FSBUILD_DIR)/libexec
	rm -rf $(BCM_FSBUILD_DIR)/etc/apparmor.d
	rm -rf $(BCM_FSBUILD_DIR)/etc/libexec
	rm -rf $(BCM_FSBUILD_DIR)/etc/lxc
	rm -f $(BCM_FSBUILD_DIR)/bin/lxc-*
	rm -f $(BCM_FSBUILD_DIR)/sbin/init.lxc
	rm -f $(BCM_FSBUILD_DIR)/sbin/init.lxc.static

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean
	
bcm_dorel_distclean: distclean
