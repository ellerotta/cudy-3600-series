EXE := scratchpadctl

OBJS := scratchpadctl.o

LIBS = -lbcm_flashutil -lbcm_boardctl
LIBS += -lbcm_util -lsys_util -lgen_util -lrt


default install: all
	mkdir -p $(EXE_INSTALL_DIR)
	install -p -m 755 $(EXE) $(EXE_INSTALL_DIR)
	(cd $(EXE_INSTALL_DIR); rm -f pspctl; ln -s scratchpadctl pspctl)

all: $(EXE)

$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_RPATH_OPTION) $(BCM_RPATH_LINK_OPTION) $(CMS_LIB_PATH) $(LIBS)


clean:
	rm -f *.o *.d $(EXE)
	rm -f $(EXE_INSTALL_DIR)/$(EXE)
	rm -f $(EXE_INSTALL_DIR)/pspctl


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
