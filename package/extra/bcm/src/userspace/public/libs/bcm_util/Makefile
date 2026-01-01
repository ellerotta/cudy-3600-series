LIB = libbcm_util.so

OBJS = bcm_timestamp.o bcm_ulog.o bcm_net.o bcm_fsutils.o \
       bcm_boardutils.o bcm_ethswutils.o bcm_strutils.o

CFLAGS += -Werror -Wfatal-errors

# These are public API headers needed by other apps which want to call this lib.
# Private/internal headers should not be listed here.
HEADERS = bcm_ulog.h bcm_timestamp.h bcm_net.h bcm_fsutils.h \
          bcm_boardutils.h bcm_ethswutils.h bcm_strutils.h


# bcm_util needs the following libs.  Link with them here so apps which link
# with bcm_util do not need to explicitly link with them.  (app must use BCM_RPATH_LINK_OPTION).
NEEDED_LIBS := -lgen_util -lsys_util -ldl

# bcm_bootstate.h is included and the reset_status macros are different
# with or without CONFIG_BRCM_SMC_BASED defined
ifeq ($(findstring _$(strip $(BRCM_CHIP))_,_68880_6837_),_$(strip $(BRCM_CHIP))_)
CFLAGS += -DCONFIG_BRCM_SMC_BASED
endif

all: $(LIB)

install: $(LIB)
	mkdir -p $(LIB_INSTALL_DIR)
	install -t $(LIB_INSTALL_DIR) $(LIB)
	mkdir -p $(HEADER_INSTALL_DIR)
	install -p -m 444 -t $(HEADER_INSTALL_DIR) $(addprefix $(mdir)/,$(HEADERS))

clean:
	rm -f *.o *.d $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)
	rm -f $(addprefix $(HEADER_INSTALL_DIR)/,$(HEADERS))

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive \
          $(BCM_LIB_PATH) $(NEEDED_LIBS)


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
