LIB := libeyescope.so

OBJS := d6s_eye.o

all: $(LIB)

# Set VPATH so we can compile in a separate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OBJS) -Wl,--no-whole-archive

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	install -p -m 755 -t $(LIB_INSTALL_DIR) $(LIB)

clean:
	rm -f *.d *.o $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)

# Generate and use dependencies.
CFLAGS += -MD -Werror
-include $(OBJS:%.o=%.d)
