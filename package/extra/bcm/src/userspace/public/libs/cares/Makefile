SRC := c-ares-1.23.0
LIB := libcares

ifeq ($(strip $(DESKTOP_LINUX)),y)
BLT32_FLAGS = CFLAGS='$(BCM_LD_FLAGS)'
BLT32_CXXFLAGS = CXXFLAGS='$(BCM_LD_FLAGS)'
TOOLCHAIN_PREFIX=$(PROFILE_ARCH)-none-linux
endif


check_config: $(SRC).tar.gz
	@if [ ! -e $(LIB)/Makefile ]; then \
	    echo "Untarring original $(SRC) source and configure"; \
		mkdir -p $(LIB); \
		( cd $(LIB); tar --strip-components=1 -xzf ../$(SRC).tar.gz; \
		./configure --prefix=$(BCM_FSBUILD_DIR)/public --host=$(TOOLCHAIN_PREFIX) $(BCM_BLT32_FLAGS) $(BLT32_CXXFLAGS) --without-random --disable-tests ); \
	fi

install: check_config
	if [ ! -d $(LIB)/src/lib/.libs ]; then \
		$(MAKE) -C $(LIB) install; \
	fi;

clean:
	[ ! -d $(LIB) ] || $(MAKE) -C $(LIB) clean
	rm -rf $(LIB)
