
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

APP = breakpad
VERSION = v2023.01.27

BINARIES := core_handler

.PHONY: conditional_build check_versions check_config $(APP)

ifneq ($(strip $(BUILD_BREAKPAD)),)
conditional_build: $(BINARIES)

# Depend on tar file
# Use $(APP)/configure to mark tar progress is completed
$(APP)/untar_and_patch: $(APP).$(VERSION).tar.xz Makefile
	rm -rf $(APP)
	mkdir $(APP) && tar --strip-components=1 -xf $(APP).$(VERSION).tar.xz -C $(APP)
	echo "Applying patches to $(APP)"
	touch $@
	@echo "$(APP) is untarred"

# Depend on $(APP)/untar_and_patch so that tar progress should be done only once
$(APP)/Makefile: $(APP)/untar_and_patch
	cd $(APP); ./configure --prefix=$(BCM_FSBUILD_DIR) \
	--host=$(TOOLCHAIN_PREFIX) \
	CXXFLAGS=-I$(BCM_FSBUILD_DIR)/public/include \
	LDFLAGS=-L$(BCM_FSBUILD_DIR)/public/lib; cd ../.;
	@echo "$(APP) is configured"
	touch $@

$(BINARIES): $(APP)/Makefile | $(APP)
	install -m 755 $(APP)/src/tools/linux/core_handler/$@ $(INSTALL_DIR)/bin/$@

# Depend on check_config so that make progress can continue where it is stopped before
$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP)
	@echo "Done building $(APP)"
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -rf $(APP)
	rm -f $(INSTALL_DIR)/bin/$(BINARIES)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
