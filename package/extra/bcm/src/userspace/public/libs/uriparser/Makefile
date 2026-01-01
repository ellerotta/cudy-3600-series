SRC := uriparser-0.9.7
LIB := liburiparser
LIB_DIR := uriparser

HEADERS = UriBase.h UriDefsAnsi.h UriDefsConfig.h UriDefsUnicode.h Uri.h UriIp4.h

check_cmake: $(SRC).tar.gz
	@if [ ! -e $(LIB_DIR)/Makefile ]; then \
	    echo "Untarring original $(SRC) source and cmake"; \
		mkdir -p $(LIB_DIR); \
		( cd $(LIB_DIR); tar --strip-components=1 -xzf ../$(SRC).tar.gz; \
		cmake \
		-DURIPARSER_BUILD_TESTS=OFF \
		-DURIPARSER_BUILD_DOCS=OFF \
		-DURIPARSER_BUILD_TOOLS=OFF \
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
	if [ ! -d $(LIB_DIR)/.libs ]; then \
		make -C $(LIB_DIR)/lib; \
	    make -C $(LIB_DIR) install; \
	fi

clean:
	rm -f $(LIB_INSTALL_DIR)/$(LIB).so*
	rm -rf $(HEADER_INSTALL_DIR)/$(LIB_DIR)
	rm -rf $(LIB_DIR)
