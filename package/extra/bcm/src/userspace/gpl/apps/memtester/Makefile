all dynamic install: conditional_build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

APP_VER=4.5.0
APP_BIN= memtester
APP = $(APP_BIN)-$(APP_VER)

ifneq ($(strip $(BUILD_MEMTESTER)),)
conditional_build: $(APP)
else
conditional_build:
	@echo "skipping $(APP_BIN) (not configured)"
endif

$(APP)/Makefile:
	tar -xvzf $(APP).tar.gz
	echo "$(CC) -O2 -DPOSIX -D_POSIX_C_SOURCE=200809L -D_FILE_OFFSET_BITS=64 -DTEST_NARROW_WRITES -c" > $(APP)/conf-cc
	echo "$(CC) -s" > $(APP)/conf-ld
	@echo "$(APP) is untarred and patched."

$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP) $(APP_BIN)
	$(INSTALL) $(APP)/$(APP_BIN) $(INSTALL_DIR)/bin/.
	@echo "Done building $(APP)"

clean:
	rm -rf $(APP)

distclean: clean
