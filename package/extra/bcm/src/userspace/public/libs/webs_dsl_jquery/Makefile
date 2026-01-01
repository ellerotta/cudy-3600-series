
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

APP := webs_dsl_jquery

WEBS_DIR := $(BCM_FSINSTALL_DIR)/webs
WEBS_LIB_DIR := $(WEBS_DIR)/lib
WEBS_LIB_PUBLIC_DIR := $(WEBS_LIB_DIR)/public

ifeq "$(BRCM_DSL_PHY_BAS)" "y"
conditional_build: cpall
	@echo "done $@"

.PHONY: cpall check_src_tar clean_src clean_webs

WEBS_JQUERY_DIR := $(WEBS_LIB_PUBLIC_DIR)/jquery
JQUERY_SRC := jquery-3.6.0.min.js
JQUERY_FN := jquery.min.js

cpall: $(JQUERY_SRC)
	@cp -vf ./$(JQUERY_SRC) ./$(JQUERY_FN)
	@mkdir -p $(WEBS_JQUERY_DIR)
	@$(INSTALL) -v -D -m 664 -t $(WEBS_JQUERY_DIR) $(JQUERY_FN)

clean: clean_src clean_webs generic_clean

clean_src:
	@echo "removing $(JQUERY_FN)"
	-rm -f ./$(JQUERY_FN)

clean_webs:
	@echo "remove installed files from webs directory"
	-rm -rf $(WEBS_JQUERY_DIR)/$(JQUERY_FN)

else
conditional_build_dsl_jquery: sanity_check
	@echo "skipping $(APP) - not configured"
endif

clean:

distclean: clean
