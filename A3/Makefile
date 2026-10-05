.PHONY: all clean
CFLAGS?=-Ofast -g
LDFLAGS?=-lm

all: build/main

build/%: src/%.c
	mkdir -p build
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $^

clean:
	$(RM) -rf build
