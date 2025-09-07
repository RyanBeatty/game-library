# Thanks to Job Vranish (https://spin.atomicobject.com/2016/08/26/makefile-c-projects/)
TARGET_EXEC := main

IS_WINDOWS := $(findstring Windows_NT,$(OS))

BUILD_DIR := build
SRC_DIRS := src
LIB_DIR := libs

ifeq ($(IS_WINDOWS), Windows_NT)
	AUDIO_FLAGS := -lole32 -lxaudio2_8
	PLATFORM_LDFLAGS := -lopengl32 -lgdi32  $(AUDIO_FLAGS)
else
	PLATFORM_LDFLAGS :=  -lGL -ldl -lrt -lX11
endif

LDFLAGS := $(LDFLAGS) -lm $(PLATFORM_LDFLAGS)

# Find all the C and C++ files we want to compile
# Note the single quotes around the * expressions. Make will incorrectly expand these otherwise.
# SRCS := $(shell find $(SRC_DIRS) -name '*.cpp' -or -name '*.c' -or -name '*.s')
SRCS := $(SRC_DIRS)/all.c

# String substitution for every C/C++ file.
# As an example, hello.cpp turns into ./build/hello.cpp.o
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# String substitution (suffix version without %).
# As an example, ./build/hello.cpp.o turns into ./build/hello.cpp.d
DEPS := $(OBJS:.o=.d)

# Every folder in ./src will need to be passed to GCC so that it can find header files
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
# Add a prefix to INC_DIRS. So moduleA would become -ImoduleA. GCC understands this -I flag
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
# TODO: Figure out how to auto detect these.
EXT_INC_FLAGS := -Iexternal/glad -Iexternal/stb

# The -MMD and -MP flags together generate Makefiles for us!
# These files will have .d instead of .o as the output.
CPPFLAGS := $(INC_FLAGS) $(EXT_INC_FLAGS) -MMD -MP -g3 -Wall -Wshadow

PLATFORM_FLAG := WIN32
BTFLAGS := -DUNITY_BUILD=1 

# The final build step.
$(BUILD_DIR)/$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Build step for C source
$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(BTFLAGS) -D L_PLATFORM=$(PLATFORM_FLAG) -c $< -o $@ $(LDFLAGS)


raylib: PLATFORM_FLAG := RAYLIB
raylib: LDFLAGS := -lraylib -L/raylib/raylib/src -I/raylib/raylib/src  -lwinmm $(LDFLAGS)
raylib: BTFLAGS := -DPLATFORM_DESKTOP=1 -DNK_INCLUDE_DEFAULT_ALLOCATOR=1
raylib: $(BUILD_DIR)/$(TARGET_EXEC)


.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(DEPS)
