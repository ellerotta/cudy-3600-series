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

APP = xl2tpd-1.3.18

clean:
	@rm -rf $(APP) ;
	@rm -f $(INSTALL_DIR)/bin/xl2tpd ;

#.PHONY: check_untar_patch_configure conditional_build

ifneq ($(strip $(BUILD_L2TPAC)),)
check_untar_patch_configure: sanity_check
	@if [ ! -e $(APP)/README.md ]; then \
    	echo "Untarring original $(APP) source"; \
	    (tar zxvf $(APP).tar.gz> /dev/null || true); \
	    if [ -e ./$(APP)/Makefile ];then \
	    patch -p0 -b -s < $(APP).patch; \
	    echo "configure and patch is done" ; \
	    fi; \
	 fi   

conditional_build: check_untar_patch_configure
	cd ./$(APP); make ; make install;
	mkdir -p $(INSTALL_DIR)/etc/xl2tpd
	cp -f ./$(APP)/examples/xl2tpd.conf $(INSTALL_DIR)/etc/xl2tpd/.
	mkdir -p $(INSTALL_DIR)/etc/ppp/peers
	cp -f ./$(APP)/examples/ppp-options.xl2tpd $(INSTALL_DIR)/etc/ppp/peers/.	
else	
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif

