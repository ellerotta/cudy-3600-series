SRC := lua-cjson-2.1.0
LIB := cjson
LIB_DIR := cjson

check_untar: $(SRC).tar.gz
	@if [ ! -e $(LIB_DIR)/Makefile ]; then \
		echo "Untarring original $(SRC) source"; \
		tar -xzf $(SRC).tar.gz; \
	fi

install: check_untar
	$(MAKE) -C $(SRC) install PREFIX=$(PREFIX) LUA_VERSION=""

clean:
	[ ! -d $(SRC) ] || $(MAKE) -C $(SRC) clean

distclean: clean
	rm -rf $(SRC)
