LIB := libframework_cli.so

OBJS := framework_cli.o cmdedit.o

CFLAGS += -Werror -Wfatal-errors
# allow cast of 64 bit void * key to integer in compare function.
CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast

# These are public API headers needed by other apps which want to call this lib.
# Private/internal headers should not be listed here.
HEADERS := framework_cli.h

# framework_cli needs the following libs.  Link with them here so apps which
# link with framework_cli does not need to explicitly link with them.  (app must use BCM_RPATH_LINK_OPTION).
NEEDED_LIBS := -lbcm_util


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
