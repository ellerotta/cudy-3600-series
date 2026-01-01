# Makefile to build PPPD

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
include $(BUILD_DIR)/make.common
COPTS = -O2 -pipe -Wall -g
INCLUDE_DIRS= -I../include \
              -I$(KERNEL_LINKS_DIR) \
              -I$(BCM_FSBUILD_DIR)/bcmdrivers/include \
              -I$(BCM_FSBUILD_DIR)/shared/opensource/include/$(BRCM_BOARD) \
              -I$(BCM_FSBUILD_DIR)/include \
              -I$(BCM_FSBUILD_DIR)/public/include \
	          -I$(INC_INSTALL_KERNEL_HDR_PATH)/include
CFLAGS+= $(COPTS) $(COMPILE_FLAGS) $(INCLUDE_DIRS)
CFLAGS+=-I$(TOOLCHAIN)/include
CFLAGS+= $(BCM_CFLAGS)
LDFLAGS+=-L$(BCM_FSBUILD_DIR)/public/lib\ -L$(BCM_FSBUILD_DIR)/gpl/lib
PREFIX =
APPSOURCE = ppp-2.5.0.tar.gz
APPDIR = ppp-2.5.0
APPVER = 2.5.0
APPNAME = pppd
DESTDIR = $(CURR_DIR)/install
BCM_PCAP_PATH = $(BCM_FSBUILD_DIR)/public
BCM_OPENSSL_PATH = $(BCM_FSBUILD_DIR)/public  
BCM_PPPD_PATH_VARRUN = /var/run 
LIBS = -lethswctl -lrt 
LIBS += -L$(INSTALL_DIR)/lib

ifneq ($(strip $(BUILD_BRCM_CMS)),)
LIBS 	+= -lcms_msg $(CMS_COMMON_LIBS) $(BCM_FLASH_LIBS)
CFLAGS 	+= $(CMS_COMPILE_FLAGS)
endif

ifneq ($(strip $(BUILD_BRCM_BDK)),)
LIBS 	+= -lcms_msg $(CMS_COMMON_LIBS) $(BCM_FLASH_LIBS)
CFLAGS 	+= $(CMS_COMPILE_FLAGS)
endif	 

CXX := $(CC)

# this app is very very old and uses an internal glibc header
# (sys/cdefs.h) which defines the __P macro for portable
# function prototypes for pre-C90 code. When we're compiling
# with MUSL, just define __P(args) as 'args' on the command
# line.
ifeq ($(BRCM_GLIBC),)
BCM_CFLAGS += -D__P\(args\)=args
endif

export CXX CFLAGS CXXFLAGS LDFLAGS CC USERSPACE_PUBLIC_LIBS_DIR INSTALL_DIR DESTDIR 
export CMS_COMMON_LIBS LIBS 
export BCM_CFLAGS
export INC_INSTALL_KERNEL_HDR_PATH
export KERNEL_DIR KERNEL_LINKS_DIR KERNEL_INCLUDE_LINK_NAME KERNEL_INCLUDE_LINK

clean_build:
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/pppd clean

clean:
	@if [ -e ./$(APPDIR)/$(APPNAME)/Makefile ]; then \
	rm -rf $(APPDIR) ; \
	rm -rf install ; \
	fi ; \
	if [ -e ./pppd.version ]; then \
	rm -f pppd.version ; \
	fi

#.PHONY: check_untar_patch_configure conditional_build

ifneq ($(strip $(BUILD_PPPD)),)
check_untar_patch_configure: sanity_check
	if [ ! -e ./$(APPDIR)/$(APPNAME)/Makefile ]; then \
		echo "Untarring $(APPDIR) source and overrides..." ; \
		(tar xf $(APPSOURCE) || true) ; \
		cd $(APPDIR); \
		#./configure LIBS="$(LIBS)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" --prefix=$(PREFIX) --host=$(TOOLCHAIN_PREFIX) --with-pcap=$(BCM_PCAP_PATH) --with-openssl=$(BCM_OPENSSL_PATH) ;\
		(./configure \
		--host=$(TOOLCHAIN_PREFIX) --prefix=$(PREFIX) \
		--with-pcap=$(BCM_PCAP_PATH) --with-openssl=$(BCM_OPENSSL_PATH) --with-runtime-dir=$(BCM_PPPD_PATH_VARRUN) --with-logfile-dir=$(BCM_PPPD_PATH_VARRUN) \
		--enable-multilink \
		LIBS="$(LIBS)" CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)");\
		cd $(CURR_DIR); \
		if [ ! -e ./pppd.version ]; then \
		echo PPPD_VERSION = $(APPDIR) > ./pppd.version; \
		fi; \
		if [ -e ./$(APPDIR)/$(APPNAME)/Makefile ];then \
		patch -p0 -b -s < $(APPDIR).patch; \
		fi; \
		echo "configure and patch is done" ; \
	fi

conditional_build: check_untar_patch_configure
ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
	# compile pppd	
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/$(APPNAME) install	
	# install pppd exec
	mkdir -p $(INSTALL_DIR)/bin
	install -m 755 $(CURR_DIR)/install/sbin/$(APPNAME) $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/$(APPNAME)
ifeq ($(strip $(DESKTOP_LINUX)),)
	# compile pppd plugins, use install-data option to compile *.so of plugins only
	$(MAKE) -C $(CURR_DIR)/$(APPDIR)/$(APPNAME)/plugins install-data
	# pppoatm libs
	mkdir -p $(INSTALL_DIR)/lib
	$(INSTALL)  -m 755 $(CURR_DIR)/install/lib/$(APPNAME)/$(APPVER)/pppoatm.so $(INSTALL_DIR)/lib
	# pppol2tp libs
	mkdir -p $(INSTALL_DIR)/lib
	$(INSTALL)  -m 755 $(CURR_DIR)/install/lib/$(APPNAME)/$(APPVER)/pppol2tp.so $(INSTALL_DIR)/lib
	$(INSTALL)  -m 755 $(CURR_DIR)/install/lib/$(APPNAME)/$(APPVER)/openl2tp.so $(INSTALL_DIR)/lib
	# pppoe libs
	$(INSTALL)  -m 755 $(CURR_DIR)/install/lib/$(APPNAME)/$(APPVER)/pppoe.so $(INSTALL_DIR)/lib
	cd $(INSTALL_DIR)/lib ; rm -f rp-pppoe.so ; ln -s pppoe.so rp-pppoe.so ;		
endif
endif
else
conditional_build: sanity_check
	@echo "skipping $(APPDIR) (not configured)"
endif

bcm_dorel_distclean: clean
	find $(APPDIR) -name .git -or -name .gitignore  -print0 | xargs -0 rm -rf
