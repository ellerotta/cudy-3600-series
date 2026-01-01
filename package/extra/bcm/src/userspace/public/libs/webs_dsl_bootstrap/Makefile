
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

APP := webs_dsl_bootstrap

WEBS_DIR := $(BCM_FSINSTALL_DIR)/webs
WEBS_LIB_DIR := $(WEBS_DIR)/lib
WEBS_LIB_PUBLIC_DIR := $(WEBS_LIB_DIR)/public
WEBS_LIB_PRIVATE_DIR := $(WEBS_LIB_DIR)/private

ifeq "$(BRCM_DSL_PHY_BAS)" "y"
conditional_build: cpall
	@echo "done $@"

SRC_NAME := bootstrap-4.3.1-dist
SRC_ZIP := $(SRC_NAME).zip
SRC_FOLDER := $(SRC_NAME)
WEBS_TARGET_FOLDER := bootstrap
SRCS = $(SRC_FOLDER)/css/bootstrap.min.css

.PHONY: cpall check_src_zip clean_src clean_webs $(SRCS)
cpall: $(SRCS)
	@mkdir -p $(WEBS_LIB_PUBLIC_DIR)/$(WEBS_TARGET_FOLDER)
	@$(INSTALL) -D -m 664 -t $(WEBS_LIB_PUBLIC_DIR)/$(WEBS_TARGET_FOLDER) $(SRCS)

$(SRCS): check_src_zip
#	@echo running $@
	@test -f $@

check_src_zip: $(SRC_ZIP)
	@if [ ! -d $(SRC_FOLDER) ]; then \
		echo "Unzipping original $(SRC_ZIP) source"; \
		( unzip -o ./$(SRC_ZIP) ) \
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
