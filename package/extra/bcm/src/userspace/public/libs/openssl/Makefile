
openssl: conditional_build

.PHONY: all distclean clean conditional_build openssl

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

# Patches
PATCH_FOLDER = bcm_patches_openssl_v3
PATCH_001_EDIT_OPEN_MAKE_CONFIG =            001-edit-openssl-make-configuration-files.patch
PATCH_002_CRYPTO_ARMCAP_DO_NOT_PROBE_NEON =  002-do-not-probe-neon-crypto-armcap.patch
PATCH_003_CNF_ADD_LEGACY_PROVIDER_SEC =      003-add-legacy-provider-sect-openssl-cnf.patch
PATCH_004_CNF_LOAD_ENGINES_FROM_FILE =       004-load-engines-from-file-openssl-cnf.patch

PATCHES_TO_APPLY := # Patches will be appended if needed
CRYPTO_ENGINES_TO_LOAD := # Engines will be appended if needed
ENGINESDIR = /lib$(if $(filter $(USER_ARCH),aarch64),64)/ssl/engines/
MODULESDIR = /lib$(if $(filter $(USER_ARCH),aarch64),64)

PATCH_COMPLETE_FILE = .patch_complete
UNTAR_COMPLETE_FILE = .untar_complete
CONFIG_ID_FILE = BRCM_CONFIG_ID_$(TOOLCHAIN_PREFIX)_$(LINUX_VER_STR)

# Openssl app folder name
APP=openssl-3.1.4
 
# Configurations
CONFIGURATIONS_10_MAIN = $(APP)/Configurations/10-main.conf
CONFIGURATIONS_UNIX_MAKEFILE = $(APP)/Configurations/unix-Makefile.tmpl
 
ifeq ($(strip $(DO_BUILD_OPENSSL)),y)
conditional_build: all
else
conditional_build:
	@echo "skipping $(APP) (not configured)"
endif

# Clear CC to avoid conflict with openssl configuration
CC=

# The options are ordered for the configure script
ifeq ($(strip $(PROFILE_ARCH)),aarch64)
SSLOPTIONS := linux-aarch64
else
SSLOPTIONS := linux-bcmarm
endif

ifeq ($(strip $(DESKTOP_LINUX)),y)
ifeq ($(strip $(USER_ARCH)),x86_64)
SSLOPTIONS := linux-x86_64
else
SSLOPTIONS := $(BCM_LD_FLAGS) linux-generic32
endif
endif

SSLOPTIONS += --prefix=$(BCM_FSBUILD_DIR)/public --openssldir=$(BCM_FSBUILD_DIR)/etc/ssl
SSLOPTIONS += shared no-md2 no-mdc2 no-rc2 no-idea no-unit-test no-cast no-bf

# The Default TLS Security Level
# Level 0 Everything is permitted. SSL 3, TLS 1.0, TLS 1.1, and DTLS 1.0 only work at this level.
# Level 1 The security level corresponds to a minimum of 80 bits of security.
# Level 2 Security level set to 112 bits of security / Level 3 set to 128 bits / Level 4 set to 192 bits / Level 5 set to 256 bits
SSLOPTIONS += -DOPENSSL_TLS_SECURITY_LEVEL=1

# By default, the OpenSSL library is built with tracing disabled. To use the tracing functionality, it is necessary to configure with the 'enable-trace' option
# SSLOPTIONS += enable-trace

ifeq ($(strip $(BUILD_TR69C_SSL)$(BUILD_TR69C_BASIC_SSL)$(BUILD_HTTPD_SSL)$(BUILD_HTTPD_BASIC_SSL)$(BUILD_BRCM_HOSTAPD)),)
SSLOPTIONS += no-rc4
endif

ifneq ($(strip $(BENCHMARKS_SMP)),y)
SSLOPTIONS += no-threads no-zlib
endif

ifeq ($(strip $(BUILD_HTTPD_SSL)$(BUILD_HTTPD_BASIC_SSL)),)
SSLOPTIONS += no-rc5 no-err no-ssl2 -DOPENSSL_NO_HEARTBEATS
endif

ifeq ($(strip $(ARCH_ENDIAN)),little)
SSLOPTIONS += -DL_ENDIAN
else
SSLOPTIONS += -DB_ENDIAN
endif

