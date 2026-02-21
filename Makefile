PROJECT     := diamonds
BUILD_DIR   := build
SRC_DIR     := src
INCLUDE_DIR := src/include
TEST_DIR    := tests

CC   ?= cc
BASH ?= bash

# Point at a Bash source tree for headers (recommended)
# Override: make BASH_SRC=/home/opsman/bash
BASH_SRC ?= /usr/include/bash/

WARN  := -Wall -Wextra -Werror -Wpedantic
DEFS  := -D_GNU_SOURCE
STD   := -std=c11

# Include paths
INCFLAGS := -I$(INCLUDE_DIR) -I$(SRC_DIR)/builtins -I$(SRC_DIR)/include
ifneq ($(strip $(BASH_SRC)),)
INCFLAGS += -I$(BASH_SRC) -I$(BASH_SRC)/include -I$(BASH_SRC)/builtins
endif

CPPFLAGS := $(DEFS)
CFLAGS_COMMON := $(STD) $(WARN) -fPIC -fvisibility=hidden -MMD -MP $(INCFLAGS)

LDFLAGS_SO := -shared
LDLIBS     := -lm

# Auto-discover builtins from src/builtins/builtin_*.c
BUILTIN_SRCS := $(wildcard $(SRC_DIR)/builtins/builtin_*.c)
BUILTINS     := $(patsubst $(SRC_DIR)/builtins/builtin_%.c,%,$(BUILTIN_SRCS))

CORE_SRCS := $(wildcard $(SRC_DIR)/diamondcore/*.c)

DBG_OBJDIR := $(BUILD_DIR)/obj.dbg
REL_OBJDIR := $(BUILD_DIR)/obj.rel

CORE_DBG_OBJS := $(patsubst $(SRC_DIR)/diamondcore/%.c,$(DBG_OBJDIR)/core/%.o,$(CORE_SRCS))
CORE_REL_OBJS := $(patsubst $(SRC_DIR)/diamondcore/%.c,$(REL_OBJDIR)/core/%.o,$(CORE_SRCS))

.PHONY: all debug rel clean test list-builtins

all: debug

debug: CFLAGS := $(CFLAGS_COMMON) -O0 -g3 -fno-omit-frame-pointer
debug: $(BUILD_DIR) $(DBG_OBJDIR) \
       $(foreach b,$(BUILTINS),$(BUILD_DIR)/$(b).debug.so)

rel: CFLAGS := $(CFLAGS_COMMON) -O2 -DNDEBUG
rel: $(BUILD_DIR) $(REL_OBJDIR) \
     $(foreach b,$(BUILTINS),$(BUILD_DIR)/$(b).so)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DBG_OBJDIR):
	mkdir -p $(DBG_OBJDIR)/core

$(REL_OBJDIR):
	mkdir -p $(REL_OBJDIR)/core

# Compile diamondcore (debug)
$(DBG_OBJDIR)/core/%.o: $(SRC_DIR)/diamondcore/%.c | $(DBG_OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compile diamondcore (release)
$(REL_OBJDIR)/core/%.o: $(SRC_DIR)/diamondcore/%.c | $(REL_OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Link each builtin (debug)
$(BUILD_DIR)/%.debug.so: $(SRC_DIR)/builtins/builtin_%.c $(CORE_DBG_OBJS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS_SO) -o $@ $^ $(LDLIBS)

# Link each builtin (release)
$(BUILD_DIR)/%.so: $(SRC_DIR)/builtins/builtin_%.c $(CORE_REL_OBJS) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS_SO) -o $@ $^ $(LDLIBS)

test: debug
	BASH_BUILTINS_DIR="$$(cd "$(BUILD_DIR)" && pwd)" bats $(TEST_DIR)

list-builtins:
	@echo "$(BUILTINS)"

clean:
	rm -rf $(BUILD_DIR)

-include $(DBG_OBJDIR)/core/*.d
-include $(REL_OBJDIR)/core/*.d
