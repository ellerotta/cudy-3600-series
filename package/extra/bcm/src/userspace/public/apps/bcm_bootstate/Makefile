EXE := bcm_bootstate

OBJS := bcm_bootstate.o

LIBS := -lbcm_flashutil -lbcm_boardctl -lbcm_util -lsys_util -lgen_util -lrt

CFLAGS += -Werror -Wfatal-errors

default: install

all: $(EXE)

install: all
	mkdir -p $(EXE_INSTALL_DIR)
	install -p -m 755 $(EXE) $(EXE_INSTALL_DIR)

clean:
	rm -f *.o *.d $(EXE)
	rm -f $(EXE_INSTALL_DIR)/$(EXE)


$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_RPATH_OPTION) $(CMS_LIB_PATH) $(LIBS)


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
