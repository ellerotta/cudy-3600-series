EXE := pw_wrap

all install: conditional_build 


CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

FINAL_EXE_INSTALL_DIR := $(INSTALL_DIR)/bin$(BCM_INSTALL_SUFFIX_DIR)

ifneq ($(strip $(BRCM_BUILD_PWWRAP)),)

conditional_build: winstall

else

conditional_build:
	@echo "pw_wrap not selected -- skipping"

endif

 # pw_wrap: pw_wrap.c

winstall: pw_wrap
	cp -p $(EXE) $(FINAL_EXE_INSTALL_DIR)

clean:
	rm -f $(FINAL_EXE_INSTALL_DIR)/$(EXE)
	rm -f $(EXE)


shell:
	@echo "Entering makefile debug shell (type exit to exit) >>>"
	@bash -i
	@echo "exiting debug shell."

