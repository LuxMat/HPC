
CFLAGS = -O2 -fopenmp -march=native -mno-avx512f -g
LDLIBS = -lm



distances : distances.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

test : test.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

distances_test1 : distancesextra.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

distances_test : test_distances.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)


