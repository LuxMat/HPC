/* Referencs
 * 
 * A summary card for all OpenMP commands is available at
 * https://www.openmp.org/wp-content/uploads/OpenMPRef-5.0-111802-web.pdf
 */

#include <omp.h>
#include <stdlib.h>
#include <stdio.h>

int
main()
{
  const int sz = 1000;
  double *v = (double*) malloc(sz*sizeof(double));
  double *w;

  for ( int ix = 0; ix < sz; ++ix )
    v[ix] = ix+1;

  // The number of threads used by default heavily depends on the
  // circumstances.
  omp_set_num_threads(8);

  int a = 0;
  int ix;


  w = (double*) malloc(sz*sizeof(double));

  a += omp_get_max_threads();

  // The parallel construct can be fused with many other work sharing
  // constructs.
  #pragma omp parallel for
  for ( int ix = 0; ix < sz; ++ix )
    w[ix] = 2*v[ix];

  printf("a = %i\n", a);
  printf("w[0] = %f\n", w[0]);

  free(w);


  // Variables by default are shared accross all threads, but the presented
  // default clause overwrite this and requires explicit annotation of all
  // variables. The varaible a, v, and w are shared. The variable b is private,
  // which means that it is not initialized in the begining and not preserved
  // at the end.
  #pragma omp parallel default(none) shared(a,v,w) private(ix)
  {
    // Inside of a parallel region, each thread executes each line of, if other
    // programs do not instruct differently.
    #pragma omp atomic
    a += 1;
    
    // To force execution in only one of threads, one can use the single
    // construct.
    #pragma omp single
    w = (double*) malloc(sz*sizeof(double));

    // The for construct splits up the work associated with a for loop among
    // all threads participating in the embracing parallel region. 
    #pragma omp for
    for ( ix = 0; ix < sz; ++ix ) {
      // By default, code in the body of the for loop may not depend on any
      // code for a different ix. In OpenMP 4.5 this requirement can be relaxed
      // through dependency clauses.
      w[ix] = 2*v[ix];
    }
    // A the end of a for construct (and simlar constructs) involved threads
    // synchronize.

    #pragma omp single
    {
      printf("a = %i\n", a);
      printf("w[0] = %f\n", w[0]);
    }

    // When the nowait clause is provided, threads do not synchronize at the
    // end of the associated block.
    #pragma omp single nowait
    free(v);
    #pragma omp single nowait
    free(w);
  }


  return 0;
}

