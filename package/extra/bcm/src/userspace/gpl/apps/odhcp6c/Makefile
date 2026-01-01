# Makefile to build odhcp6c
APP = odhcp6c-2015-07-29

check_config: $(APP).tar.bz2
	if [ ! -e $(APP)/Makefile ]; then \
		echo "Untarring odhcp6c source ..." ; \
		(tar xjf $(APP).tar.bz2 2> /dev/null || true) ; \
		echo "Applying patches to $(APP)" ; \
		cd $(APP); \
		ln -sf ../patches .; \
		if [ ! -e patches/series ]; then (cd patches; ls *.patch > series;); fi; \
		quilt push -a || exit $$?; \
	fi

install: check_config
	cd $(APP); \
	cmake . && $(MAKE) || exit $?; \

clean:
	[ ! -e $(APP)/Makefile ] || $(MAKE) -C $(APP) clean
	rm -rf $(APP)
