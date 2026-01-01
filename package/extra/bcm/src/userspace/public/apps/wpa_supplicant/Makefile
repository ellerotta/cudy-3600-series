# wpa_supplicant makefile
WPA_SUPPLICANT_VER := 2.9
WPA_SUPPLICANT_TAR := wpa_supplicant-$(WPA_SUPPLICANT_VER).tar.gz
WPA_SUPPLICANT_DIR := wpa_supplicant-$(WPA_SUPPLICANT_VER)

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
conditional_build: wpa_supplicant
else
conditional_build:
	@echo "skipping hostapd (not configured)"
endif

$(WPA_SUPPLICANT_DIR)/Makefile: $(WPA_SUPPLICANT_TAR)
	@rm -rf $(WPA_SUPPLICANT_DIR)
	@echo "Untarring source and overrides..."
	@tar xf $(WPA_SUPPLICANT_TAR) 2> /dev/null
	@patch -p1 -b -N -s -d$(WPA_SUPPLICANT_DIR) < $(WPA_SUPPLICANT_DIR).patch

wpa_supplicant: $(WPA_SUPPLICANT_DIR)/Makefile
	cp ./brcm_wpa_supplicant.config $(WPA_SUPPLICANT_DIR)/wpa_supplicant/.config;
	$(MAKE) -C $(WPA_SUPPLICANT_DIR)/wpa_supplicant
	cp $(WPA_SUPPLICANT_DIR)/wpa_supplicant/wpa_supplicant $(INSTALL_DIR)/bin/wpa_supplicant_macsec
	cp $(WPA_SUPPLICANT_DIR)/wpa_supplicant/wpa_cli $(INSTALL_DIR)/bin/wpa_cli_macsec
	cp -f ./wpa_supplicant_macsec.conf $(INSTALL_DIR)/etc/

clean:
	-@rm -rf $(WPA_SUPPLICANT_DIR)
	-@rm -f $(INSTALL_DIR)/bin/wpa_supplicant_macsec
	-@rm -f $(INSTALL_DIR)/bin/wpa_cli_macsec

bcm_dorel_distclean: distclean

distclean:
	-rm -rf $(WPA_SUPPLICANT_DIR)