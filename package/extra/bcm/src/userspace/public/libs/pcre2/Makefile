SRC := pcre2-10.37

ifeq ($(strip $(DESKTOP_LINUX)),y)
BCM_BLT32_FLAGS = CFLAGS='$(BCM_LD_FLAGS)'
TOOLCHAIN_PREFIX=$(PROFILE_ARCH)-none-linux
endif


check_config: $(SRC).tar.gz
	@if [ ! -e $(SRC)/Makefile ]; then \
	   tar zxvf $(SRC).tar.gz; \
	   cd $(SRC); \
	   ./configure --host=$(TOOLCHAIN_PREFIX) --disable-static \
	               --prefix=$(BCM_FSBUILD_DIR)/public $(BCM_BLT32_FLAGS); \
	   cd ..; \
	fi;

install: check_config
	$(MAKE) -C $(SRC) install;

clean:
	-[ ! -e $(SRC)/Makefile ] || $(MAKE) -C $(SRC) uninstall
	rm -rf $(SRC)
