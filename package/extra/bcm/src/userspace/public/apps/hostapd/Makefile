# hostap makefile
HOSTAPD_VER := 2.9
HOSTAPD_DIR := hostapd-$(HOSTAPD_VER)
HOSTAPD_TAR := $(HOSTAPD_DIR).tar.gz


all: conditional_build

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


ifeq ($(strip $(BUILD_MACSEC)),y)
conditional_build: hostapd
else
conditional_build:
	@echo "skipping hostapd (not configured)"
endif

$(HOSTAPD_DIR)/Makefile: $(HOSTAPD_TAR)
	@rm -rf $(HOSTAPD_DIR)
	@echo "Untarring source and overrides..."
	@tar xf $(HOSTAPD_TAR) 2> /dev/null
	@patch -p1 -b -N -s -d$(HOSTAPD_DIR) < $(HOSTAPD_DIR).patch

hostapd: $(HOSTAPD_DIR)/Makefile
	cp ./brcm_hostapd.config $(HOSTAPD_DIR)/hostapd/.config;
	$(MAKE) -C $(HOSTAPD_DIR)/hostapd
	cp $(HOSTAPD_DIR)/hostapd/hostapd $(INSTALL_DIR)/bin/hostapd_macsec
	cp $(HOSTAPD_DIR)/hostapd/hostapd_cli $(INSTALL_DIR)/bin/hostapd_cli_macsec
	cp -f ./hostapd_macsec.conf $(INSTALL_DIR)/etc/

clean:
	-@rm -f $(INSTALL_DIR)/bin/hostapd_macsec
	-@rm -f $(INSTALL_DIR)/bin/hostapd_cli_macsec
	-@rm -rf $(HOSTAPD_DIR)

bcm_dorel_distclean: distclean

distclean:
	-rm -rf $(HOSTAPD_DIR)