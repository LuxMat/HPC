

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <omp.h>


//kanske måste ändra på antal till andel?
int read_file(FILE *file, long start, int del1, float (*cell1)[3]);

void compute_index(float (*cell1)[3], int del1, long *final_index);

void compute_offdia(
    float (*cell1)[3], int del1, float (*cell2)[3],  int del2, long *final_index);


int
main(
    int argc,
    char *argv[]
    )
{

  int max_antal_distance = 3465;
  long *final_index = calloc(max_antal_distance, sizeof(*final_index));
  if (final_index == NULL)
    return -1;
  

  FILE *file = fopen("cells100.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;}
  fseek(file, 0, SEEK_END);
  long SIZE = ftell(file);
  //fseek(file, 0, SEEK_SET);
  //fclose(file);
  printf("bytes: %li\n", SIZE);      
  

  int antal = SIZE/24;                //antal total.
  printf("antal: %i st\n", antal);



  int del1 = antal; 
  int del2 = antal;
  if (antal > 40000) {
    del1 = 20000;
    del2 = 20000;
  }


  //reads the file
  rewind(file);
  file = fopen("cells100.txt", "r");
   if (file == NULL) 
   {
     free(final_index);
     return -1;
   }
  

  float (*cell1)[3] = malloc(del1 * sizeof(*cell1)); //Change to andel??
  float (*cell2)[3] = malloc(del2 * sizeof(*cell2)); //Change to andel??
  if (cell1 == NULL || cell2 == NULL) 
  {
    free(final_index);    //kan klumpa ihop med "goto"
    fclose(file);
    return -1;
  }

  for (int inx = 0; inx*del1 < antal; ++inx) {


    //parallel här?
    rewind(file);
   // file = fopen("cells100.txt", "r");
   //   if (file == NULL) 
   //   {
   //     free(final_index);
   //     return -1;
   //   }

    if (read_file(file, (long)inx*del1, del1, cell1) != 0) 
    {
      fprintf(stderr, "error reading file\n");
      free(cell1);
      free(final_index);
      fclose(file);
      return -1;
    }
    fclose(file);
    compute_index(cell1, del1, final_index);

    for (int jnx = inx +1; jnx*del2 < antal; ++jnx) { 
     if (read_file(file, (long)jnx*del2, del2, cell2) != 0) 
     {
       fprintf(stderr, "error reading file\n");
       free(cell2);
       free(final_index);
       fclose(file);
       return -1;
     }
    fclose(file);
    compute_offdia(cell1, del1, cell2, del2, final_index);
    }
  }
  
  if (read_file(file, inx*del1*24, del1, cell1) != 0) 
  {
    fprintf(stderr, "error reading file\n");
    free(cell1);
    free(final_index);
    fclose(file);
    return -1;
  }
  fclose(file);

  compute_index(cell1, del1, final_index);
  


  /*
  for (int index = 0; index < 10; ++index)    //lästest
    printf("c%d = (%+.3f, %+.3f, %+.3f)\n", 
        index + 1, cells[index][0], cells[index][1], cells[index][2]);
  
  for (int test = 0; test < max_antal_distance; ++test) { //summations test
    if (final_index[test] > 0)
      printf("index: %i, counts: %li\n", test, final_index[test]);} 

  long total = 0;                             //antals test
  printf("antal: %i\n", antal);
  long antall = antal;
  for (int i = 0; i < max_antal_distance; ++i) total += final_index[i];
    printf("par: %ld (förväntat %li)\n", total, antall*(antall-1)/2);
 */ 
  

  free(final_index);
  free(cell1);
  free(cell2);
  return 0;
}





int 
read_file(FILE *file, long start, int antal, float (*cell1)[3])
{
  rewind(file);
  if( fseek(file, start*24, SEEK_SET) != 0 ) //0 ok, -1 fail. 24 bytes per row. 
    return -1;
  for (int row = 0; row < antal; ++row)             //change to andel here?
    for (int xyz = 0; xyz < 3; ++xyz)                   //DONE
      if (fscanf(file, "%f", &cell1[row][xyz]) != 1) 
        return -1;
  return 0;
}


void
compute_index(float (*cell1)[3], int del1, long *final_index)
{
   for (int ix = 0; ix < del1; ++ix)     //have to match with row @38
      for (int jx = ix + 1; jx < del1; ++jx) {          // not needed +1 
      float dx = (cell1[jx][0] - cell1[ix][0]);
      float dy = (cell1[jx][1] - cell1[ix][1]);
      float dz = (cell1[jx][2] - cell1[ix][2]);
      //int dist = (int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f);
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
      //distance kanske int?
    } 
}                                                    //DONE


void
compute_offdia(         //kan flytta in denna i compute index senare.
    float (*cell1)[3], int del1, float (*cell2)[3],  int del2, long *final_index)
{
  for (int ix = 0; ix < del1; ++ix) {
    float x1 = cell1[ix][0];
    float y1 = cell1[ix][1];
    float z1 = cell1[ix][2];
    for (int jx = 0; jx < del2; ++jx) {
      float dx = cell2[jx][0] - x1; 
      float dy = cell2[jx][1] - y1;
      float dz = cell2[jx][2] - z1;
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
    }
  }
}
