BINS = newton 
CFLAGS = -O0 -fopenmp -march=native -mno-avx512f -g
LDLIBS = -lm

.PHONY : all
all : $(BINS)


newton : newton.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)


.PHONY : clean
clean :
	rm -rf $(BINS)
