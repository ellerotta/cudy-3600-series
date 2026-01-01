all dynamic install: conditional_build

LIB := libgrpc
TARBALL := v1.48.0.tar.gz
UNTAR_FOLDER := grpc-1.48.0
CMAKE_TOOLCHAIN := linux.cmake

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

CMAKE := $(if $(shell which cmake3),cmake3,cmake)
PREFIX = $(BCM_FSBUILD_DIR)/public

export PATH := $(BCM_FSBUILD_DIR)/host/bin:$(PATH)

$(UNTAR_FOLDER)/cmake: $(TARBALL)
	@if [ ! -e $@ ]; then \
	    echo "Extract original $< source"; \
	    (tar xf $< 2>/dev/null); \
	    echo "Extract third party libraries"; \
	    (tar xf third_party.tar.gz 2>/dev/null); \
	fi

build/host/Makefile: build/host/$(CMAKE_TOOLCHAIN) $(BCM_FSBUILD_DIR)/host/bin/protoc $(UNTAR_FOLDER)/cmake
	@mkdir -p $(dir $@)
	(cd $(dir $@); \
		$(CMAKE) ../../$(UNTAR_FOLDER) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN) \
			-DCMAKE_INSTALL_PREFIX=$(BCM_FSBUILD_DIR)/host \
			-DgRPC_INSTALL=ON \
			-DgRPC_BUILD_TESTS=OFF \
			-DgRPC_SSL_PROVIDER=package \
	)

$(UNTAR_FOLDER)/third_party/protobuf/configure: $(UNTAR_FOLDER)/cmake
	cd $(dir $@) && ./autogen.sh

$(UNTAR_FOLDER)/third_party/protobuf/Makefile: $(UNTAR_FOLDER)/third_party/protobuf/configure
	cd $(dir $@) && ./configure --prefix=$(BCM_FSBUILD_DIR)/host CC=gcc CXX=g++ AR=ar LD=ld

$(UNTAR_FOLDER)/third_party/protobuf/src/protoc: $(UNTAR_FOLDER)/third_party/protobuf/Makefile
	@$(MAKE) -C $(dir $@) install

$(BCM_FSBUILD_DIR)/host/bin/protoc: $(UNTAR_FOLDER)/third_party/protobuf/src/protoc
	@echo " HOST INSTALL    $(notdir $@)"
	-@mkdir -p $(dir $@) 2>/dev/null || true
	@cp -f $< $@
	@touch $@

$(BCM_FSBUILD_DIR)/host/bin/grpc_cpp_plugin: build/host/Makefile $(BCM_FSBUILD_DIR)/host/bin/protoc
	@$(MAKE) -C build/host grpc_cpp_plugin
	@echo " HOST INSTALL    $(notdir $@)"
	@cp -f build/host/grpc_cpp_plugin $@
	@touch $@

build/host/$(CMAKE_TOOLCHAIN): Makefile
	@mkdir -p $(dir $@)
	@echo "set(CMAKE_SYSTEM_NAME Linux)" > $@
	@echo "set(CMAKE_C_COMPILER gcc)" >> $@
	@echo "set(CMAKE_CXX_COMPILER g++)" >> $@

build/target/$(CMAKE_TOOLCHAIN): Makefile
	@mkdir -p $(dir $@)
	@echo "set(CMAKE_SYSTEM_NAME Linux)" > $@
	@echo "set(CMAKE_SYSTEM_PROCESSOR $(ARCH))" >> $@
	@echo "set(CMAKE_PROGRAM_PATH $(TOOLCHAIN_TOP)/$(TOOLCHAIN_USR_DIR)/bin)" >> $@
	@echo "set(CMAKE_C_COMPILER $(TOOLCHAIN_PREFIX)-gcc)" >> $@
	@echo "set(CMAKE_CXX_COMPILER $(TOOLCHAIN_PREFIX)-g++)" >> $@
	@echo "set(CMAKE_FIND_ROOT_PATH $(TOOLCHAIN_SYSROOT) $(INSTALL_DIR))" >> $@
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)" >> $@
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)" >> $@
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)" >> $@
	@echo "set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)" >> $@

build/target/Makefile: build/target/$(CMAKE_TOOLCHAIN) $(UNTAR_FOLDER)/cmake $(BCM_FSBUILD_DIR)/host/bin/grpc_cpp_plugin
	@mkdir -p $(dir $@)
	(cd $(dir $@); \
		$(CMAKE) ../../$(UNTAR_FOLDER) \
			-DCMAKE_TOOLCHAIN_FILE=$(CMAKE_TOOLCHAIN) \
			-DCMAKE_BUILD_TYPE=Release \
			-DgRPC_BUILD_TESTS=OFF \
			-DgRPC_BUILD_CODEGEN=OFF \
			-DgRPC_BUILD_CSHARP_EXT=OFF \
			-DgRPC_BUILD_GRPC_CSHARP_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_NODE_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_OBJECTIVE_C_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_PHP_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_PYTHON_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_RUBY_PLUGIN=OFF \
			-DgRPC_BUILD_GRPC_CPP_PLUGIN=OFF \
			-DgRPC_SSL_PROVIDER=package \
			-DBUILD_SHARED_LIBS=OFF \
			-DOPENSSL_ROOT_DIR=$(PREFIX)/lib \
			-DOPENSSL_CRYPTO_LIBRARY=$(PREFIX)/lib/libcrypto.so \
			-DOPENSSL_SSL_LIBRARY=$(PREFIX)/lib/libssl.so \
			-DOPENSSL_INCLUDE_DIR=$(PREFIX)/include \
			-DCMAKE_INSTALL_PREFIX=$(PREFIX) \
			-DCMAKE_POSITION_INDEPENDENT_CODE=TRUE \
	)


ifneq ($(strip $(BUILD_GRPC)),)
conditional_build: build/target/Makefile sanity_check
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	$(MAKE) -C build/target install
else
conditional_build:
	@echo "skipping $(LIB) (not configured)"
endif

clean:
	rm -rf build $(UNTAR_FOLDER)

#
# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean

bcm_dorel_distclean: distclean

shell:
	bash -i
