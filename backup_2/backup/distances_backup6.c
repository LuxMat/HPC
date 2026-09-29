/*
gcc -O0 -march=native -fopenmp -o distances distances.c -lm 


//approx, i.e. rounding, truncating, or similar, to 2 decimal places.
//Rounding and truncation errors are tolerated to a certain extend. 
//In case of doubt use the test script.



//Your program should accept command line arguments:
// ./distances -t5, 
//The argument to -t give the number of OpenMP threads that you may use.



//Additional grace memory of 32 KiB for auxiliary data structures is granted.


//Assume the number of cells less than or equal to 100,000.
3 koordinater, 8 bytes per float, 100 000 celler, 2 400 000 bytes. 


The program at no time may consume more than 1 MiB = 1 * 1024^2 bytes of memory. 
= 1 048 576 bytes


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
#include <unistd.h>

int
main(
    int argc,
    char *argv[]
    )
{

  //Parallelization arguments,  -t5, 5 = #, t=threads.
  size_t t = 1;
  size_t opt; // standard 
 /* 
  while((opt = getopt(argc,  argv, "t:")) != -1) {
    if ()
      atoi 
  }*/

  omp_set_num_threads(t); //?????


  FILE *file;

  file = fopen("cells100.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;
  }

  float cells [10][3];
  for (int index = 0; index < 10; ++index)
    for (int xyz = 0; xyz < 3; ++xyz)
      fscanf(file, "%f", &cells[index][xyz]);
  fclose(file);
  
  for (int index = 0; index < 10; ++index)
    printf("c%d = (%+.3f, %+.3f, %+.3f)\n",
           index + 1, cells[index][0], cells[index][1], cells[index][2]);

 
//Computes the distances between cells,
//assume coordinates are between -10 and 10.
  double testing[10][3];
  size_t index1, index2;
  double sumx[100], sumy[100], sumz[100];
  double distxyz[100];
  double counts[100];
  size_t n = 0;

  
  //#pragma omp parallel for
  for (index1 = 0; index1 < 10; ++index1)  {
    //ind2 = ind1, för c1,2 = c2,1, osv. 
    for (index2 = index1 + 1; index2 < 10; ++index2) {  
      for (int xyz = 0; xyz < 3; ++xyz)
        testing[index1][xyz] = cells[index2][xyz] - cells[index1][xyz]; 
      sumx[index1] = pow(testing[index1][0],2);
      sumy[index1] = pow(testing[index1][1],2);
      sumz[index1] = pow(testing[index1][2],2);
      distxyz[index1] = sqrt(sumx[index1] + sumy[index1] + sumz[index1]);
      counts[n] = distxyz[index1];
      n++;
    } 
  }
  //counting how often each distance occurs, 
  size_t ix, jx;
  double tempt[50];
  double sortedl[50];
  sortedl[0] = 00;

  //hittar lägsta siffran
  tempt[0] = counts[0];
  for ( ix = 0; ix < n; ++ix) {
    if (counts[ix] < tempt[0])
      tempt[0] = counts[ix];
    else {
      continue;
    }
    sortedl[0] = tempt[0];
  }
  
  //sorting
  for ( ix = 1; ix < n; ++ix ) {
    tempt[0] = 300;        //coords between pm 10, 3D gives max 300.  
    for ( jx = 0; jx < n; ++jx) {
      if (tempt[0] > counts[jx] && counts[jx] > sortedl[ix-1])
        tempt[0] = counts[jx];
      else {
        continue;
      }
    sortedl[ix] = tempt[0];
    }
    }


  //counting
  int countsi[50];
  for ( ix = 0; ix < n; ++ix ) {
    countsi[ix] = 0;
    for ( jx = 0; jx < n; ++jx) {
    if (counts[ix] == counts[jx])
      countsi[ix] += 1;
    else if (counts[ix] != counts[jx])
      countsi[ix] += 0;
    }

  printf("%05.2f   %i\n", sortedl[ix], countsi[ix]);


  }
  return 0;
}




/*
  Outline of aspects that play into designing an efficient implementation.
  - Splitting the assignment into (four interdependent) smaller tasks

    - Memory management.
    - Parallelization.
*/



/*
Questions that can help you designing a good solution to this subtask:

* Given the fixed point format for input and output, floating point data types are not necessarily the optimal one. What other data types would be alternatives?

* When using a data type different from float to store input and output, it is necessary to convert to float to compute square roots. How large is the incurred performance loss?

* The choice of output format impacts the way how distance counts can be stored. How can you use each rounded distance as an array index? Determine the maximal possible number of distinct distances to obtain the size of such an array.
*/



/*
When parsing the file or parts of it, it is a huge advantage to benefit from the fixed input format. The following observations and questions can guide your implementation.

* How are strings encoded in C? In doubt rewatch the video on string encoding.

* Is there a connection between the value of a digit (as a character) and the numerical value that is represents? How can that be used to parse efficiently a string of the form "+15.023"
*/



/*
There are three aspects of memory management that you have to balance. Recall that block iteration was a technique you saw in the videos.

* File access is relatively time consuming. In particular, latency is relatively high. So you want to avoid reading from file frequently.

* A large number of cells would not fit into 5 MiB of memory, so you have to reload at times. The functions fseek and ftell allow you to navigate inside of a file.

* Balancing these first two aspects yields a read size that is bad for memory locality when iterating through pairs of cells. So you also have to employ an additional block iteration to improve locality in inner for-loops.
*/



/*
Parallelization can enter at several points of the program. Parsing and counting distances are the two most beneficial ones.

* Is there a synchronization issue with parsing?

* Can you avoid explicit synchronization when counting distances in parallel?
*/
