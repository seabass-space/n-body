CC      = clang

CFLAGS  = -std=c99 -Wall -Wextra -Werror
CFLAGS += -I.
CFLAGS += -DSIM_IMPLEMENTATION -DPREDICTOR_IMPLEMENTATION -DDRAW_IMPLEMENTATION -DCAMERA_IMPLEMENTATION -DGUI_IMPLEMENTATION -DAPP_IMPLEMENTATION

LDFLAGS = $(shell pkg-config --libs raylib)

DEBUG = -g -fcolor-diagnostics -fansi-escape-codes -fsanitize=address
RELEASE = -O3 -march=native

all: build/main

build/main: src/*.c src/*.h
	$(CC) $< $(CFLAGS) $(LDFLAGS) $(DEBUG) -o $@

debug: build/main
	./$<

build/release: src/*.c src/*.h
	$(CC) $< $(CFLAGS) $(LDFLAGS) $(RELEASE) -o $@

release: build/release
	./$<

clean:
	rm -rf build

web:
	# https://anguscheng.com/post/2023-12-12-wasm-game-in-c-raylib/

compile_flags.txt: FORCE
	@echo "Generating compile_flags.txt."
	@echo $(CFLAGS) | tr " " "\n" > $@
FORCE:
