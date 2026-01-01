VER := 4.3.3
SRC := $(LIB)-$(VER).tar.gz
PATCH := $(LIB)-$(VER).patch
CMAKE := $(if $(shell which cmake3),cmake3,cmake)
CMAKE_VER := $(shell ${CMAKE} --version | sed -n 's/^.\+version \([0-9]\).[0-9\.]\+/\1/p')
ifneq ($(CMAKE_VER),3)
        $(error Error - unsupported cmake version $(CMAKE_VER) detected, please install version 3.x)
endif

HEADERS := lws_config.h libwebsockets.h

check_cmake: $(SRC)
	@if [ ! -e $(LIB)/Makefile ]; then \
	    echo "Untarring original $(SRC) source and cmake"; \
		mkdir -p $(LIB); \
		( cd $(LIB); tar --strip-components=1 -xzf ../$(SRC); \
		cd ../; patch -p1 -b -s -d $(LIB) <$(PATCH); cd $(LIB);\
		${CMAKE} \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_RANLIB=$(RANLIB) \
		-DCMAKE_STRIP=$(STRIP) \
		-DCMAKE_NM:FILEPATH=$(NM) \
		-DCMAKE_OBJCOPY:FILEPATH=$(OBJCOPY) \
		-DCMAKE_OBJDUMP=$(OBJDUMP) \
		-DCMAKE_LINKER:FILEPATH=$(word 1,$(LD)) \
		-DCMAKE_AR:FILEPATH=$(AR) \
		-DCMAKE_FIND_ROOT_PATH:PATH="$(ROOT_PATH)" \
		-DCMAKE_PREFIX_PATH:PATH="$(PREFIX)" \
		-DCMAKE_INSTALL_PREFIX:PATH=$(PREFIX) \
		-DLWS_WITH_SHARED=ON \
		-DLWS_WITH_STATIC=OFF \
		-DLWS_WITHOUT_TESTAPPS=ON \
		-DLWS_WITHOUT_TEST_CLIENT=ON -DLWS_WITHOUT_TEST_PING=ON \
		-DLWS_WITHOUT_TEST_SERVER=ON -DLWS_WITHOUT_TEST_SERVER_EXTPOLL=ON \
		-DLWS_HAVE_HMAC_CTX_new=ON -DLWS_HAVE_EVP_MD_CTX_free=ON \
		-DLWS_WITH_SSL=ON -DLWS_WITH_EXTERNAL_POLL=ON \
		-DLWS_OPENSSL_LIBRARIES=$(LIB_INSTALL_DIR) \
		-DLWS_OPENSSL_INCLUDE_DIRS=$(INCLUDE_DIR) ); \
	fi

install: check_cmake
	if [ ! -d $(LIB)/.libs ]; then \
		make -C $(LIB)/lib; \
	    make -C $(LIB) install; \
	fi;

clean:
	rm -f $(LIB_INSTALL_DIR)/$(LIB)*
	rm -f $(LIB_INSTALL_DIR)/pkgconfig/$(LIB)*
	rm -rf $(LIB_INSTALL_DIR)/cmake/$(LIB)
	rm -f $(addprefix $(HEADER_INSTALL_DIR)/,$(HEADERS))
	rm -rf $(HEADER_INSTALL_DIR)/$(LIB)
	rm -rf $(LIB)
