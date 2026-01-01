
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

ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_BRCM_BDK)),)
LIBS += -lcms_cli -lmdm_cli -lframework_cli
ifneq ($(strip $(BUILD_GPON)),)
LIBS += -lmdm_cli_gpon -lponsys
endif
ifneq ($(strip $(BUILD_EPON_SDK)),)
LIBS += -lmdm_cli_epon
endif
LIBS += -lcms_dal -lcms_msg $(CMS_CORE_LIBS) $(CMS_COMMON_LIBS)
CUSTOM_CFLAGS += -DBRCM_CMS_BUILD
CFLAGS +=$(ALLOWED_INCLUDE_PATHS)
endif

ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS := $(BCM_LD_FLAGS)
export BCM_BLT32_FLAGS
TOOLCHAIN_PREFIX=$(PROFILE_ARCH)-none-linux
endif

#
# Public apps are only allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/include \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(BCM_FSBUILD_DIR)/private/include \
                         -I$(BUILD_DIR)/userspace/private/include  \
                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)

APP = dropbear
VER = 2022.83

clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

#.PHONY: check_untar_patch_configure conditional_build

# Public apps are only allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/private/lib:/public/lib
LOCAL_LDFLAGS := $(BCM_BLT32_FLAGS) $(BCM_RPATH_LINK_OPTION) $(BCM_LD_FLAGS)

ifneq ($(strip $(BUILD_SSHD)),)

# disable hardening because dropbear crashes when it is dynamically linked
# and hardened. Possibly some corner case with SSP. Since we link to several
# libraries, static linking is not a good option here.
check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/Makefile.in ]; then \
    	echo "Untarring original $(APP) source"; \
	    (mkdir $(APP); cd $(APP); tar xkf ../$(APP)-$(VER).tar.bz2 --strip-components=1 2> /dev/null || true); \
	    if [ "x$(BUILD_BTEST_TOOLS)" == "x" ]; then \
	        patch -p1 -b -s -d$(APP) < $(APP)-$(VER).patch; \
	    fi; \
	    if [ "x$(BUILD_BRCM_CMS)" != "x" ] || [ "x$(BUILD_BRCM_BDK)" != "x" ]; then \
		patch -p1 -b -s -d$(APP) < $(APP)-$(VER)_cms.patch; \
	    fi; \
	    (cd $(APP); ./configure CFLAGS="$(CFLAGS)" LDFLAGS="$(LOCAL_LDFLAGS) -L$(INSTALL_DIR)/lib" LIBS="$(LIBS)" --host=$(TOOLCHAIN_PREFIX) \
        --target=$(TOOLCHAIN_PREFIX) \
        --disable-largefile --disable-zlib  --disable-openpty --disable-syslog  --disable-shadow  --disable-lastlog  --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx --disable-harden \
        --prefix=$(INSTALL_DIR) \
        --sbindir=$(INSTALL_DIR)/bin); \
	fi

conditional_build: check_untar_patch_configure
	cd $(APP); $(MAKE)
	install -m 755 $(APP)/dropbear  $(INSTALL_DIR)/bin/sshd
	install -m 755 $(APP)/dropbearconvert $(INSTALL_DIR)/bin/dropbearconvert
ifneq ($(strip $(BUILD_BTEST_TOOLS)),)
	mkdir -p $(INSTALL_DIR)/etc/dropbear
	ln -fs /var/tmp/dropbear_dss_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_dss_host_key
	ln -fs /var/tmp/dropbear_rsa_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_rsa_host_key
	ln -fs /var/tmp/dropbear_ecdsa_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_ecdsa_host_key
	ln -fs /var/tmp/dropbear_ed25519_host_key $(INSTALL_DIR)/etc/dropbear/dropbear_ed25519_host_key
endif
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
