BINS =  main1 distances_test
CFLAGS = -O2 -fopenmp -march=native -mno-avx512f -g
LDLIBS = -lm

.PHONY : all
all : $(BINS)

distances : distances.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

main1 : distances.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

test : test.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

distances_test : distances_test.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

.PHONY : clean
clean :
	rm -rf $(BINS)
