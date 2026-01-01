#***********************************************************************
#
#  Copyright (c) 2012  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
PPPD_DIR = $(BUILD_DIR)/userspace/gpl/apps/ppp
PPPD_VER_FILE = $(PPPD_DIR)/pppd.version
PPTP_SRC_PPPD_DIR = $(CURR_DIR)/$(APPDIR)/pppd_plugin/src/pppd
PPTPD_PLUGIN_DIR = $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/plugins
PPTPD_IFPPPOX_H = $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/if_pppox.h
INCLUDE_DIRS= -I../include \
              -I$(KERNEL_LINKS_DIR) \
              -I$(BCM_FSBUILD_DIR)/bcmdrivers/include \
              -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
              -I$(BCM_FSBUILD_DIR)/include \
              -I$(BCM_FSBUILD_DIR)/public/include \
	          -I$(INC_INSTALL_KERNEL_HDR_PATH)/include
	          
CXX := $(CC)
APPSOURCE = accel-pptp-0.8.5.tar.bz2
APPDIR = accel-pptp-0.8.5
PPTPD_DIR = pptpd-1.3.3
PPTPD_EXEC = pptpd
PPTPD_CTL_EXEC = pptpctrl
BCRELAY_EXEC = bcrelay
PPTPD_LOGWTMP_PLUGINS = pptpd-logwtmp.so
PPTPD_CFLAGS   += -DBCM_PPTPD_LIB_PATH=1 -DPPTPD_CTRL_LOG=1
PPTPD_BIN_PATH = /bin

# this app is very very old and uses an internal glibc header
# (sys/cdefs.h) which defines the __P macro for portable
# function prototypes for pre-C90 code. When we're compiling
# with MUSL, just define __P(args) as 'args' on the command
# line.
ifeq ($(BRCM_GLIBC),)
CFLAGS += -D__P\(args\)=args
PPTPD_CFLAGS += -D__P\(args\)=args
endif

export CXX CFLAGS CXXFLAGS LDFLAGS CC USERSPACE_PUBLIC_LIBS_DIR PPTPD_CFLAGS

clean:
	if [ -f ./$(APPDIR)/pppd_plugin/Makefile ];then rm -rf install $(APPDIR); fi;

ifneq ($(strip $(BUILD_ACCEL_PPTP))$(strip $(BUILD_ACCEL_PPTPNS)),)
ifneq ("$(wildcard $(PPPD_VER_FILE))","")
include $(PPPD_VER_FILE)
endif
check_untar_patch_configure: sanity_check
	if [ ! -e $(APPDIR)/Makefile ]; then \
	(tar xvf $(APPSOURCE)> /dev/null || true);\
	rm -f $(PPTPD_IFPPPOX_H);\
	fi; \
	if [ ! -e ./$(APPDIR)/pppd_plugin/Makefile ];then \
	patch -p1 -b -N -s -d $(APPDIR) <$(APPDIR).patch;\
	cd ./$(APPDIR)/pppd_plugin; ./configure pppd=$(PPPD_VERSION) KERNELVERSION=$(LINUX_SUB_DIR) --prefix=$(CURR_DIR)/install --host=$(TOOLCHAIN_PREFIX) --target=$(TOOLCHAIN_PREFIX);\
	cd $(CURR_DIR) ;\
	fi
	if [ ! -e ./$(APPDIR)/$(PPTPD_DIR)/Makefile ];then \
	cd ./$(APPDIR)/$(PPTPD_DIR); ./configure pppd=$(PPPD_VERSION) KERNELVERSION=$(LINUX_SUB_DIR) --sbindir=$(PPTPD_BIN_PATH) --prefix=$(CURR_DIR)/install --enable-bcrelay --host=$(TOOLCHAIN_PREFIX) --target=$(TOOLCHAIN_PREFIX);\
	cd $(CURR_DIR) ;\
	fi
	@echo "configure is done"
	@echo "ACCEL-PPTP VERSION is $(APPDIR), PPTPD VERSION is $(PPTPD_DIR), PPPD VERSION is $(PPPD_VERSION) "
conditional_build: check_untar_patch_configure
	@echo "Start to compile pppd_plugin ..."
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/pppd_plugin/src install
	@echo "pppd_plugin compile done !"
	mkdir -p $(INSTALL_DIR)/lib
	# pptp plugins for pppd
	install -m 755 $(CURR_DIR)/install/lib/pptp.so.0.0.0 $(INSTALL_DIR)/lib
	cd $(INSTALL_DIR)/lib ; rm -f pptp.so ; rm -f pptp.so.0 ; ln -s pptp.so.0.0.0 pptp.so ; ln -s pptp.so.0.0.0 pptp.so.0
	cd $(CURR_DIR)
	
	@echo "Start to compile $(PPTPD_DIR) ..."
	if [ -e $(PPPD_DIR)/$(PPPD_VERSION)/pppd/pppd.h ]; then \
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR) "PPTPD_CFLAGS=$(PPTPD_CFLAGS) -I$(PPPD_DIR)/$(PPPD_VERSION)/include -I. -I../." ; \
	else \
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR) ; \
	fi
	@echo "$(PPTPD_DIR) compile done !"
	# plugins lib
	install -m 755 $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/plugins/$(PPTPD_LOGWTMP_PLUGINS) $(INSTALL_DIR)/lib
	mkdir -p $(INSTALL_DIR)/bin
	# pptpd exec	
	install -m 755 $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/$(PPTPD_EXEC) $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/$(PPTPD_EXEC)
	# control exec	
	install -m 755 $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/$(PPTPD_CTL_EXEC) $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/$(PPTPD_CTL_EXEC)
	# broadcast relay	
	install -m 755 $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/$(BCRELAY_EXEC) $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/$(BCRELAY_EXEC)
	mkdir -p $(INSTALL_DIR)/etc
	mkdir -p $(INSTALL_DIR)/etc/ppp
	# pptpd.conf
	cp -f $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/samples/pptpd.conf $(INSTALL_DIR)/etc/.
	# options.pptpd
	cp -f $(CURR_DIR)/$(APPDIR)/$(PPTPD_DIR)/samples/options.pptpd $(INSTALL_DIR)/etc/ppp/.
else
conditional_build: sanity_check
	@echo "skipping $(APPDIR) (not configured)"
endif

