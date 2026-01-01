APP := apparmor-v3.1.3

all install: conditional_build 


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


IS_GCC50_ABOVE := 1
ifeq ($(strip $(DESKTOP_LINUX)),y)
MIN_GCC_VERSION = "8"
GCC_VERSION := "`$(CROSS_COMPILE)gcc -dumpversion`"
IS_GCC_ABOVE_MIN_VERSION := $(shell expr "$(GCC_VERSION)" ">=" "$(MIN_GCC_VERSION)")
IS_GCC50_ABOVE := $(shell expr "$(GCC_VERSION)" ">=" "5.0")
CFLAGS += $(BCM_LD_FLAGS)
ifeq "$(IS_GCC_ABOVE_MIN_VERSION)" "1"
CFLAGS += -Wno-format-truncation -Wno-format-overflow
endif
endif

export CFLAGS APP


ifeq "$(IS_GCC50_ABOVE)" "1"
ifneq ($(strip $(BUILD_APPARMOR)),)
conditional_build:
	$(MAKE) -f Makefile install
	cp -P $(BCM_FSBUILD_DIR)/gpl/lib/libapparmor.so* $(BCM_FSINSTALL_DIR)/lib/.
	mkdir -p $(BCM_FSINSTALL_DIR)/usr/bin
	mkdir -p $(BCM_FSINSTALL_DIR)/usr/sbin
	mkdir -p $(BCM_FSINSTALL_DIR)/sbin
	cp -p $(BCM_FSBUILD_DIR)/gpl/usr/bin/aa-* $(BCM_FSINSTALL_DIR)/usr/bin/.
	cp -p $(BCM_FSBUILD_DIR)/gpl/usr/sbin/aa-status $(BCM_FSINSTALL_DIR)/usr/sbin/.
	cp -p $(BCM_FSBUILD_DIR)/gpl/sbin/apparmor_parser $(BCM_FSINSTALL_DIR)/sbin/.
	mkdir -p $(BCM_FSINSTALL_DIR)/etc/apparmor
	cp -p $(BCM_FSBUILD_DIR)/gpl/etc/apparmor/parser.conf $(BCM_FSINSTALL_DIR)/etc/apparmor/.
	rm -rf $(INSTALL_DIR)/etc/apparmor.d
	ln -s /var/apparmor.d $(INSTALL_DIR)/etc/apparmor.d
else
conditional_build: 
	@echo "skipping $(APP) (not configured)"
endif
else
conditional_build: 
	@echo "skipping $(APP) (DESKTOP_LINUX with old gcc)"
endif


clean:
	rm -rf $(INSTALL_DIR)/etc/apparmor
	rm -rf $(INSTALL_DIR)/etc/apparmor.d
	rm -rf $(INSTALL_DIR)/etc/lxcapparmor.d
	rm -rf $(INSTALL_DIR)/lib/apparmor
	rm -rf $(INSTALL_DIR)/var/lib/apparmor
	rm -f $(INSTALL_DIR)/lib/libapparmor.*
	-$(MAKE) -f Makefile clean

shell:
	bash -i
