EXE := send_cms_msg

OBJS := send_cms_msg.o

LIBS = -lcms_msg $(CMS_COMMON_LIBS)

CFLAGS += -Werror -Wfatal-errors


default install: all
	mkdir -p $(EXE_INSTALL_DIR)
	install -p -m 755 $(EXE) $(EXE_INSTALL_DIR)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_RPATH_OPTION) $(BCM_RPATH_LINK_OPTION) $(CMS_LIB_PATH) $(LIBS)


clean:
	rm -f *.o *.d $(EXE)
	rm -f $(EXE_INSTALL_DIR)/$(EXE)


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
