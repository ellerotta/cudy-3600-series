# Makefile to build libsndfile

LIB = libsndfile-1.1.0

check_untar_configure: 
	if [ ! -e $(LIB)/Makefile.in ]; then              \
	  echo "Untarring source and overrides..." ;      \
	  (tar xzf $(LIB).tar.gz 2> /dev/null || true) ; \
	  cd $(LIB);                                      \
	  ./autogen.sh;                                      \
	  ./configure --host=${TOOLCHAIN_PREFIX} --prefix= --enable-shared; \
	fi

install: check_untar_configure
	@echo "Making $(LIB)"
	$(MAKE) -C $(LIB);       \
	$(MAKE) -C $(LIB) install DESTDIR=$(BCM_FSBUILD_DIR)/public

clean:
	@if [ -e $(LIB)/Makefile ]; then \
	  $(MAKE) -C $(LIB) clean ;      \
	fi
	rm -rf $(LIB)
