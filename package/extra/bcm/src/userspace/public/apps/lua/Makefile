all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

APP = lua
VER = 5.1.5
APPV=$(APP)-$(VER)

.PHONY: check_untar conditional_build

ifneq ($(strip $(BUILD_LUA)),)
conditional_build: $(APP)
else
conditional_build:
	@echo "skipping $(APP) (not configured)"
endif

check_untar: $(APPV).tar.gz 
	if [ ! -d $(APPV) ]; then \
		@echo "untarring $(APPV).tar.gz..."; \
		tar -xvf $(APPV).tar.gz; \
	fi

check_patch: check_untar
	if [ ! -e ./.patch_complete ]; then \
		echo "Patching ${APPV}"; \
		patch -p1 -b -s -d ${APPV} < ${APPV}.patch; \
		touch .patch_complete; \
	fi; \

$(APP): check_patch
	$(MAKE) -C $(APPV) linux CC=$(CC) LD=$(LD) AR="$(AR) rcu" RANLIB=$(RANLIB)
	$(MAKE) -C $(APPV) install INSTALL_TOP=$(INSTALL_DIR) INSTALL_INC=$(BCM_FSBUILD_DIR)/public/include

clean:
	if [ -e $(APPV)/Makefile ]; then \
		$(MAKE) -C $(APPV) clean; \
	fi

bcm_dorel_distclean: distclean

distclean: 
	rm -rf $(APPV)
	rm -f .patch_complete

shell:
	bash -i


