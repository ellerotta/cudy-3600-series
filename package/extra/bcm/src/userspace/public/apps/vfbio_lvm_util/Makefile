#
# In most cases, you only need to modify this first section.
#
EXE = vfbio_lvm

OBJS = vfbio_lvm_util.o


all dynamic install: $(EXE) generic_exe_install

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)

.PHONY: all dynamic install

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
                         -I$(KERNEL_LINKS_DIR)

#
# Public apps and libs are allowed to link with libraries from the
# public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib
LIB_PATH = -L $(INSTALL_DIR)$(subst :,:$(INSTALL_DIR),$(ALLOWED_LIB_DIRS))
LIBS += -lbcm_flashutil -lbcm_boardctl -lbcm_util -lsys_util -lgen_util

# treat all warnings as errors
CUSTOM_CFLAGS += -Werror -Wfatal-errors

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
#
$(EXE): $(OBJS)
	@echo building $@
	$(CC) $(BCM_LD_FLAGS) -o $@ $^ $(LIB_PATH) $(LIBS)



#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)
