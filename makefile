BINS = distances
CFLAGS = -O2 -fopenmp -march=native -mno-avx512f -g
LDLIBS = -lm

.PHONY : all
all : $(BINS)


distances : distances.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

.PHONY : clean
clean :
	rm -rf $(BINS)
