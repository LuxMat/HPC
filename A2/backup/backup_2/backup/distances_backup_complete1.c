
#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define MAXINDEX 3465


int read_file(FILE *file, long start, int del, float (*cell1)[3]);
/*
void compute_index(float (*cell1)[3], int del, long *final_index);

void compute_offdia(
    float (*cell1)[3], int del, float (*cell2)[3],  int del, long *final_index);
*/
void
compute(
    float(*cell1)[3], int del, float (*cell2)[3], 
    int del, int start, long *final_index); 

int
main(
    int argc,
    char *argv[]
    )
{
  FILE *file = fopen("./cells/cells100k.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;
  }
  fseek(file, 0, SEEK_END);
  int antal = ftell(file)/24;
  printf("antal rows: %i st, antal bytes: %i\n", antal, antal*24);

  int t = 1;
  int opt;
  while((opt = getopt(argc, argv, "t:")) != -1) {
    if (opt == 't')
      t = atoi(optarg);
  }
  omp_set_num_threads(t);


  int del = 1000;   //-t * 1 155 -------------------- GLÖM EJ
  //int del = 1000;            //någon form av min funk
  if (antal > 40000) {      //vill hitta det kortaste avståndet utifrån 
    del = 20000;         //kvar1 och 2, där vi inte får en extra run,
    //del = 20000;         //men delar lika på alla runs. 
  }
  
  float (*cell1)[3] = malloc(del * sizeof(*cell1)); 
  float (*cell2)[3] = malloc(del * sizeof(*cell2)); 
  long *final_index = calloc(MAXINDEX, sizeof(*final_index));
  if (cell1 == NULL || cell2 == NULL || final_index == NULL) {
    free(cell1);
    free(cell2);
    free(final_index);    //kan klumpa ihop med "goto"
    fclose(file);
    return -1;
  }

  for (int inx = 0; inx*del < antal; ++inx) {

    int kvar1 = antal - inx*del;    //antal rader kvar innan slut
    if (kvar1 > del) kvar1 = del;
    //parallel här?
    if (read_file(file, (long)inx*del, kvar1, cell1) != 0) {
      fprintf(stderr, "error reading file (1st)\n");
      free(cell1);
      free(cell2);
      free(final_index);
      fclose(file);
      return -1;
    }
    //compute_index(cell1, kvar1, final_index);
    compute(cell1, kvar1, cell1, kvar1, 1, final_index);

    for (int jnx = inx +  1; jnx*del < antal; ++jnx) {
    int kvar2 = antal - jnx*del;    //antal rader kvar innan slut
    if (kvar2 > del) kvar2 = del;
      
    if (read_file(file, (long)jnx*del, kvar2, cell2) != 0) {
      fprintf(stderr, "error reading file (2nd)\n");
      free(cell1);
      free(cell2);
      free(final_index);
      fclose(file);
      return -1;
    }
    //compute_offdia(cell1, kvar1, cell2, kvar2, final_index);
    compute(cell1, kvar1, cell2, kvar2, 0, final_index);
    }
  }
  fclose(file);


  
  for (int index = 0; index < 10; ++index)    //lästest
    printf("c%d = (%+.3f, %+.3f, %+.3f)\n", 
        index + 1, cell1[index][0], cell1[index][1], cell1[index][2]);
  
  for (int test = 0; test < MAXINDEX; ++test) { //summations test
    if (final_index[test] > 0)
      printf("index: %i, counts: %li\n", test, final_index[test]);} 

  long total = 0;                             //antals test
  printf("antal: %i\n", antal);
  long antall = antal;
  for (int i = 0; i < MAXINDEX; ++i) total += final_index[i];
    printf("par: %ld (förväntat %li)\n", total, antall*(antall-1)/2);
 
  

  free(final_index);
  free(cell1);
  free(cell2);
  return 0;
}





int 
read_file(FILE *file, long start, int andel, float (*cell1)[3])
{
  if( fseek(file, start*24, SEEK_SET) != 0 ) //0 ok, -1 fail. 24 bytes per row. 
    return -1;
#pragma omp parallel for
  for (int row = 0; row < andel; ++row)             //change to andel here?
    for (int xyz = 0; xyz < 3; ++xyz)                   //DONE
      if (fscanf(file, "%f", &cell1[row][xyz]) != 1) 
        return -1;
  return 0;
}


void
compute(
    float(*cell1)[3], int del, float (*cell2)[3], 
    int del, int start, long *final_index) 
{
#pragma omp parallel for reduction(+:final_index[:MAXINDEX])
  for (int ix = 0; ix < del; ++ix) {
    float x1 = cell1[ix][0];
    float y1 = cell1[ix][1];
    float z1 = cell1[ix][2];
    for (int jx = (ix+1)*start; jx < del; ++jx) {
      float dx = cell2[jx][0] - x1; 
      float dy = cell2[jx][1] - y1;
      float dz = cell2[jx][2] - z1;
      //#pragma omp atomic?
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
    }
  }
}


/*          //kanske går snabbare än när jag slagit ihop?
void
compute_index(float (*cell1)[3], int del, long *final_index)
{
   for (int ix = 0; ix < del; ++ix)     //have to match with row @38
      for (int jx = ix + 1; jx < del; ++jx) {          // not needed +1 
      float dx = (cell1[jx][0] - cell1[ix][0]);
      float dy = (cell1[jx][1] - cell1[ix][1]);
      float dz = (cell1[jx][2] - cell1[ix][2]);
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
    } 
}                                                    //DONE


void
compute_offdia(         //kan flytta in denna i compute index senare.
    float (*cell1)[3], int del, float (*cell2)[3],  int del, long *final_index)
{
  for (int ix = 0; ix < del; ++ix) {
    float x1 = cell1[ix][0];
    float y1 = cell1[ix][1];
    float z1 = cell1[ix][2];
    for (int jx = 0; jx < del; ++jx) {
      float dx = cell2[jx][0] - x1; 
      float dy = cell2[jx][1] - y1;
      float dz = cell2[jx][2] - z1;
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
    }
  }
}*/
