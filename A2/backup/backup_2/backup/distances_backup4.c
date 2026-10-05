/*
gcc -O0 -march=native -fopenmp -o distances distances.c -lm 

//The program at no time may consume more than 1 MiB = 1 * 1024^2 bytes of memory. 
//Additional grace memory of 32 KiB for auxiliary data structures is granted.



 * Given the fixed point format for input and output, floating point data 
 types are not necessarily the optimal one. What other data types would be alternatives?

 * When using a data type different from float to store input and output, it is necessary to convert to float to compute square roots. How large is the incurred performance loss?

* The choice of output format impacts the way how distance counts can be stored. How can you use each rounded distance as an array index? Determine the maximal possible number of distinct distances to obtain the size of such an array. (3464)

* A large number of cells would not fit into 5 MiB of memory, 
so you have to reload at times. The functions fseek and ftell allow you to navigate inside of a file.
*/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>


int
main(
    int argc,
    char *argv[]
    )
{

  size_t t = 1;       //Parallelization arg, -t5 = 5 antal threads.
  size_t opt;
  while((opt = getopt(argc, argv, "t:")) != -1) {
    if (opt == 't')
      t = atoi(optarg);
  }
  omp_set_num_threads(t); 
  
  FILE *file; 
  file = fopen("cells.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;
  }
 
  //skala utifrån punkter
  fseek(file, 0, SEEK_END);   // hoppa till filens slut
  long SIZE = ftell(file);    // position = antal byte
  fseek(file, 0, SEEK_SET);   // tillbaka till början
  size_t antal = (int)SIZE/24;
  fclose(file);

  int ix, jx;
  size_t max_antal = 3464; //max antal kommer från sqrt((pm10²*3)) = 3464 st. 
  double *index_test = (double*)malloc((size_t)max_antal*sizeof(*index_test));
 

  size_t delar = 1;
  if (30000 < antal) {
    delar = 3;
  }
  printf("%li\n", antal);
  printf("%li\n", delar);
  //läs filen
  size_t andel = antal/delar;
  int counts;
  float sumx, sumy, sumz;      //eventuellt dela upp här beroende på antal
  float cells[andel][3];
  long andell = andel;

  //initiera indexen.
  for (ix = 0; ix < max_antal; ++ix)      //behövs denna verkligen?
    index_test[ix] = 0;

  for (long inx = 0; inx < delar; ++inx) {

    //Malloc cells?
  file = fopen("cells.txt", "r"); //kan man läsa delvis av floaten? 
    fseek(file, inx*andell, SEEK_SET);
    for (ix = 0; ix < andel; ++ix)
      for (int xyz = 0; xyz < 3; ++xyz)
        fscanf(file, "%f", &cells[ix][xyz]); //byta till ints här?
    //fflush??
    fclose(file);


    for (ix = 0; ix < andel; ++ix) {
      for (jx = ix + 1; jx < andel; ++jx) {
        sumx = pow(cells[jx][0] - cells[ix][0],2); //eventuellt byta DT redan här 
        sumy = pow(cells[jx][1] - cells[ix][1],2);
        sumz = pow(cells[jx][2] - cells[ix][2],2);
        counts = sqrt(sumx + sumy + sumz)*100;    //räcker med avrundning?--------
        index_test[counts] += 1;
      //printf("index_test: %f, counts: %li\n",index_test[counts], counts);
      //fflush(file);
      //
      }
    }
      //free cells? 
  }



  for (ix = 0; ix < max_antal; ++ix) {
    if (index_test[ix] > 0.5)     //0.5 temporär lösning, 0.0000 > 0. :c
      printf("coord: %i, count: %.1f\n", ix, index_test[ix]); 
   }
  
  free(index_test);
  return 0;
}



/*
  Outline of aspects that play into designing an efficient implementation.
  - Splitting the assignment into (four interdependent) smaller tasks

    - Memory management.
    - Parallelization.
*/



/*
When parsing the file or parts of it, it is a huge advantage to benefit from the fixed input format. The following observations and questions can guide your implementation.

* How are strings encoded in C? In doubt rewatch the video on string encoding.

* Is there a connection between the value of a digit (as a character) and the numerical value that is represents? How can that be used to parse efficiently a string of the form "+15.023"
*/



/*
There are three aspects of memory management that you have to balance. 
Recall that block iteration was a technique you saw in the videos.

* File access is relatively time consuming. In particular, 
latency is relatively high. So you want to avoid reading from file frequently.


* Balancing these first two aspects yields a read size that is bad for 
memory locality when iterating through pairs of cells. 
So you also have to employ an additional block iteration to improve 
locality in inner for-loops.
*/



/*
Parallelization can enter at several points of the program. 
Parsing and counting distances are the two most beneficial ones.

* Is there a synchronization issue with parsing?

* Can you avoid explicit synchronization when counting distances in parallel?
*/
