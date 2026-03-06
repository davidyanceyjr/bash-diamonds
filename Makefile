PROJECT     := diamonds
BUILD_DIR   := build
SRC_DIR     := src
INCLUDE_DIR := src/include
TEST_DIR    := tests
VERSION     ?= $(shell git describe --tags --always --dirty 2>/dev/null || echo dev)
DIST_DIR    := $(BUILD_DIR)/release
DIST_NAME   := $(PROJECT)-$(VERSION)
DIST_STAGE  := $(DIST_DIR)/$(DIST_NAME)
DIST_TARBALL := $(DIST_DIR)/$(DIST_NAME).tar.gz
DIST_SHA256 := $(DIST_TARBALL).sha256

CC   ?= cc
BASH ?= bash
CLANG_FORMAT ?= clang-format

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

# All C/C++ headers and sources under src/
FORMAT_SRCS := $(shell find $(SRC_DIR) -type f \( -name '*.c' -o -name '*.h' \))

.PHONY: all debug rel dist dist-clean clean test list-builtins format format-check

all: debug

debug: CFLAGS := $(CFLAGS_COMMON) -O0 -g3 -fno-omit-frame-pointer
debug: $(BUILD_DIR) $(DBG_OBJDIR) \
       $(foreach b,$(BUILTINS),$(BUILD_DIR)/$(b).debug.so)

rel: CFLAGS := $(CFLAGS_COMMON) -O2 -DNDEBUG
rel: $(BUILD_DIR) $(REL_OBJDIR) \
     $(foreach b,$(BUILTINS),$(BUILD_DIR)/$(b).so)

dist: rel $(DIST_DIR)
	rm -rf "$(DIST_STAGE)"
	mkdir -p "$(DIST_STAGE)/builtins"
	cp $(BUILD_DIR)/*.so "$(DIST_STAGE)/builtins/"
	cp scripts/install-user.sh "$(DIST_STAGE)/"
	cp -r docs "$(DIST_STAGE)/"
	cp README.md "$(DIST_STAGE)/"
	tar -C "$(DIST_DIR)" -czf "$(DIST_TARBALL)" "$(DIST_NAME)"
	sha256sum "$(DIST_TARBALL)" > "$(DIST_SHA256)"
	@echo "Created $(DIST_TARBALL)"
	@echo "Created $(DIST_SHA256)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(DBG_OBJDIR):
	mkdir -p $(DBG_OBJDIR)/core

$(REL_OBJDIR):
	mkdir -p $(REL_OBJDIR)/core

$(DIST_DIR):
	mkdir -p $(DIST_DIR)

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

# ---- Formatting targets ----

format:
	@echo "Formatting source files..."
	@$(CLANG_FORMAT) -i $(FORMAT_SRCS)

format-check:
	@echo "Checking formatting..."
	@$(CLANG_FORMAT) --dry-run --Werror $(FORMAT_SRCS)

# -----------------------------

test: debug
	BASH_BUILTINS_DIR="$$(cd "$(BUILD_DIR)" && pwd)" bats $(TEST_DIR)

list-builtins:
	@echo "$(BUILTINS)"

clean:
	rm -rf $(BUILD_DIR)

dist-clean:
	rm -rf $(DIST_DIR)

-include $(DBG_OBJDIR)/core/*.d
-include $(REL_OBJDIR)/core/*.d
