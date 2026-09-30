


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
  FILE *file = fopen("cells100.txt", "r");
  if ( file == NULL ) {
    printf("error opening file\n");
    return -1;}
  fseek(file, 0, SEEK_END);
  long SIZE = ftell(file);
  fseek(file, 0, SEEK_SET);
  fclose(file);
  printf("bytes: %li\n", SIZE);      

  


  int max_antal_distance = 3465;
  int antal = 1000;                     //test antal
  //int antal = SIZE/24;                //antal total.
  printf("antal: %i st\n", antal);
  int *final_index = calloc(max_antal_distance, sizeof (*final_index));




  //reads the file
  file = fopen("cells100.txt", "r");
  //int row, xyz;
  float (*cells)[3] = malloc(antal*sizeof(*cells)); //Change 10 to andel
  for (int row = 0; row < antal; ++row)             // also here?
    for (int xyz = 0; xyz < 3; ++xyz)                   //DONE
      if (fscanf(file, "%f", &cells[row][xyz]) != 1) {
        fprintf(stderr, "read error at row %d, coord %d\n", row, xyz);
        free(cells);
        fclose(file);
        return -1;
      }
  fclose(file);                                         //DONE
  
  for (int index1 = 0; index1 < antal; ++index1) {           //have to match with row @38
    for (int index2 = index1 + 1; index2 < antal; ++index2) {          // not needed +1 
      float diffx = (cells[index2][0] - cells[index1][0]);
      float diffy = (cells[index2][1] - cells[index1][1]);
      float diffz = (cells[index2][2] - cells[index1][2]);
      int distance = (int)(sqrtf(diffx*diffx + diffy*diffy + diffz*diffz)*100 + 0.5f);
      final_index[distance] += 1;
    } 
  }                                                    //DONE
  
  for (int index = 0; index < 10; ++index)    //lästest
    printf("c%d = (%+.3f, %+.3f, %+.3f)\n", 
        index + 1, cells[index][0], cells[index][1], cells[index][2]);
  
  for (int test = 0; test < max_antal_distance; ++test) { //summations test
    if (final_index[test] > 0)
      printf("index: %i, counts: %i\n", test, final_index[test]);} 

  long total = 0;                             //antals test
  printf("antal: %i\n", antal);
  long antall = antal;
  for (int i = 0; i < max_antal_distance; ++i) total += final_index[i];
    printf("par: %ld (förväntat %li)\n", total, antall*(antall-1)/2);
  
  

  free(final_index);
  free(cells);
  return 0;
}
