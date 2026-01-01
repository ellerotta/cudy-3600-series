
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

PREFIX = $(BCM_FSBUILD_DIR)/public

include $(BUILD_DIR)/make.common

APP = libmnl-1.0.5
APP_BASE = libmnl

.PHONY: check_untar_patch_configure conditional_build check_versions


ifneq ($(strip $(BUILD_LIBMNL)),)

# NOTE: patch after build fixes error in make clean 
check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/config.log ]; then \
	    echo "Untarring original $(APP) source"; \
	    tar xkjf $(APP).tar.bz2 2> /dev/null || true; \
	    echo "Building $(APP) to $(BCM_FSINSTALL_DIR)/lib"; \
	    cd $(APP); \
	    ./configure --host=$(TOOLCHAIN_PREFIX) \
	    --prefix=$(PREFIX) --without-doxygen \
	    CFLAGS=-s\ -Os\ -Wno-strict-aliasing || exit 1; \
	fi

conditional_build: check_untar_patch_configure 
	cd $(APP); \
	$(MAKE); \
	$(MAKE) install; \
	test -z $(BCM_FSINSTALL_DIR)/lib || mkdir -p $(BCM_FSINSTALL_DIR)/lib ;  \
	cp -af $(PREFIX)/lib/$(APP_BASE).so* $(BCM_FSINSTALL_DIR)/lib ;  \
	echo "Done building $(APP_BASE)"

else
conditional_build:
	@echo "skipping $(APP) (not configured)"
endif


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	rm -f $(BCM_FSINSTALL_DIR)/lib/$(APP_BASE).so* ;
	-[ ! -e $(APP)/Makefile ] || $(MAKE) -C $(APP) clean distclean
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean



