SRC := yaml-0.2.5
LIB_DIR := yaml


ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS = CFLAGS='$(BCM_LD_FLAGS)'
TOOLCHAIN_PREFIX=$(PROFILE_ARCH)-none-linux
endif


check_config: $(SRC).tar.gz
	@if [ ! -e $(LIB_DIR)/Makefile ]; then \
	    echo "Untarring original $(SRC) source and configure"; \
		mkdir -p $(LIB_DIR); \
		( cd $(LIB_DIR); tar --strip-components=1 -xzf ../$(SRC).tar.gz; \
		./configure --prefix=$(BCM_FSBUILD_DIR)/public --host=$(TOOLCHAIN_PREFIX) $(BCM_BLT32_FLAGS) ); \
	fi

install: check_config
	make -C $(LIB_DIR) install;

clean:
	-[ ! -e $(LIB_DIR)/Makefile ] || make -C $(LIB_DIR) uninstall
	rm -rf $(LIB_DIR)