# Build crypto engines list and flags
ifneq ($(strip $(BUILD_CRYPTODEV_LINUX)),)
SSLOPTIONS += enable-devcryptoeng -DHAVE_CRYPTODEV -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)
CRYPTO_ENGINES_TO_CONFIGURE += devcrypto
else ifneq ($(strip $(BCM_OPTEE)),)
CRYPTO_ENGINES_TO_CONFIGURE += pkcs11
else
SSLOPTIONS += no-engine no-hw
endif

SSLOPTIONS += --with-rand-seed=devrandom
SSLOPTIONS += $(BRCM_COMMON_CFLAGS) $(SSP_TYP_COMPILER_OPTS) $(SSP_TYP_LIBS) $(SSP_LIB_INC) -DDEBUG -g

ifeq ($(strip $(DESKTOP_LINUX)),)
SSLOPTIONS += --sysroot=$(TOOLCHAIN_SYSROOT)
endif

# Build patches list
PATCHES_TO_APPLY += $(PATCH_001_EDIT_OPEN_MAKE_CONFIG)

# BRCM SPECIFIC: On the 138, NEON is only on one core. For the 148: currently the neon registers are not saved on a context switch.
ifeq ($(strip $(BUILD_OPENSSL_NEON)),)
ifneq ($(findstring _$(strip $(BRCM_CHIP))_,_63138_63148_),)
PATCHES_TO_APPLY += $(PATCH_002_CRYPTO_ARMCAP_DO_NOT_PROBE_NEON)
endif
endif	

# Many tools (pppoa/pppoe/pptp/l2tp and more) require this
# patch for legacy section addition in openssl.cnf from openssl-3.1.2
PATCHES_TO_APPLY += $(PATCH_003_CNF_ADD_LEGACY_PROVIDER_SEC)

ifneq ($(strip $(BUILD_CRYPTODEV_LINUX)),)
	PATCHES_TO_APPLY += $(PATCH_004_CNF_LOAD_ENGINES_FROM_FILE)
else ifneq ($(strip $(BCM_OPTEE)),)
	PATCHES_TO_APPLY += $(PATCH_004_CNF_LOAD_ENGINES_FROM_FILE)
endif

$(APP)/$(UNTAR_COMPLETE_FILE): $(APP).tar.gz
	rm -rf $(APP)
	tar xzf $(APP).tar.gz
	@echo "$(APP) is untarred"
	touch $(APP)/$(UNTAR_COMPLETE_FILE)
	
	# Applying patches
	@echo "Applying patches with order: $(PATCHES_TO_APPLY)"
	$(foreach p, $(PATCHES_TO_APPLY), patch -p1 -d$(APP) < $(PATCH_FOLDER)/$(p);)
	touch $(APP)/$(PATCH_COMPLETE_FILE)
	
	# avoid system consider the patched files 10-main.conf/unix-Makefile.tmpl are newer than the configdata.pm, then compile abort due to dependency
	( [[ -f $(CONFIGURATIONS_10_MAIN) ]] && touch -t `date -d "5 minutes ago" +%Y%m%d%H%M.%S` $(CONFIGURATIONS_10_MAIN) ) || true
	( [[ -f $(CONFIGURATIONS_UNIX_MAKEFILE) ]] && touch -t `date -d "5 minutes ago" +%Y%m%d%H%M.%S` $(CONFIGURATIONS_UNIX_MAKEFILE) ) || true

$(APP)/$(CONFIG_ID_FILE): $(APP)/$(UNTAR_COMPLETE_FILE)
	@echo "$(APP) is being configured"
	#beware: Configure modifies the Makefile
	(cd $(APP); ./Configure $(SSLOPTIONS);)
	$(MAKE) -C $(APP) depend
	rm -f $(APP)/BRCM_CONFIG_ID_*
	touch $(APP)/$(CONFIG_ID_FILE)

ifdef NO_SSL_PARABUILD
NOPARABUILD :=-j1
endif

all: $(APP)/$(CONFIG_ID_FILE)
	$(MAKE) $(NOPARABUILD) -C $(APP) ENGINESDIR=$(ENGINESDIR) MODULESDIR=$(MODULESDIR) all
	$(MAKE) -C $(APP) install_dev
	mkdir -p $(INSTALL_DIR)/bin
	mkdir -p $(INSTALL_DIR)/etc/ssl
	install -m 755 $(APP)/apps/openssl $(INSTALL_DIR)/bin
	install -m 755 $(APP)/apps/openssl.cnf $(INSTALL_DIR)/etc/ssl
	mkdir -p $(INSTALL_DIR)/lib
	mkdir -p $(BCM_FSBUILD_DIR)/public/lib
	SHLIB_VERSION_NUMBER=$$(grep -Po '(?<=SHLIB_VERSION_NUMBER=).*' ./$(APP)/Makefile ); \
	install -m 755 $(APP)/libssl.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib/; \
	cd $(INSTALL_DIR)/lib; rm -f libssl.so; ln -s libssl.so.$$SHLIB_VERSION_NUMBER libssl.so; cd - > /dev/null; \
	install -m 755 $(APP)/libcrypto.so.$$SHLIB_VERSION_NUMBER $(INSTALL_DIR)/lib/; \
	cd $(INSTALL_DIR)/lib; rm -f libcrypto.so; ln -s libcrypto.so.$$SHLIB_VERSION_NUMBER libcrypto.so; cd - > /dev/null; \
	cd $(BCM_FSBUILD_DIR)/public/lib; rm -f libssl.so; ln -s libssl.so.$$SHLIB_VERSION_NUMBER libssl.so; cd - > /dev/null; \
	cd $(BCM_FSBUILD_DIR)/public/lib; rm -f libcrypto.so; ln -s libcrypto.so.$$SHLIB_VERSION_NUMBER libcrypto.so;
	#install legacy.so for pppd-2.5.x later version
	cd $(BCM_FSBUILD_DIR)/public/lib; rm -f legacy.so; cd - > /dev/null; \
	cd $(INSTALL_DIR)/lib; rm -f legacy.so; cd - > /dev/null; \
	install -m 755 $(APP)/providers/legacy.so $(BCM_FSBUILD_DIR)/public/lib/; \
	install -m 755 $(APP)/providers/legacy.so $(INSTALL_DIR)/lib/;
	mkdir -p $(BCM_FSBUILD_DIR)/public/include/openssl
	cp -p $(APP)/include/openssl/* $(BCM_FSBUILD_DIR)/public/include/openssl

ifneq ($(strip $(CRYPTO_ENGINES_TO_CONFIGURE)),)
	mkdir -p $(INSTALL_DIR)/$(ENGINESDIR)
	install -m 755 $(APP)/engines/*.so $(INSTALL_DIR)/$(ENGINESDIR)
	@echo Crypto engines to configure: $(CRYPTO_ENGINES_TO_CONFIGURE)
	# Uncomment each engine in CRYPTO_ENGINES_TO_CONFIGURE
	mkdir -p $(INSTALL_DIR)/etc/ssl/engines.cnf.d/
	install -m 755 files/engines.cnf $(INSTALL_DIR)/etc/ssl/
	install -m 755 files/engines/*.cnf $(INSTALL_DIR)/etc/ssl/engines.cnf.d/
	$(foreach eng, $(CRYPTO_ENGINES_TO_CONFIGURE), sed -i 's/#$(eng)/$(eng)/' $(INSTALL_DIR)/etc/ssl//engines.cnf)
endif

clean:
	rm -rf $(APP)
	rm -f $(INSTALL_DIR)/lib/libssl.so*
	rm -f $(INSTALL_DIR)/lib/libcrypto.so*
	rm -f $(INSTALL_DIR)/bin/openssl
	rm -rf $(INSTALL_DIR)/etc/ssl
	rm -rf $(INSTALL_DIR)/$(ENGINESDIR)
	rm -f $(BCM_FSBUILD_DIR)/public/lib/libssl.so*
	rm -f $(BCM_FSBUILD_DIR)/public/lib/libcrypto.so*
	rm -f $(BCM_FSBUILD_DIR)/public/include/openssl/*
	rm -f $(INSTALL_DIR)/lib/legacy.so*
	rm -f $(BCM_FSBUILD_DIR)/public/lib/legacy.so*
distclean: clean
	rm -rf $(APP)

bcm_dorel_distclean: distclean

shell:
	bash -i

