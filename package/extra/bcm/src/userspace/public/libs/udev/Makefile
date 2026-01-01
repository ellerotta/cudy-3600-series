# Makefile to build libudev
LIB = eudev-3.2.11

check_untar_configure: 
	if [ ! -e $(LIB)/Makefile.in ]; then              \
	  echo "Untarring source and overrides..." ;      \
	  (tar xzf $(LIB).tar.gz 2> /dev/null || true) ; \
	  cd $(LIB);                                      \
	  ./autogen.sh;          \
	  ./configure --host=${TOOLCHAIN_PREFIX} --prefix= --enable-shared; \
	fi

all: check_untar_configure
	@echo "Making $(LIB)"
	$(MAKE) -C $(LIB)

install: all
	@echo "Installing $(LIB) to $(INSTALL_DESTDIR)"
	$(MAKE) -C $(LIB) install DESTDIR=$(INSTALL_DESTDIR)
clean:
	@if [ -e $(LIB)/Makefile ]; then \
	  $(MAKE) -C $(LIB) clean ;      \
	fi
	rm -rf $(LIB)
