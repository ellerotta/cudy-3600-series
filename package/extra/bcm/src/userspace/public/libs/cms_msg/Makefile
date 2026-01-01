LIB = libcms_msg.so

OBJS = msg.o addon_misc.o addon_configfile.o


# BRCM_SUPPORTS_MULTIARCH_BUILD

# treat all warnings as errors
CFLAGS += -Werror -Wfatal-errors
# for dealing with 64 bit int/pointer field in CmsMsgHeader
CFLAGS += -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast


# These are public API headers needed by other apps which want to call this lib.
# Private/internal headers should not be listed here.
HEADERS := cms_msg.h cms_msg_modsw.h cms_msg_pubsub.h \
           cms_msg_remoteobj.h cms_msg_usp.h

all: $(LIB)

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	install -p -t $(LIB_INSTALL_DIR) $(LIB)
	mkdir -p $(HEADER_INSTALL_DIR)
	$(INSTALL_HEADERS_WITH_CP) $(addprefix $(mdir)/,$(HEADERS)) $(HEADER_INSTALL_DIR)

$(LIB): $(OBJS) force_oal
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) \
	      -o $@ $(OBJS) $(OALDIR)/oal.a -Wl,--no-whole-archive

# At this level, we don't know if anything in oaldir has changed, so always go
# down into that dir and do a make.
force_oal:
	mkdir -p $(OALDIR)
	$(MAKE) -C $(OALDIR) -f $(mdir)/$(OALDIR)/Makefile


clean: 
	rm -f *.o *.d $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
