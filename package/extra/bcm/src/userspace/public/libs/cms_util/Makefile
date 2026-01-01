LIB  := libcms_util.so

OBJS := eid.o logging.o memory.o bget.o timestamp.o timer.o \
       eid_parser.o strconv.o prctl.o \
       bcm_generic_hal_utils.o \
       assert.o network.o file.o \
       base64_wrapper.o hexbinary_wrapper.o \
       lzw_encode.o lzw_decode.o xml.o passwd.o \
       vbuf.o \
       tokenizer.o errorcodes.o \
       ftrace.o linklist.o random.o unicode.o \
       pcp.o data_model_selector.o \
       strconv2.o math.o md5.o \
       dhcp_common.o
	   
ifneq ($(strip $(BUILD_NETAPP_RESTRICT)),)
OBJS += security_app.o
endif

# if you want to use CMS_MEM_LEAK_TRACING feature on ARM based platforms,
# uncomment out the following line.  But to avoid complicated license
# issues, do NOT compile this file into your production code.
# OBJS += backtrace_arm.o


# These are public API headers needed by other apps which want to call this lib.
# Private/internal headers should not be listed here.
HEADERS := bcm_generic_hal_utils.h cms_ast.h cms_base64.h \
          cms_data_model_selector.h cms_dhcp_common.h cms_dlist.h \
          cms_eid.h cms_errorcodes.h  cms_fil.h cms_ftrace.h cms_hexbinary.h \
          cms_linklist.h cms_log.h cms_lzw.h \
          cms_math.h cms_mem.h cms_net.h cms_pwd.h \
          cms_rand.h cms_strconv.h cms_strconv2.h cms_tmr.h cms_tms.h \
          cms_tokenizer.h cms_unicode.h cms_util.h cms_utilc.h cms_vbuf.h \
          cms_xml.h md5.h pcp.h prctl.h 
		  
ifneq ($(strip $(BUILD_NETAPP_RESTRICT)),)
HEADERS += security_app.h
endif


# image.c needs to know what chip we are using
CFLAGS += -DBRCM_CHIP_HEX=0x$(BRCM_CHIP)
CFLAGS += -Werror -Wfatal-errors

# OS abstraction layer
OALDIR := linux

# EID symbol table
SYMBOL_TAB := symbol_table.txt

# EID files in the scripts directory
ifneq ($(strip $(BUILD_BRCM_CMS))$(strip $(BUILD_DISTRIBUTED_MDM)),)
EID_FILES := eid_bcm_base.txt \
             eid_bcm_mgmt.txt
endif

# Note in CMS, these eid_bcm_* files will cause smd to launch these apps.
# In BDK, the wifi and voice components will launch the apps so these files
# must NOT be installed in BDK build.
ifneq ($(strip $(BUILD_BRCM_CMS)),)
EID_FILES += eid_bcm_voip.txt eid_bcm_wifi.txt eid_bcm_audio.txt
endif


all: $(LIB) $(SYMBOL_TAB)

install: all
	mkdir -p $(LIB_INSTALL_DIR)
	install -p -t $(LIB_INSTALL_DIR) $(LIB)
	mkdir -p $(HEADER_INSTALL_DIR)
	$(INSTALL_HEADERS_WITH_CP) $(addprefix $(mdir)/,$(HEADERS)) $(HEADER_INSTALL_DIR) 
	mkdir -p $(FINAL_ETC_INSTALL_DIR)
	cp -upf $(SYMBOL_TAB) $(addprefix $(mdir)/scripts/,$(EID_FILES)) $(FINAL_ETC_INSTALL_DIR)

clean:
	rm -f *.o *.d $(LIB)
	rm -f $(OALDIR)/oal.a $(OALDIR)/*.o $(OALDIR)/*.d
	rm -f $(SYMBOL_TAB) $(REALTIME_LEVELS)
	rm -f $(LIB_INSTALL_DIR)/$(LIB)
	rm -f $(addprefix $(HEADER_INSTALL_DIR)/,$(HEADERS))
	rm -f $(addprefix $(FINAL_ETC_INSTALL_DIR)/,$(EID_FILES))
	rm -f $(FINAL_ETC_INSTALL_DIR)/$(SYMBOL_TAB)

# grab some #defines from cms_eid.h and cms_params.h so that symbols can
# be used in the eid files
SYMBOL_TABLE_SRCS := cms_eid.h \
                     $(BCM_FSBUILD_DIR)/public/include/cms_params.h

$(SYMBOL_TAB): $(SYMBOL_TABLE_SRCS)
	@echo Generating $@
	@egrep 'EID_[[:alnum:]_-]+=' $(mdir)/cms_eid.h | sed -e 's/,//g' | sed -e 's/=/ /g' > $@
	@egrep 'NDA_ACCESS_[[:alnum:]_\t\ ]+0x' $(mdir)/cms_eid.h | sed -e 's/#define//g' >> $@
	@grep '#define EIF_' $(mdir)/cms_eid.h | sed -e 's/#define//g' >> $@
	@grep '#define FTPD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define TFTPD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define SSHD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define TELNETD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define HTTPD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define SNMPD_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define TR69C_CONN_REQ_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define TR64C_HTTP_CONN_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@
	@grep '#define TR69C_2_CONN_REQ_PORT' $(BCM_FSBUILD_DIR)/public/include/cms_params.h | sed -e 's/#define//g' >> $@


$(LIB): $(OBJS) force_oal
	$(CC) -shared $(BCM_LD_FLAGS) -Wl,--whole-archive,-soname,$(notdir $@) \
	    -o $@ $(OBJS) $(OALDIR)/oal.a -Wl,-lcrypt -lm -ldl -Wl,--no-whole-archive

# At this level, we don't know if anything in oaldir has changed, so always go
# down into that dir and do a make.
force_oal:
	mkdir -p $(OALDIR)
	$(MAKE) -C $(OALDIR) -f $(mdir)/$(OALDIR)/Makefile


# Set VPATH because we want to compile in a seperate dir than source.
name := $(lastword $(MAKEFILE_LIST))
mdir := $(realpath $(dir $(name)))
VPATH=$(mdir)

# Generate and use dependencies.
CFLAGS += -MD
-include $(OBJS:%.o=%.d)
