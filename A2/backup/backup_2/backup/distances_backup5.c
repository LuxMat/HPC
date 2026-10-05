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

  int t = 1;       //Parallelization arg, -t5 = 5 antal threads
  int opt;
  while((opt = getopt(argc, argv, "t:")) != -1) {
    if (opt == 't')
      t = atoi(optarg);
  }
  omp_set_num_threads(t); 
  
  FILE *file; 
  file = fopen("cells100.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;
  }
 
  //skala utifrån punkter
  fseek(file, 0, SEEK_END);   // hoppa till filens slut
  long SIZE = ftell(file);    // position = antal byte
  printf("bytes: %li\n", SIZE);      
  fseek(file, 0, SEEK_SET);   // tillbaka till början
  size_t antal = (int)SIZE/24;
  fclose(file);

  int ix, jx;
  size_t max_antal = 3465; //max antal kommer från sqrt((pm10²*3)) = 3464 st. 
  double *counts = (double*)malloc((size_t)max_antal*sizeof(*counts));

  size_t delar = 1;
  if (30000 < antal) {
    delar = 600;
  }
  //printf("antal: %li\n", antal);
  //printf("delar: %li\n", delar);
  //läs filen
  size_t andel = antal/delar;
  //int test_index;
  float sumx, sumy, sumz;      //eventuellt dela upp här beroende på antal
  float cells[andel][3];
  long andell = andel;
  printf("andel: %li, andell: %li\n", andel, andell);
  //initiera indexen.
  for (ix = 0; ix < max_antal; ++ix)      //behövs denna verkligen?
    counts[ix] = 0;

  double total = 0;

  for (long inx = 0; inx < delar; ++inx) {
  //long fileSize = ftell(file);
  //printf("The size of the file is: %ld bytes\n", fileSize);
    rewind(file); 
    file = fopen("cells100.txt", "r"); 
    fseek(file, inx*andell*24, SEEK_SET);
    for (ix = 0; ix < andel; ++ix) { //här måste man läsa av den sista 
                                     //index antal*24 - inx*andell*24
      for (int xyz = 0; xyz < 3; ++xyz) {
        //fscanf(file, "%f", &cells[ix][xyz]); //byta till ints här?
        if (fscanf(file, "%f", &cells[ix][xyz]) != 1) {
          fprintf(stderr, "Läsfel vid rad %d\n", ix);
          exit(1);}
      }
    }
    //printf("%f %f %f\n", cells[0][0], cells[0][1], cells[0][2]);
    //printf("%f %f %f\n", cells[andel-1][0], cells[andel-1][1], cells[andel-1][2]);
    fclose(file);
    for (ix = 0; ix < andel; ++ix) {
      for (jx = ix + 1; jx < andel; ++jx) {
        sumx = cells[jx][0] - cells[ix][0]; //eventuellt byta DT redan här 
        sumy = cells[jx][1] - cells[ix][1];
        sumz = cells[jx][2] - cells[ix][2];
        int test_index = (int)sqrtf(sumx*sumx + sumy*sumy + sumz*sumz)*100;    
        //räcker med avrundning?--------
        counts[test_index] += 1;
        total += 1;
      }

    //printf("counts: %f, index: %i\n", counts[test_index], test_index);
    //printf("inx: %li, andell*inx: %li\n", inx, andell*inx*24, ix, jx);
    }
  }

  printf("par: %.0f, förväntat: %.0f\n", total, (double)antal*(antal-1)/2);
  
  for (ix = 0; ix < 10; ++ix) {
    if (counts[ix] > 0.5)     //0.5 temporär lösning, 0.0000 > 0. :c
      printf("coord: %i, count: %.1f\n", ix, counts[ix]); 
   }
  
  free(counts);
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
