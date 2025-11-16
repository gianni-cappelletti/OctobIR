# If RACK_DIR is not defined when calling the Makefile, default to two directories above
RACK_DIR ?= ../..

# FLAGS will be passed to both the C and C++ compiler
FLAGS += -I../../libs/octob-ir-core/include
FLAGS += -I../../third_party/iPlug2/WDL
FLAGS += -I../../third_party
CFLAGS +=
CXXFLAGS +=

# Careful about linking to shared libraries, since you can't assume much about the user's environment and library search path.
# Static libraries are fine, but they should be added to this plugin's build system.
LDFLAGS +=

# Add .cpp files to the build
SOURCES += $(wildcard src/*.cpp)

# Add octob-ir-core library sources
SOURCES += ../../libs/octob-ir-core/src/AudioBuffer.cpp
SOURCES += ../../libs/octob-ir-core/src/IRLoader.cpp
SOURCES += ../../libs/octob-ir-core/src/IRProcessor.cpp

# Add iPlug2 WDL sources
SOURCES += ../../third_party/iPlug2/WDL/convoengine.cpp
SOURCES += ../../third_party/iPlug2/WDL/resample.cpp
SOURCES += ../../third_party/iPlug2/WDL/fft.c

# Add files to the ZIP package when running `make dist`
# The compiled plugin and "plugin.json" are automatically added.
DISTRIBUTABLES += res
DISTRIBUTABLES += $(wildcard LICENSE*)
DISTRIBUTABLES += $(wildcard presets)

# Include the Rack plugin Makefile framework
include $(RACK_DIR)/plugin.mk

# Formatting targets
.PHONY: format format-check
format:
	clang-format -i src/*.cpp src/*.hpp

format-check:
	clang-format --dry-run --Werror src/*.cpp src/*.hpp
