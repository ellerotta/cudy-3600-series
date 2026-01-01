
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

PREFIX = $(BCM_FSBUILD_DIR)/gpl

include $(BUILD_DIR)/make.common

APP = conntrack-tools-1.4.8

.PHONY: check_untar_patch_configure conditional_build


ifneq ($(strip $(BUILD_CONNTRACK_TOOLS)),)

check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring original $(APP) source"; \
		(tar xf $(APP).tar.xz 2> /dev/null || true); \
		echo "Running configure on $(APP)"; \
		echo "building to $(INSTALL_DIR)"; \
		patch -p1 -b -s -d$(APP) < remove_rpath.patch; \
		cd $(APP); \
		export PKG_CONFIG_LIBDIR=$(PREFIX)/lib:$(PREFIX)/../public/lib; \
		export PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig:$(BCM_FSBUILD_DIR)/public/lib/pkgconfig; \
		./configure --host=$(TOOLCHAIN_PREFIX) \
		--prefix=$(PREFIX) \
		LIBNETFILTER_CTHELPER_CFLAGS=-I$(PREFIX)/include LIBNETFILTER_CTHELPER_LIBS="-L$(PREFIX)/lib -lnetfilter_cthelper" \
		LIBNETFILTER_CONNTRACK_CFLAGS=-I$(PREFIX)/include LIBNETFILTER_CONNTRACK_LIBS="-L$(PREFIX)/lib -lnetfilter_conntrack" \
		LIBTIRPC_CFLAGS=-I$(PREFIX)/../public/include/tirpc LIBTIRPC_LIBS=-L$(PREFIX)/../public/lib; \
		LIBMNL_CFLAGS=-I$(BCM_FSBUILD_DIR)/public/include LIBMNL_LIBS="-L$(BCM_FSBUILD_DIR)/public/lib -lmnl" \
		LIBNETFILTER_CTTIMEOUT_CFLAGS=-I$(PREFIX)/include LIBNETFILTER_CTTIMEOUT_LIBS="-L$(PREFIX)/lib -lnetfilter_cttimeout" \
		; \
	fi

conditional_build: check_untar_patch_configure 
	cd $(APP); $(MAKE) ; $(MAKE) install; \
	test -z $(INSTALL_DIR)/sbin || mkdir -p $(INSTALL_DIR)/sbin ;  \
	cp -f $(PREFIX)/sbin/conntrack* $(INSTALL_DIR)/sbin

else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean



