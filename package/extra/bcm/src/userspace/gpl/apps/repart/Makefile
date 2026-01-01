SCRIPT = cooked/$(REPART_SCRIPT_COOKED)
EXE  = blkdev_part

OBJS = blkdev_part.o


all install : $(EXE) $(SCRIPT)
	mkdir -p $(PREFIX)/bin
	install -p -m 755 $(EXE) $(PREFIX)/bin
	install -p -m 755 $(SCRIPT) $(PREFIX)/bin/$(REPART_SCRIPT_COOKED)


$(EXE): $(OBJS)
	$(CC) $(BCM_LD_FLAGS) -o $@ $(OBJS) $(CMS_OPTION_RPATH)

cooked/%: scripts/%
	mkdir -p cooked
	cat $< \
	| sed -e "s/__MIN_BOOTFS_SIZE_MB__/$(REPART_MIN_BOOTFS_SIZE_MB)/" \
	    > $@

clean: 
	-rm -f *.o *.d $(EXE) $(PREFIX)/bin/$(EXE) $(PREFIX)/bin/$(REPART_SCRIPT_COOKED)


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
