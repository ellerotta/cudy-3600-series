LIBOUT := libxchg.a

all: install

CURR_DIR  := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

# Must be included after main target
include $(BUILD_DIR)/make.common

# Submodules that will be built.  Each submodule must define <NAME>_SRCS and 
# <NAME>_DEFS and have <name>.mk that matches the directory name.
SUBMODULES = assert bos str

export ASSERT_ROOT := $(CURR_DIR)/assert
export BOS_ROOT    := $(CURR_DIR)/bos
export STR_ROOT    := $(CURR_DIR)/str

$(foreach sub, $(SUBMODULES), $(eval include $(sub)/$(sub).mk))

CFLAGS += $(ASSERT_DEFS) $(BOS_DEFS) $(STR_DEFS)

ALL_SRCS := $(addprefix assert/,$(ASSERT_SRCS)) \
            $(addprefix bos/,$(BOS_SRCS)) \
            $(addprefix str/,$(STR_SRCS))

ALL_OBJS := $(patsubst %.c,%.o,$(ALL_SRCS))

HEADER_INSTALL_DIR := $(BCM_FSBUILD_DIR)/private/include/xchg
LIB_INSTALL_DIR    := $(BCM_FSBUILD_DIR)/private/lib

# Targets
$(LIBOUT): $(ALL_OBJS)
	$(AR) rcs $@ $^

install: $(LIBOUT)
	mkdir -p $(HEADER_INSTALL_DIR)/LinuxUser
	$(INSTALL_HEADERS_WITH_CP) $(CURR_DIR)/assert/inc/* $(HEADER_INSTALL_DIR)
	$(INSTALL_HEADERS_WITH_CP) $(CURR_DIR)/bos/publicInc $(HEADER_INSTALL_DIR)
	$(INSTALL_HEADERS_WITH_CP) $(CURR_DIR)/bos/LinuxUser/*.h $(HEADER_INSTALL_DIR)/LinuxUser
	$(INSTALL_HEADERS_WITH_CP) $(CURR_DIR)/str/inc/* $(HEADER_INSTALL_DIR)

	mkdir -p $(LIB_INSTALL_DIR)
	install -p -m 755 -t $(LIB_INSTALL_DIR) $(LIBOUT)

clean: 
	rm -f $(LIBOUT) $(ALL_OBJS)
	rm -rf $(HEADER_INSTALL_DIR)
	rm -f $(LIB_INSTALL_DIR)/$(LIBOUT)
