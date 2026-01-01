
all dynamic install: conditional_build

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

VER = 2.0.6
APP = minicoredumper
APP_TAR = $(APP)-$(VER).tar.xz

.PHONY: conditional_build

ifneq ($(strip $(BUILD_MINICOREDUMPER)),)
# Depend on tar file
# Use $(APP)/untar_and_patch to mark tar progress is completed
$(APP)/untar_and_patch: $(APP_TAR) Makefile
	rm -rf $(APP) && \
	mkdir $(APP) && \
	tar --strip-components=1 -xf $(APP_TAR) -C $(APP)
	touch $@
	@echo "Applying patches to $(APP)"
	@echo "$(APP) is untarred"

# Depend on $(APP)/untar_and_patch so that tar progress should be done only once
$(APP)/Makefile: $(APP)/untar_and_patch
	cd $(APP) && \
	./configure --host=$(TOOLCHAIN_PREFIX) \
	--prefix= \
	CFLAGS="-I$(BCM_FSBUILD_DIR)/gpl/include/ -I$(BCM_FSBUILD_DIR)/public/include/" \
	LDFLAGS="-L$(BCM_FSBUILD_DIR)/gpl/lib/ -L$(BCM_FSBUILD_DIR)/public/lib/" LIBS=-lz; cd ..
	@echo "$(APP) is configured"

$(APP): $(APP)/Makefile
	$(MAKE) -C $(APP)

conditional_build: $(APP)
	install -m 755 $(CURR_DIR)/minicoredumper/src/minicoredumper/minicoredumper $(INSTALL_DIR)/bin/minicoredumper
	mkdir -p $(INSTALL_DIR)/etc/minicoredumper
	sed 's/\"in_tar\": true/\"in_tar\": false/' $(CURR_DIR)/minicoredumper/etc/minicoredumper/generic.recept.json > $(INSTALL_DIR)/etc/minicoredumper/generic.recept.json
	sed -e '/generic.recept.json/c\            \"recept\": \"/etc/minicoredumper/generic.recept.json\"' \
            $(CURR_DIR)/minicoredumper/etc/minicoredumper/minicoredumper.cfg.json > $(INSTALL_DIR)/etc/minicoredumper/minicoredumper.cfg.json
	@echo "Done building $(APP)"
else
conditional_build: sanity_check
	@echo "skipping $(APP) (not configured)"
endif


# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	@echo "Cleaning  minicoredumper..."
	rm -f $(INSTALL_DIR)/bin/minicoredumper
	rm -rf $(INSTALL_DIR)/etc/minicoredumper/
	rm -rf $(APP)

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
