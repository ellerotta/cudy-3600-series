
.PHONY: all dynamic install conditional_build clean distclean

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

APP := webs_dsl_chartjs

WEBS_DIR := $(BCM_FSINSTALL_DIR)/webs
WEBS_LIB_DIR := $(WEBS_DIR)/lib
WEBS_LIB_PUBLIC_DIR := $(WEBS_LIB_DIR)/public
WEBS_LIB_PRIVATE_DIR := $(WEBS_LIB_DIR)/private

ifeq "$(BRCM_DSL_PHY_BAS)" "y"
conditional_build: cpall
	@echo "done $@"

SRC_TAR := chart.js-2.9.4.tgz
SRC_FOLDER := chart.js

WEBS_TARGET_FOLDER := chart.js

# Note: have to list files of interest manually
# since we might not have untarred the sources yet
SRCS = $(SRC_FOLDER)/dist/Chart.min.js \
		$(SRC_FOLDER)/dist/Chart.min.css

.PHONY: cpall check_src_tar clean_src clean_webs $(SRCS)
cpall: $(SRCS)
	@mkdir -p $(WEBS_LIB_PUBLIC_DIR)/$(WEBS_TARGET_FOLDER)
	@$(INSTALL) -D -m 664 -t $(WEBS_LIB_PUBLIC_DIR)/$(WEBS_TARGET_FOLDER) $(SRCS)

$(SRCS): check_src_tar
#	@echo running $@
	@test -f $@

check_src_tar: $(SRC_TAR)
	@if [ ! -d $(SRC_FOLDER) ]; then \
		echo "Untarring original $(SRC_TAR) source"; \
		mkdir -p $(SRC_FOLDER); \
		( tar --strip-components=1 -xzf $(SRC_TAR) -C $(SRC_FOLDER) ); \
	fi

clean: clean_src clean_webs generic_clean

clean_src:
	@echo "remove untarred files in $(SRC_FOLDER)"
	-rm -rf ./$(SRC_FOLDER)

clean_webs:
	@echo "remove installed files from webs directory"
	-rm -rf $(WEBS_LIB_PUBLIC_DIR)/$(WEBS_TARGET_FOLDER)

else
conditional_build_dsl_jquery: sanity_check
	@echo "skipping $(APP) - not configured"
endif

clean:

distclean: clean
