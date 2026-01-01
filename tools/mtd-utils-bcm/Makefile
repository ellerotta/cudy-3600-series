#
# Copyright (C) 2006-2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
include $(TOPDIR)/rules.mk

PKG_NAME:=mtd_utils_bcm

include $(INCLUDE_DIR)/host-build.mk

HOST_CONFIGURE_VARS+= \
	UUID_CFLAGS="-I$(STAGING_DIR_HOST)/include/e2fsprogs/uuid"

define Host/Prepare
	mkdir -p $(HOST_BUILD_DIR)
	$(CP) ./src/* $(HOST_BUILD_DIR)/
	(cd $(HOST_BUILD_DIR)  && ./configure --without-xattr --disable-tests)
endef

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) BUILDDIR=$(HOST_BUILD_DIR) WITHOUT_XATTR=1
endef

define Host/Install
	$(CP) \
		$(HOST_BUILD_DIR)/mkfs.jffs2 \
		$(HOST_BUILD_DIR)/mkfs.ubifs \
		$(HOST_BUILD_DIR)/ubinize \
		$(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/{mkfs.jffs2,mkfs.ubifs,ubinize}
endef

$(eval $(call HostBuild))
