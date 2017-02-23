BUILDDIR=./build
TOOLSDIR=./tools
HOST=linux

BIN=escargot
SHARED_LIB=libescargot.so
STATIC_LIB=libescargot.a

#######################################################
# Environments
#######################################################

ARCH=#x86,x64
TYPE=none#interpreter
MODE=#debug,release
NPROCS:=1
OS:=$(shell uname -s)
SHELL:=/bin/bash
OUTPUT=
LTO=
QUIET=@
ifeq ($(OS),Linux)
  NPROCS:=$(shell grep -c ^processor /proc/cpuinfo)
  SHELL:=/bin/bash
endif
ifeq ($(OS),Darwin)
  NPROCS:=$(shell sysctl -n machdep.cpu.thread_count)
  SHELL:=/opt/local/bin/bash
endif

$(info goal... $(MAKECMDGOALS))

ifneq (,$(findstring x86,$(MAKECMDGOALS)))
  ARCH=x86
else ifneq (,$(findstring x64,$(MAKECMDGOALS)))
  ARCH=x64
else ifneq (,$(findstring arm,$(MAKECMDGOALS)))
  ARCH=arm
endif

ifneq (,$(findstring interpreter,$(MAKECMDGOALS)))
  TYPE=interpreter
endif

ifneq (,$(findstring debug,$(MAKECMDGOALS)))
  MODE=debug
else ifneq (,$(findstring release,$(MAKECMDGOALS)))
  MODE=release
endif

ifneq (,$(findstring tizen_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_wearable
  VERSION=2.3.1
  TIZEN_PROFILE=wearable
else ifneq (,$(findstring tizen3_wearable_arm,$(MAKECMDGOALS)))
  HOST=tizen_3.0_wearable
  VERSION=3.0
  TIZEN_PROFILE=wearable
else ifneq (,$(findstring tizen_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_mobile
  VERSION=2.3.1
  TIZEN_PROFILE=mobile
else ifneq (,$(findstring tizen3_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_3.0_mobile
  VERSION=3.0
  TIZEN_PROFILE=mobile
else ifneq (,$(findstring tizen_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_2.3.1_wearable
  VERSION=2.3.1
  ARCH=i386
  TIZEN_PROFILE=wearable
else ifneq (,$(findstring tizen3_wearable_emulator,$(MAKECMDGOALS)))
  HOST=tizen_3.0_wearable
  VERSION=3.0
  ARCH=i386
  TIZEN_PROFILE=wearable
else ifneq (,$(findstring tizen24_mobile_emulator,$(MAKECMDGOALS)))
  HOST=tizen_2.4_mobile
  VERSION=2.4
  ARCH=i386
  TIZEN_PROFILE=mobile
else ifneq (,$(findstring tizen24_mobile_arm,$(MAKECMDGOALS)))
  HOST=tizen_2.4_mobile
  VERSION=2.4
  ARCH=arm
  TIZEN_PROFILE=mobile
else ifneq (,$(findstring tizen_obs_arm,$(MAKECMDGOALS)))
  HOST=tizen_obs
else ifneq (,$(findstring tizen_obs_i386,$(MAKECMDGOALS)))
  HOST=tizen_obs
  ARCH=i386
endif

ifneq (,$(findstring tizen,$(HOST)))
  #LTO=1
endif

ifneq (,$(findstring shared,$(MAKECMDGOALS)))
  OUTPUT=shared_lib
else ifneq (,$(findstring static,$(MAKECMDGOALS)))
  OUTPUT=static_lib
else
  OUTPUT=bin
endif

ifeq ($(HOST),tizen_obs)
  QUIET=
endif

OUTDIR=out/$(HOST)/$(ARCH)/$(TYPE)/$(MODE)
ESCARGOT_ROOT=.

$(info host... $(HOST))
$(info arch... $(ARCH))
$(info type... $(TYPE))
$(info mode... $(MODE))
$(info output... $(OUTPUT))
$(info build dir... $(OUTDIR))

#######################################################
# Build flags
#######################################################

include $(BUILDDIR)/Toolchain.mk
include $(BUILDDIR)/Flags.mk

# common flags
CXXFLAGS += $(ESCARGOT_CXXFLAGS_COMMON)
LDFLAGS += $(ESCARGOT_LDFLAGS_COMMON)

# HOST flags
ifeq ($(HOST), linux)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_LINUX)
else ifneq (,$(findstring tizen,$(HOST)))
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_TIZEN)
endif

# ARCH flags
ifeq ($(ARCH), x64)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X64)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X64)
else ifeq ($(ARCH), x86)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X86)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X86)
else ifeq ($(ARCH), i386)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_X86)
  LDFLAGS += $(ESCARGOT_LDFLAGS_X86)
else ifeq ($(ARCH), arm)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_ARM)
  LDFLAGS += $(ESCARGOT_LDFLAGS_ARM)
endif

# TYPE flags
ifeq ($(TYPE), interpreter)
  CXXFLAGS+=$(ESCARGOT_CXXFLAGS_INTERPRETER)
  CXXFLAGS+=$(ESCARGOT_CXXFLAGS_JIT)
endif

# MODE flags
ifeq ($(MODE), debug)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_DEBUG)
else ifeq ($(MODE), release)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_RELEASE)
endif

# OUTPUT flags
ifeq ($(OUTPUT), bin)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_BIN)
  LDFLAGS += $(ESCARGOT_LDFLAGS_BIN)
else ifeq ($(OUTPUT), shared_lib)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_SHAREDLIB)
  LDFLAGS += $(ESCARGOT_LDFLAGS_SHAREDLIB)
else ifeq ($(OUTPUT), static_lib)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_STATICLIB)
  LDFLAGS += $(ESCARGOT_LDFLAGS_STATICLIB)
