/*Splitting the assignment into smaller tasks

This assignment comprises six largely independent tasks:

    Parsing command line arguments.

    Synchronization of compute and write threads.

    Data transfer between compute and write threads.

    Checking the convergence and divergence conditions.

    Computation of of x_n in the iteration step.

    Writing to file.*/



#include <omp.h>      // omp_set_num_threads, pragman behöver -fopenmp vid kompilering
#include <math.h>     // sqrtf – kräver -lm vid länkning
#include <stdio.h>    // FILE, fopen, fscanf, fseek, ftell, printf
#include <unistd.h>   // getopt (POSIX, ej standard-C)
#include <stdlib.h>   // malloc, calloc, free, atoi

//c11 library?



// flag:
// l, #rows and cols, 
// t threads, 
// t5, 5t (olika ordninar)


int t = 1;
int opt;
while((opt = getopt(argc, argv, "t:")) != -1) {
  if (opt == 't')
    t = atoi(optarg);
}
omp_set_num_threads(t);



/*  
Implementationi
The files should be named newton_attractors_xd.ppm and newton_convergence_xd.ppm,
where d is replaced by the exponent of x that is used.



The last argument is d, the exponent of x in x^d - 1. The parameter provided with the flag -l gives the number of rows and columns for the output picture. The parameter provided with the flag -t gives the number of threads used to compute the pictures. For example, the above command computes images of size 1000 x 1000 for the polynomial x^7 - 1. It uses 5 threads for the Newton iteration.


Implemention details: Iteration
If x_i is closer than 10^-3 to one of the roots of f(x), then abort the iteration.

Newton iteration does not converge for all points. To accommodate this, abort iteration
   if x_i is closer than 10^-3 to the origin,
   if the absolute value of its real or imaginary part is bigger than 10^5, or
   if the iteration takes 50 steps or more. In the pictures treat these cases as if there were three additional zeros of f(x) to which these iterations converge.

You have to run the iteration for each point until either of these criteria is met.
Implemention details: Writing to file

    Besides the threads to compute the iteration you may use one main thread for convenience and one thread that writes to disc. Neither of them may compute the Newton iteration.

    The output file must be a level 3 PPM file. Observe that this is an ASCII based image format and it is an RGB (as opposed to grayscale) format.

PPM files

PPM (Portable PixMap) files are an ASCII based image format. You can write and read them as a text file. Wikipedia contains a description with examples. In this assignment the header must look like

P3
L L
M

where L is the number of rows and columns and M is the maximal color value used in the body of the file.



Implementation details: Simplifications

You may assume that the degree is small, i.e. less than 10, and hardcode optimal formulas case by case.

You may assume that the number of lines is not too large, i.e. less than 10000.



Implementation strategy

The following is a possible implementation strategy. While it is not mandatory to follow it, I highly recommend any student without extensive programming experience to adhere to it.





Parsing

You can adjust the code from Assignment 0, and use global variables to store the number of threads and lines to make them available to all threads.
Synchronization

A framework for synchronizing compute and write threads is presented in the video material.

Build up you implementation on the code made available with the video on multi-stage computing.

You can split the picture into rows which are then distributed among the threads for computing the iteration.

To start with, you can omit all code for computation, checking, and writing.

Make sure via valgrind (use optimization level 0 when compiling) that the code you write accesses memory correctly for various numbers of threads (e.g. 1, 2, 3, 4) and lines (e.g. 10, 37, 1000).

Add as a writing step printf("%d ", ix), where ix is the item index (i.e. row index).

    Run the program for various numbers of threads (e.g. 1, 2, 3, 4) and lines (e.g. 10, 37, 100) and make sure that the program prints the numbers in correct order and that valgrind does not detect any invalid memory access.

When finishing these steps you have functional synchronization facilities as required for the assignment. It is helpful to

    make a copy of your code so far for later comparison in case you encounter errors, or

    add a tag in case you are using git for version control.

Data transfer

Recall that the result of a single computation in the video was represented by an array

int * result;

Since items correspond to rows in this implementation, each result has to include the attractor index and the convergence index for each pixel in a row.

    Given the assumption that the degree and hence number of attractors is small, what is the smallest data type that can encode the attractor index associated with a pixel? Call this type TYPE_ATTR, that is, whenever code in this description refers to TYPE_ATTR replace it by, say int, float etc., depending on which type you decided is the most suitable to use.

    Given the assumption that the number of iterations may be capped at a small value, what is the smallest data type that can encode the (capped) number of iterations till convergence associated with a pixel? Call this type TYPE_CONV.

Implement, allocate, and free two global arrays

TYPE_ATTR ** attractors;
TYPE_CONV ** convergences;

and corresponding local arrays in the computation and writing threads:

TYPE_ATTR * attractors_line;
TYPE_CONV * convergences_line;

Writing

It is advantageous to implement the writing step prior to the computation step, since you will have accessible feedback once the writing is in place. To guarantee meaningful input values in the writing thread add the following initialization to your computing threads:

for ( size_t cx = 0; cx < picture_size; ++cx ) {
  attractors_line[cx] = 0;
  convergences_line[cx] = 0;
}

Make sure that the attractor and convergence value 0 indeed is assigned meaning in your encoding. Most likely this is the case, but it is important to check these assumptions to avoid unnecessary searching for errors in your code.

    In the code for the writing thread, prior to iterating through the items, make sure that you open your output files and write the required file header.

One might be tempted to implement the writing as

fprintf(attr_file, "%d %d %d ", COLOR_VALUE0, COLOR_VALUE1, COLOR_VALUE2);
fprintf(conv_file, "%d %d %d ", GRAY_VALUE, GRAY_VALUE, GRAY_VALUE);

but recall that fprintf is slow; fwrite is a better solution.

    Recall string encoding in C.

    Prior to iterating through the items, for instance before creating any threads, prepare strings for each triple of color and gray values that you will need. You can either hardcode them or employ sprintf.

    When writing an item (i.e. a row) write the prepared strings directly to file via fwrite.

Checking

Before providing code for the iteration, the checking should be in place to guarantee that each step of the computation is carried out under the valid assumption that the norm of x be not too small.

There are three kinds of checks:

    An upper bound on the absolute value of the real and imaginary part of x.

    A lower bound on the absolute value of x.

    A lower bound on the absolute value of x - x', where x' is one of the roots of the given polynomial.

When implementing the arithmetic part of this step, it is helpful to first consider two questions:

    The absolute value of a complex number is the square root of its square norm. How can one avoid taking the square root? In particular, how can you avoid the use of the function cabs?

    The square norm of a complex number is the sum of two squares. When computing it for a difference x - x', how can one avoid computing twice the difference of the respective real and imaginary parts?

    How can the suggested precomputing of the exact values of the roots of the given polynomial accelerate any of these steps?

The checking for the d+3 states (d the degree of the given polynomial) inside of a repeated computation like the iteration in this assignment requires an adapted programming pattern. Consider one similar to the following:

for ( conv = 0, attr = DEFAULT_VALUE; ; ++conv ) {
  if ( CHECK CONDITION ) {
    attr = VALUE;
    break;
  }
  if ( CHECK CONDITION ) {
    attr = VALUE;
    break;
  }
  for ( EXPRESSION )
    if ( CHECK CONDITION ) {
      attr = VALUE_NOT_EQUAL_TO_THE_DEFAULT_ONE;
      break;
    }
  if ( attr != DEFAULT_VALUE )
    break;

  // computation
}

Computation

It remains to implement the computation in order to complete the program. Since you can use functionality from complex.h and you can hardcode your formula, this step is largely about finding an expression for the iteration step that is as efficient as possible. Recall that using cpow is not an option.

    Inserting the Newton iteration step naively, you obtain x - (x^d - 1)/(d\*x^(d-1)). How can you simplify it?

When hardcoding the expression that you derive from this, the following syntax is convenient:

switch ( degree ) {
  case 1:
    STATEMENTS FOR DEGREE 1;
    break;
  case 2:
    STATEMENTS FOR DEGREE 2;
    break;
  case 3:
    STATEMENTS FOR DEGREE 3;
    break;
  // insert further cases

  default:
    fprintf(stderr, "unexpected degree\n");
    exit(1);
}

To understand this code, read up on

    the switch, case, and default statements in C,

    the file pointer stderr,

    the function exit.

Going into depth: Further questions to consider

This assignment presents plenty of opportunities for improvement, both from the algorithmic point of view and the technical one. Here are some inspirations for further exploration.

Dropping the assumption that the degree is small requires several tweaks of the code.

For very large numbers of lines a more sophisticated and flexible partitioning of the work is necessary.

Almost every computation has at least one or two iterations. Can one cut them but precomputing more?

It is possible to achieve complexity log(d) in the degree d. The biggest problem is that naive checking distances to roots is slow if the degree is large. One approach to solve this is to combine a Taylor expansion of x^d - 1 and a binary search*/
