BINS = what_is_openmp for_sections_single reduction reduction_advanced attributes simd schedulers tasks
CFLAGS = -O2 -fopenmp 
LDLIBS = -lm

.PHONY : all
all : $(BINS) 

what_is_openmp : what_is_openmp.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

for_sections_single : for_sections_single.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

reduction : reduction.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

reduction_advanced : reduction_advanced.c
	gcc $(CFLAGS) -o $@ $< $(LDLIBS)

attributes : attributes.c
	gcc $(CFLAGS) -o $@ $<

simd : simd.c
	gcc $(CFLAGS) -o $@ $<

schedulers : schedulers.c
	gcc $(CFLAGS) -o $@ $<

tasks : tasks.c
	gcc $(CFLAGS) -o $@ $<

.PHONY : clean
clean :
	rm -rf $(BINS)

