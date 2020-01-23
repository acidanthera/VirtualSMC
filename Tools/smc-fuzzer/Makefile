ifeq ($V, 1)
	VERBOSE =
else
	VERBOSE = @
endif

CXX ?= cc
LD := $(CXX)
STRIP ?= strip

ARCH ?= 64
CXXFLAGS := \
    -std=c++14 \
    -Wall \
    -pedantic \
    -fno-rtti \
    -D__USE_MINGW_ANSI_STDIO=1 \
    -Wno-deprecated \
    -fno-exceptions \
    -m$(ARCH)

LDFLAGS := -m$(ARCH)

PLATFORM ?= Darwin

SRC := smc.cpp
TARGET := build/smc$(ARCH)$(TARGET_SUFFIX)

ifeq ($(PLATFORM), Darwin)
	LDFLAGS := -framework IOKit -framework CoreFoundation -lc++
	CXXFLAGS += -stdlib=libc++
	SRC += smc_mac.cpp
else
ifneq ($(findstring Mingw, $(PLATFORM)),)
	LDFLAGS := -static-libgcc -static-libstdc++ -static -mconsole
	CXXFLAGS +=
	SRC += smc_win.cpp
endif
endif

OBJ := $(SRC:%.cpp=build/%.o)
DEP := $(OBJ:%.o=%.d)

all: $(TARGET) | build

.PHONY: clean all
.SUFFIXES:

-include $(DEP)

build:
	@mkdir -p build

build/%.o: %.cpp
	@echo cc $<
	@mkdir -p $(dir $@)
	$(VERBOSE) $(ENV) $(CXX) $(CXXFLAGS) $(INC) $(DEF) -MMD -MT $@ -MF build/$*.d -o $@ -c $<

$(TARGET).sym: $(OBJ)
	@echo ld $(notdir $@)
	$(VERBOSE) $(ENV) $(LD) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

$(TARGET): $(TARGET).sym
	@echo strip $(notdir $@)
	$(VERBOSE) $(ENV) $(STRIP) $(TARGET).sym -o $@

clean:
	@rm -rf build
