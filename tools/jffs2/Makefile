
include $(TOPDIR)/rules.mk

PKG_NAME := jffs2

include $(INCLUDE_DIR)/host-build.mk

define Package/busybox/config
	source "$(SOURCE)/Config.in"
endef

define Host/Prepare
	mkdir -p $(HOST_BUILD_DIR)
	$(CP) ./src/* $(HOST_BUILD_DIR)/
	find $(HOST_BUILD_DIR) -name .svn | $(XARGS) rm -rf
endef

define Host/Compile
	$(MAKE) -C $(HOST_BUILD_DIR) LDFLAGS="$(HOST_STATIC_LINKING)"
endef

define Host/Configure
endef

define Host/Install
	$(CP) $(HOST_BUILD_DIR)/mkfs.jffs2 $(STAGING_DIR_HOST)/bin/
endef

define Host/Clean
	rm -f $(STAGING_DIR_HOST)/bin/mkfs.jffs2
endef

$(eval $(call HostBuild))




