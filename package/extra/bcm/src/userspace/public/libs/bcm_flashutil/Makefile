LIB := libbcm_flashutil.so

OBJS := bcm_flashutil.o bcm_flashutil_nand.o \
        bcm_imgutil.o bcm_imgutil_api.o bcm_imgcombo.o bcm_imgif.o \
        image.o led.o psp.o


ifeq ($(strip $(DESKTOP_LINUX)),)
OBJS += bcm_flashutil_emmc.o  bcm_imgif_nand.o bcm_imgif_emmc.o bcm_imgif_pkgtb.o bcm_flashutil_nor.o
ifeq ($(strip $(BUILD_VFBIO_IMG)),y)
OBJS += bcm_flashutil_vfbio.o
endif
endif

ifeq ($(strip $(BUILD_VFBIO_IMG)),y)
OBJS += bcm_vfbio_api.o
endif

BCM_LD_FLAGS += -Wl,-Bstatic -Wl,--whole-archive -lfdt -L$(BCM_FSBUILD_DIR)/public/lib $(BCM_EXTRA_STATIC_LIBS) -Wl,-Bdynamic -lbcm_boardctl

CFLAGS += -Werror -Wfatal-errors -Wno-attributes

# These are public API headers needed by other apps which want to call this lib.
# Private/internal headers should not be listed here.
HEADERS := bcm_flashutil.h bcm_imgif.h bcm_imgutil.h bcm_imgutil_api.h \
           bcm_imgutil_def.h \
           cms_image.h cms_led.h cms_psp.h


all: $(LIB)

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	install -p -t $(LIB_INSTALL_DIR) $(LIB)
	mkdir -p $(HEADER_INSTALL_DIR)
	$(INSTALL_HEADERS_WITH_CP) $(addprefix $(mdir)/,$(HEADERS)) $(HEADER_INSTALL_DIR)

clean:
	rm -f *.o *.d $(LIB)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)
	rm -f $(addprefix $(HEADER_INSTALL_DIR)/,$(HEADERS))

$(LIB): $(OBJS)
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) \
	      -o $@ $(OBJS) -Wl,-ldl -Wl,--no-whole-archive


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
