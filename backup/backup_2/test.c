
#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int
main(
    int argc,
    char *argv[]
    )
{
  int antal = 999+1; //denna är fast. bestämt.
  int del   = 2000;  //säg att max per del får vara 2000.
  
  int antal_block = antal/del;
  printf("antal block: %i\n", antal_block);
}
