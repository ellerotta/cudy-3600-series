# Makefile file for bftpd
#
# License:  GNU General Public License, Version 2.
#
EXE = bftpd


OBJS=commands.o list.o login.o main.o mystring.o

LIBS= -lcms_msg $(CMS_COMMON_LIBS) $(BCM_FLASH_LIBS)

all dynamic: sanity_check $(EXE) generic_exe_install

# Remove symlinks so customers do not import them into their source control systems
clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)
	rm -f INSTALL README

# this removes all non-source controlled files (this list is not complete)
distclean: clean
	rm -rf doc CHANGELOG INSTALL install-sh .project .settings \
	README Makefile.in debian .cdtproject configure configure.in \
	targzip.h



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
# GPL apps and libs are only allowed to include header files from the
# gpl and public directories
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BCM_FSBUILD_DIR)/public/include \
                         -I$(SHARED_DIR)/opensource/include/bcm963xx


#
# GPL apps and libs are only allowed to link with libraries from the
# gpl and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/usr/lib:/public/lib


ifeq ($(strip $(BRCM_USER_SSP)),y)
CFLAGS += $(SSP_MIN_COMPILER_OPTS)
LIBS += $(SSP_LIBS)
endif


#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): CHANGELOG $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_RPATH_OPTION) $(BCM_LIB_PATH) $(LIBS)


#
# If we have not untar'd the source, this file will be missing.
# Go untar it now.
#
CHANGELOG:
	@echo untaring source files
	cd ..; (tar xkfj ftpd.tar.bz2 2> /dev/null || true)



# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
