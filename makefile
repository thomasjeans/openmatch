# Simple Makefile for a raylib project

CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -Wshadow -Wconversion -Wundef
LDFLAGS ?=

SRC := src/main.c
OUT := bin/Open\ Match

# Detect platform for default libs when pkg-config is not available
UNAME_S := $(shell uname -s)

HAVE_PKGCONFIG := $(shell command -v pkg-config >/dev/null 2>&1 && echo yes || echo no)

ifeq ($(UNAME_S),Darwin)
# Try Homebrew install paths when pkg-config is missing
BREW_PREFIX := $(shell brew --prefix 2>/dev/null)
ifeq ($(HAVE_PKGCONFIG),no)
  ifneq ($(BREW_PREFIX),)
    RL_CFLAGS := -I$(BREW_PREFIX)/include
    RL_LIBS := -L$(BREW_PREFIX)/lib -lraylib
  endif
endif
DEFAULT_LIBS := -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
DEFAULT_LIBS := -lraylib -lm -lpthread -ldl -lrt -lX11
endif

ifeq ($(HAVE_PKGCONFIG),yes)
RL_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RL_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)
endif

ALL_CFLAGS := $(CFLAGS) $(RL_CFLAGS)
ALL_LIBS := $(if $(RL_LIBS),$(RL_LIBS),$(DEFAULT_LIBS)) $(LDFLAGS)

.PHONY: all run clean dirs

all: $(OUT)

dirs:
	@mkdir -p $(dir $(OUT))

$(OUT): $(SRC) | dirs
	$(CC) $(ALL_CFLAGS) $^ -o $(OUT) $(ALL_LIBS)

run: $(OUT)
	./$(OUT)

clean:
	rm -rf bin *.o *.a *.dSYM