endif

CXXFLAGS += $(ESCARGOT_CXXFLAGS_THIRD_PARTY)
LDFLAGS += $(ESCARGOT_LDFLAGS_THIRD_PARTY)

ifeq ($(VENDORTEST), 1)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_VENDORTEST)
endif

ifeq ($(LTO), 1)
  CXXFLAGS += $(ESCARGOT_CXXFLAGS_LTO)
  LDFLAGS += $(ESCARGOT_LDFLAGS_LTO)
  ifeq ($(OUTPUT), bin)
    LDFLAGS += $(CXXFLAGS)
  endif
endif

#######################################################
# SRCS & OBJS
#######################################################

SRC=
SRC += $(foreach dir, src , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/heap , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/interpreter , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/parser , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/parser/ast , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/parser/esprima_cpp , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/runtime , $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, src/util , $(wildcard $(dir)/*.cpp))
ifeq ($(OUTPUT), bin)
  SRC += $(foreach dir, src/shell , $(wildcard $(dir)/*.cpp))
  CXXFLAGS += -DESCARGOT_SHELL
endif

SRC += $(foreach dir, third_party/yarr, $(wildcard $(dir)/*.cpp))
SRC += $(foreach dir, third_party/GCutil, $(wildcard $(dir)/*.cpp))

SRC_CC =
SRC_CC += $(foreach dir, third_party/double_conversion , $(wildcard $(dir)/*.cc))

OBJS := $(SRC:%.cpp= $(OUTDIR)/%.o)
OBJS += $(SRC_CC:%.cc= $(OUTDIR)/%.o)
OBJS += $(SRC_C:%.c= $(OUTDIR)/%.o)

ifeq ($(OUTPUT), bin)
  OBJS_GC=third_party/GCutil/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).static/.libs/libgc.a
else
  OBJS_GC=third_party/GCutil/bdwgc/out/$(HOST)/$(ARCH)/$(MODE).shared/.libs/libgc.a
endif

#######################################################
# Targets
#######################################################

# pull in dependency info for *existing* .o files
-include $(OBJS:.o=.d)

.DEFAULT_GOAL:=x64.interpreter.debug

x86.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x86.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
x64.interpreter.debug.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
x64.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
x64.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
x64.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
#tizen_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen_mobile_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
#tizen_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen_wearable_arm.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen_wearable_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen_wearable_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen_wearable_emulator.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .

##### TIZEN_OBS #####
tizen_obs_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
tizen_obs_i386.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
tizen_obs_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
tizen_obs_i386.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)

tizen_obs_arm.interpreter.release: $(OUTDIR)/$(BIN)
tizen_obs_i386.interpreter.release: $(OUTDIR)/$(BIN)
tizen_obs_arm.interpreter.debug: $(OUTDIR)/$(BIN)
tizen_obs_i386.interpreter.debug: $(OUTDIR)/$(BIN)

##### TIZEN24 #####
tizen24_mobile_emulator.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
tizen24_mobile_emulator.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
tizen24_mobile_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
tizen24_mobile_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)

##### TIZEN3 #####
#tizen3_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen3_mobile_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
#tizen3_mobile_arm.interpreter.debug: $(OUTDIR)/$(BIN)
#	cp -f $< .
#tizen3_mobile_arm.interpreter.release: $(OUTDIR)/$(BIN)
#	cp -f $< .
tizen3_wearable_arm.interpreter.release: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.interpreter.debug: $(OUTDIR)/$(BIN)
	cp -f $< .
tizen3_wearable_arm.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen3_wearable_arm.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_arm.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.release.shared: $(OUTDIR)/$(SHARED_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.debug.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .
tizen3_wearable_emulator.interpreter.release.static: $(OUTDIR)/$(STATIC_LIB)
	cp -f $< .

DEPENDENCY_MAKEFILE = Makefile $(BUILDDIR)/Toolchain.mk $(BUILDDIR)/Flags.mk

$(OUTDIR)/$(BIN): $(OBJS) $(OBJS_GC) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(QUIET) $(CXX) -o $@ $(OBJS) $(OBJS_GC) $(LDFLAGS)

$(OUTDIR)/$(SHARED_LIB): $(OBJS) $(OBJS_GC) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(CXX) -shared -Wl,-soname,$(SHARED_LIB) -o $@ $(OBJS) $(OBJS_GC) $(LDFLAGS)

$(OUTDIR)/$(STATIC_LIB): $(OBJS) $(DEPENDENCY_MAKEFILE)
	@echo "[LINK] $@"
	$(AR) rc $@ $(ARFLAGS) $(OBJS)

$(OUTDIR)/%.o: %.cpp $(DEPENDENCY_MAKEFILE)
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	$(QUIET) $(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

$(OUTDIR)/%.o: %.cc $(DEPENDENCY_MAKEFILE)
	@echo "[CXX] $@"
	@mkdir -p $(dir $@)
	$(QUIET) @$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) -MM $(CXXFLAGS) -MT $@ $< > $(OUTDIR)/$*.d

full:
	make x64.interpreter.debug -j$(NPROCS)
	ln -sf out/x64/interpreter/debug/$(BIN) $(BIN).x64.id
	make x64.interpreter.release -j$(NPROCS)
	ln -sf out/x64/interpreter/release/$(BIN) $(BIN).x64.ir
	make x86.interpreter.debug -j$(NPROCS)
	ln -sf out/x86/interpreter/debug/$(BIN) $(BIN).x86.id
	make x86.interpreter.release -j$(NPROCS)
	ln -sf out/x86/interpreter/release/$(BIN) $(BIN).x86.ir

clean:
	rm -rf out

.PHONY: clean

#######################################################
# Test
#######################################################

include $(TOOLSDIR)/Test.mk
