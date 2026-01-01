SRC := getdns-1.7.3
LIB_DIR := getdns

check_cmake: $(SRC).tar.gz
	@if [ ! -e $(LIB_DIR)/Makefile ]; then \
	    echo "Untarring original $(SRC) source and cmake"; \
		mkdir -p $(LIB_DIR); \
		( cd $(LIB_DIR); tar --strip-components=1 -xzf ../$(SRC).tar.gz; \
		(patch -p1 < ../$(SRC).patch); \
		cmake \
		-DENABLE_STUB_ONLY=ON \
		-DBUILD_STUBBY=ON \
		-DUSE_LIBIDN2=OFF \
		-DBUILD_TESTING=OFF \
		-DOPENSSL_CRYPTO_LIBRARY=$(LIB_INSTALL_DIR)/libcrypto.so \
		-DOPENSSL_SSL_LIBRARY=$(LIB_INSTALL_DIR)/libssl.so \
		-DOPENSSL_INCLUDE_DIR=$(INCLUDE_DIR) \
		-DLIBYAML_LIBRARY=$(LIB_INSTALL_DIR)/libyaml.so \
		-DLIBYAML_INCLUDE_DIR=$(INCLUDE_DIR) \
		-DBUILD_LIBEV=OFF \
		-DBUILD_LIBEVENT2=OFF \
		-DBUILD_LIBUV=OFF \
		-DCMAKE_RANLIB=$(RANLIB) \
		-DCMAKE_STRIP=$(STRIP) \
		-DCMAKE_NM:FILEPATH=$(NM) \
		-DCMAKE_OBJCOPY:FILEPATH=$(OBJCOPY) \
		-DCMAKE_OBJDUMP=$(OBJDUMP) \
		-DCMAKE_LINKER:FILEPATH=$(LD) \
		-DCMAKE_AR:FILEPATH=$(AR) \
		-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
		-DCMAKE_FIND_ROOT_PATH:PATH="$(ROOT_PATH)" \
		-DCMAKE_PREFIX_PATH:PATH="$(PREFIX)" \
		-DCMAKE_INSTALL_LIBDIR=$(LIB_INSTALL_DIR) \
		-DCMAKE_INSTALL_PREFIX:PATH=$(PREFIX) ); \
	fi

install: check_cmake
	make -C $(LIB_DIR) install;

clean:
	@if [ -e $(LIB_DIR)/Makefile ]; then \
		[ -e $(LIB_DIR)/install_manifest.txt ] && xargs rm -rf < $(LIB_DIR)/install_manifest.txt; \
		rm -rf $(PREFIX)/share/doc/getdns $(INCLUDE_DIR)/getdns; \
		rm -rf $(PREFIX)/share/doc/stubby $(PREFIX)/etc/stubby; \
	fi
	rm -rf $(LIB_DIR)
