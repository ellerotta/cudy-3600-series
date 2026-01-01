# Normally, a private/lib/xyz directory has a Makefile and Makefile.fullsrc.
# This file is a combination of Makefile and Makefile.fullsrc. TODO: split into Makefile and Makefile.fullsrc.

LIB   := libatmctl.so

SAVED := $(LIB)_$(ARCH).shipped


all install: $(LIB)
	mkdir -p $(LIB_INSTALL_DIR)
	install -p -t $(LIB_INSTALL_DIR) $(LIB)

clean: 
	rm -f *.o *.d $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)
	-mkdir -p $(OALDIR)
	-$(MAKE) -C $(OALDIR) -f $(mdir)/$(OALDIR)/Makefile clean


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

ifneq ($(wildcard $(mdir)*.c)$(wildcard $(mdir)/$(OALDIR)/*.c),)
# Source file(s) present: do normal compile.
$(LIB): $(OBJS)
	mkdir -p $(OALDIR)
	$(MAKE) -C $(OALDIR) -f $(mdir)/$(OALDIR)/Makefile
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) -o $@ $(OALDIR)/oal.a -Wl,--no-whole-archive

else
# Binary only distribution: used saved pre-compiled file.
$(LIB): ../$(SAVED)
	cat $< > $@

endif


binaryonly_prepare:
	-cp $(LIB) ../$(SAVED)
	-$(MAKE) -f $(name) clean
	rm -rf $(mdir)/*.c $(mdir)/*.h $(mdir)/$(OALDIR)


# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
