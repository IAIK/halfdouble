CPPFLAGS ?= -Os -std=c++17 -Wall -g3 -Wno-strict-aliasing -Wno-format-zero-length -static -Wno-unused-function
LDFLAGS ?=

BINARY=c3_child_spray
DEBUG?=0

ifeq (${DEBUG}, 1)
CPPFLAGS += -DDEBUG
endif

aarch64: CXX := aarch64-linux-gnu-g++
x86: CXX := g++

all: aarch64 x86

aarch64: $(BINARY)_aarch64
x86: $(BINARY)_x86

$(BINARY)%: main.cpp
	@echo [CXX] ${CXX} ${CPPFLAGS} ${CFLAGS} $^ -g -o $@ ${LDFLAGS}
	@${CXX} ${CPPFLAGS} ${CFLAGS} $^ -g -o $@ ${LDFLAGS}

clean:
	@rm -rf $(BINARY)_aarch64 $(BINARY)_x86
