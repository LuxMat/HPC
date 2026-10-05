.PHONY: all clean
CFLAGS?=-Ofast -g
LDFLAGS?=-lm

all: build/main

submit.tar.gz:
	tar -cpf $@ Makefile src/*.c

%.png: %.ppm
	magick convert $^ $@

build/%: src/%.c
	mkdir -p build
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	$(RM) -rf build *.png
