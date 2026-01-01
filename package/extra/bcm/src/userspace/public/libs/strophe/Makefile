APP = libstrophe
LIB = libstrophe.so
VER = 0.12.3

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

LIB_INSTALL_DIR := $(BCM_FSBUILD_DIR)/public/lib
# normally, the copy from LIB_INSTALL_DIR to FINAL_LIB_INSTALL_DIR is done
# by Bcmbuild.mk, but we don't have a Bcmbuild.mk in this directory yet.
FINAL_LIB_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/lib
export PKG_CONFIG_LIBDIR=$(LIB_INSTALL_DIR)
export PKG_CONFIG_PATH=$(LIB_INSTALL_DIR)/pkgconfig

ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS = CFLAGS='$(BCM_LD_FLAGS)'
endif

.PHONY: conditional_build $(APP)

ifneq ($(strip $(BUILD_TR69C_SSL)),)
ifneq ($(strip $(BUILD_XMPP)),)
conditional_build: $(APP)

# Depend on tar file
$(APP)/Makefile: $(APP)-$(VER).tar.gz
	mkdir $(APP); \
	( cd $(APP); tar --strip-components=1 -xzf ../$(APP)-$(VER).tar.gz ; \
	patch -p1 < ../bcm_patches/00_correct_expat_path.diff; \
	./configure --prefix=$(BCM_FSBUILD_DIR)/public/ --host=$(TOOLCHAIN_PREFIX) $(BCM_BLT32_FLAGS) --enable-shared --disable-examples; );
	@echo "$(APP) is untarred and patched"

$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP) install && cp -d $(LIB_INSTALL_DIR)/$(LIB)* $(FINAL_LIB_INSTALL_DIR)/.
	@echo "Done building $(APP)"

else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif
else
conditional_build: sanity_check
	@echo "skipping $(APP) (TR69C SSL not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(FINAL_LIB_INSTALL_DIR)/$(LIB)*
	rm -f $(LIB_INSTALL_DIR)/$(LIB)*;
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
