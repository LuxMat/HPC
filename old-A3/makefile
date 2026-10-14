.PHONY: all clean
CFLAGS?=-Ofast -g -march=native
LDFLAGS?=-lm

all: build/main

submit.tar.gz: Makefile $(shell find src -type f)
	tar -cpzf $@ Makefile src/*.c

%.png: %.ppm
	magick convert $^ $@

build/%: src/%.c
	mkdir -p build
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

newton: build/main
	cp $^ $@

clean:
	$(RM) -rf build *.png
