APP = alsa-plugins-1.2.1

check_untar_configure:
	if [ ! -e $(APP)/Makefile.in ]; then              \
	  echo "Untarring source and overrides..." ;      \
	  (tar xfj $(APP).tar.bz2 2> /dev/null || true) ; \
	  cd $(APP);	                                    \
	  ./autogen.sh ;                                  \
	  ./configure --host=${TOOLCHAIN_PREFIX} --prefix=$(BCM_FSBUILD_DIR)/public --enable-shared --disable-oss --disable-mix               \
	              --with-speex=no --disable-usbstream --disable-arcamav --disable-jack --disable-pulseaudio --disable-avcodec --disable-speexdsp;          \
	fi

install: check_untar_configure
	@echo "Making $(APP)"
	@cd $(APP); \
	make;       \
	make install

clean:
	@if [ -e $(APP)/Makefile ]; then \
	  cd $(APP);                     \
	  $(MAKE) clean ;                \
	fi
	rm -rf $(APP)
