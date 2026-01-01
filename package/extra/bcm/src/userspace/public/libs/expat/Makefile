APP := expat-2.5.0
LIB := libexpat.so

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
BCM_BLT32_FLAGS := "CFLAGS=$(BCM_LD_FLAGS)" "CXXFLAGS=$(BCM_LD_FLAGS)" "LDFLAGS=$(BCM_LD_FLAGS)"
else
BCM_BLT32_FLAGS := "LDFLAGS=$(LIBS_CC_RT)"
endif


HEADER_INSTALL_DIR := $(BCM_FSBUILD_DIR)/public/include/expat
EXT_HEADERS := expat.h expat_external.h
HEADERS := $(addprefix $(BCM_FSBUILD_DIR)/public/include/expat/,$(strip $(EXT_HEADERS)))
LIB_INSTALL_DIR := $(BCM_FSBUILD_DIR)/public/lib
# normally, the copy from LIB_INSTALL_DIR to FINAL_LIB_INSTALL_DIR is done
# by Bcmbuild.mk, but we don't have a Bcmbuild.mk in this directory yet.
FINAL_LIB_INSTALL_DIR := $(BCM_FSINSTALL_DIR)/lib

.PHONY: check_untar_patch_configure conditional_build check_versions $(APP)


ifneq ($(strip $(DO_BUILD_EXPAT)),)
conditional_build: do_install
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


# Depend on $(APP)/configure so that tar progress should be done only once
$(APP)/Makefile: $(APP).tar.gz Makefile
	rm -rf $(APP)
	mkdir -p $(APP)
	cd $(APP) && tar --strip-components=1 -xzf ../$(APP).tar.gz
	@echo "$(APP) is untarred"
	cd $(APP) && \
	ac_cv_func_malloc_0_nonnull=yes ac_cv_func_realloc_0_nonnull=yes \
	./configure --enable-static=no --enable-shared=yes --without-docbook --host=$(TOOLCHAIN_PREFIX) --prefix=$(BCM_FSBUILD_DIR)/public/ $(BCM_BLT32_FLAGS)
	@echo "$(APP) is configured"
	touch $@

# Depend on check_config so that make progress can continue where it is stopped before
$(LIB_INSTALL_DIR)/$(LIB): $(APP)/Makefile | $(APP)
	$(STRIP) $(LIB_INSTALL_DIR)/$(LIB)*

# expat installs headers at include/, but we want them in include/expat/
$(HEADERS): $(APP)/Makefile | $(APP)
	-mkdir -p $(dir $@)
	mv $(dir $@)/../$(notdir $@) $@

$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP)
	$(MAKE) -C $(APP) install
	@echo "Done building $(APP)"

do_install: $(LIB_INSTALL_DIR)/$(LIB) $(HEADERS)
	mkdir -p $(FINAL_LIB_INSTALL_DIR)
	cp -p $(LIB_INSTALL_DIR)/$(LIB)* $(FINAL_LIB_INSTALL_DIR)


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(LIB_INSTALL_DIR)/$(LIB)* $(HEADER_INSTALL_DIR)
	rm -rf $(APP)
	rm -rf $(FINAL_LIB_INSTALL_DIR)/$(LIB)*


# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
