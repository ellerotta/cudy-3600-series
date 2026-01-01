all install default: conditional_build

APP=blkpg-part
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common


ARCH=$(PROFILE_ARCH)
PREFIX=$(INSTALL_DIR)
ALLOWED_INCLUDE_PATHS  := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR)

export ARCH PREFIX CFLAGS CMS_OPTION_RPATH 

SCRIPT:=repart.sh

ifneq ($(strip $(BUILD_EMMC_REPART)),)

ifeq ($(strip $(BRCM_REPART_MIN_BOOTFS_SIZE_MB)),)
$(error "Repart build requires REPART_MIN_BOOTFS_SIZE_MB to be configured with valid values!")	
endif

conditional_build: check_config
	mkdir -p objs
	$(MAKE) -C objs -f ../$(APP)/Makefile install
	mkdir -p objs/cooked
	@cat scripts/$(SCRIPT) \
	| sed -e "s/__MIN_BOOTFS_SIZE_MB__/$(BRCM_REPART_MIN_BOOTFS_SIZE_MB)/" \
	    > objs/cooked/$(SCRIPT)
	mkdir -p $(PREFIX)/usr/sbin 
	install -p -m 755 objs/cooked/$(SCRIPT) $(PREFIX)/usr/sbin/$(SCRIPT)

else

conditional_build:
	@echo "Skipping Repart (not configured)"

endif

check_config: $(APP)/Makefile

$(APP)/Makefile: $(APP).tar.gz
	(tar xkzf $(APP).tar.gz 2> /dev/null || true)
	echo "Applying patches to $(APP)" ; \
	patch -p1 -b -N -s -d$(APP) < $(APP).patch ; \
	echo "blkpg-part is untarred"

clean:
	rm -f $(PREFIX)/usr/sbin/$(SCRIPT)
	@if [ -d objs ]; then \
		$(MAKE) -C objs -f ../$(APP)/Makefile clean; \
		rm -rf objs; \
	fi;


shell:
	bash -i

