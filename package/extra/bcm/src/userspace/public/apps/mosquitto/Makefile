

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

ifeq ($(strip $(DESKTOP_LINUX)),y)
CFLAGS += $(BCM_LD_FLAGS)
LDFLAGS += $(BCM_LD_FLAGS)
CPPFLAGS += $(BCM_LD_FLAGS)
export CFLAGS CPPFLAGS LDFLAGS
endif

unexport CROSS_COMPILE

APP = mosquitto
VER = 2.0.18
APPV = $(APP)-$(VER)

.PHONY: check_untar_patch_configure conditional_build check_versions

ifneq ($(strip $(BUILD_MOSQUITTO)),)
conditional_build: $(APP)
else 
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

check_untar_patch_configure:
	if [ ! -d $(APPV) ]; then \
	tar xkfz $(APPV).tar.gz; \
	(cd $(APPV); patch -p1 < ../$(APPV).patch); \
	fi

$(APP): check_untar_patch_configure
	$(MAKE) -C ./$(APPV) DESTDIR=$(BCM_FSBUILD_DIR)/public WITH_CJSON=no WITH_TLS_PSK=no WITH_MEMORY_TRACKING=no
	$(MAKE) -C ./$(APPV)/client install DESTDIR=$(BCM_FSBUILD_DIR)/public
	$(MAKE) -C ./$(APPV)/src install DESTDIR=$(BCM_FSBUILD_DIR)/public
	$(MAKE) -C ./$(APPV)/lib install DESTDIR=$(BCM_FSBUILD_DIR)/public
	$(INSTALL) -m 755 $(BCM_FSBUILD_DIR)/public/bin/mosquitto*  $(INSTALL_DIR)/bin/.
	$(INSTALL) -m 755 $(BCM_FSBUILD_DIR)/public/sbin/mosquitto  $(INSTALL_DIR)/bin/mosquitto
	cp -d $(BCM_FSBUILD_DIR)/public/lib/libmosquitto* $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)
	if [ -e ./$(APPV)/apps/mosquitto_passwd/mosquitto_passwd ];then install -m 755 ./$(APPV)/apps/mosquitto_passwd/mosquitto_passwd  $(INSTALL_DIR)/bin/mosquitto_passwd;fi;

clean:
	@if [ -d $(APPV) ]; then \
	$(MAKE) -C $(APPV)/client uninstall DESTDIR=$(BCM_FSBUILD_DIR)/public; \
	$(MAKE) -C $(APPV)/src uninstall DESTDIR=$(BCM_FSBUILD_DIR)/public; \
	$(MAKE) -C $(APPV)/lib uninstall DESTDIR=$(BCM_FSBUILD_DIR)/public; \
	$(MAKE) -C $(APPV) clean DESTDIR=$(BCM_FSBUILD_DIR)/public; \
	fi
	rm -f $(INSTALL_DIR)/bin/mosquitto*
	rm -f $(INSTALL_DIR)/lib$(BCM_INSTALL_SUFFIX_DIR)/libmosquitto*
	rm -rf $(APPV)
