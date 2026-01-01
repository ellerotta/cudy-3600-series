all dynamic install: conditional_build

APP := davici-1.4
LIB := libdavici
TARBALL := v1.4.tar.gz

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

PREFIX = $(BCM_FSBUILD_DIR)/public

ifneq ($(strip $(BUILD_LIBEV)),)
conditional_build: $(INSTALL_DIR)/$(LIB) | sanity_check ;
else
conditional_build:
	@echo "skipping $(APP) (not configured)"
endif

$(APP)/Makefile.am: $(TARBALL)
	@if [ ! -e $(APP)/Makefile ]; then \
		echo "Extract original $(TARBALL) source"; \
		(tar xvf $(TARBALL) 2> /dev/null); \
	fi

$(APP)/configure: $(APP)/Makefile.am
	cd $(APP); \
	libtoolize --force ; \
	aclocal ; \
	autoheader ; \
	automake --force-missing --add-missing ; \
	autoconf

$(APP)/Makefile: $(APP)/configure
	(cd $(APP); \
	./configure \
		--prefix=$(PREFIX) \
		--host=$(TOOLCHAIN_PREFIX) \
		--includedir=$(PREFIX)/include \
		$(BCM_BLT32_FLAGS) || exit 1)
	@echo "$(APP) is configured"

$(INSTALL_DIR)/$(LIB): $(APP)/Makefile
	@$(MAKE) -C $(APP) install
	@mkdir -p $(INSTALL_DIR)/lib/public/
	@cp -d $(PREFIX)/lib/$(LIB).so* $(INSTALL_DIR)/lib/public/

clean:
	rm -f $(PREFIX)/lib/$(LIB).*
	rm -f $(INSTALL_DIR)/lib/public/$(LIB).*
	rm -rf $(PREFIX)/include/davici.h
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
