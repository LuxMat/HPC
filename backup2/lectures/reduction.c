#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
  const int sz = 1000;
  double *v = (double*) malloc(sz*sizeof(double));
  double *w = (double*) malloc(sz*sizeof(double));
  double sums[4];
  double sum;

  for ( int ix = 0; ix < sz; ++ix )
    v[ix] = ix+1;

  // The number of threads used by default heavily depends on the
  // circumstances.
  omp_set_num_threads(4);


  printf("reduction\n");

  // Typical work sharing among threads is illustrated by summation of vector
  // entries: this is a reduction operation, reducing a vector to a scalar.
  sums[0] = 0.;
  sums[1] = 0.;
  sums[2] = 0.;
  sums[3] = 0.;

  #pragma omp parallel for
  for ( int ix = 0; ix < sz; ++ix )
    sums[omp_get_thread_num()] += v[ix];
  sum = sums[0] + sums[1] + sums[2] + sums[3];

  printf("sum is %lf\n", sum);


  sums[0] = 0.;
  sums[1] = 0.;
  sums[2] = 0.;
  sums[3] = 0.;

  // As opposed to the previous example of a missing for construct, this is not
  // fatally wrong in as far as it reliably gives the same (wrong) result.
  #pragma omp parallel
  for ( int ix = 0; ix < sz; ++ix )
    sums[omp_get_thread_num()] += v[ix];
  sum = sums[0] + sums[1] + sums[2] + sums[3];

  printf("sum is %lf\n", sum);


  // The is a shorthand attribute for variables by which we reduce.
  sum = 0.;
  #pragma omp parallel for reduction(+:sum)
  for ( int ix = 0; ix < sz; ++ix )
    sum += v[ix];

  printf("sum is %lf\n", sum);


  // Initialization of the reduced variables is not altered.
  sum = 100.;
  #pragma omp parallel for reduction(+:sum)
  for ( int ix = 0; ix < sz; ++ix )
    sum += v[ix];

  printf("sum is %lf\n", sum);


  // Reduction can be performed with respect to further operations, for example
  // the maximum. The name of the variable plays no role.
  // Possible operators defined by the standard: +, *, -, &, |, ^, min, max
  sum = 0.;
  #pragma omp parallel for reduction(max:sum)
  for ( int ix = 0; ix < sz; ++ix )
    sum = sum > v[ix] ? sum : v[ix];
 
  printf("max is %lf\n", sum);


  printf("\nreduction: precision\n");

  // Reduction behaves numerically differently from sequential or synchronized
  // addition.
  sum = 0.;
  for ( int ix = 0; ix < 100000000; ++ix )
    sum += sqrt(ix);

  printf("sum is %lf\n", sum);


  sum = 0.;
  #pragma omp parallel for reduction(+:sum)
  for ( int ix = 0; ix < 100000000; ++ix )
    sum += sqrt(ix);

  printf("sum is %lf\n", sum);
  

  sum = 0.;
  #pragma omp parallel for shared(sum)
  for ( int ix = 0; ix < 100000000; ++ix )
    #pragma omp atomic
    sum += sqrt(ix);

  printf("sum is %lf\n", sum);


  free(v);
  free(w);

  return 0;
}

