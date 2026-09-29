#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#define MAXINDEX 3465


int read_file(FILE *file, long start, int del, float (*cell1)[3]);

void
compute(
    float(*cell1)[3], int del1, float (*cell2)[3], 
    int del2, int start, long *final_index); 

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

  int del = ((1000000-(27720*t))/24); 
    if (del > antal) del = antal;
  printf("del: %i\n", del);
  
  float (*cell1)[3] = malloc(del *sizeof(*cell1)); 
  float (*cell2)[3] = malloc(del *sizeof(*cell2)); 
  long *final_index = calloc(MAXINDEX, sizeof(*final_index));
  if (cell1 == NULL || cell2 == NULL || final_index == NULL) {
    goto fail;
  }

  for (int inx = 0; inx*del < antal; ++inx) {

    int kvar1 = antal - inx*del;    //antal rader kvar innan slut
    if (kvar1 > del) kvar1 = del;
    //parallel här?
    if (read_file(file, (long)inx*del, kvar1, cell1) != 0) {
        fprintf(stderr, "error reading file (1st)\n"); goto fail;
    }
    compute(cell1, kvar1, cell1, kvar1, 1, final_index);

    for (int jnx = inx +  1; jnx*del < antal; ++jnx) {
    int kvar2 = antal - jnx*del;    //antal rader kvar innan slut
    if (kvar2 > del) kvar2 = del;
      
    if (read_file(file, (long)jnx*del, kvar2, cell2) != 0) {
        fprintf(stderr, "err reading file (2nd)\n"); goto fail;}
    compute(cell1, kvar1, cell2, kvar2, 0, final_index);
    }
  }
  //fclose(file);
  
  float coord_test;
  for (int test = 0; test < MAXINDEX; ++test) { //summations test
    if (final_index[test] > 0) {
      coord_test = (float)test/100;
      printf("%i,  %05.2f,  %li\n", test,  coord_test, final_index[test]);}
  } 
      

  long antall = antal;
  long total = 0;
  for (int i = 0; i < MAXINDEX; ++i) total += final_index[i];
    printf("par: %ld (förväntat %li)\n", total, antall*(antall-1)/2);
  
fail: 
  fclose(file);//kan klumpa ihop med "goto"
  free(cell1);
  free(cell2);
  free(final_index);    
  return 0;
}




int 
read_file(FILE *file, long start, int del, float (*cell1)[3])
{
  int fel = 0;
  if( fseek(file, start*24, SEEK_SET) != 0 ) //0 ok, -1 fail. 24 bytes per row. 
    return -1;
//#pragma omp parallel for
  for (int row = 0; row < del; ++row)             
    for (int xyz = 0; xyz < 3; ++xyz)                   
      if (fscanf(file, "%f", &cell1[row][xyz]) != 1)
        //fel = 1; //lösning p.g.a pragma läs
        return -1;       
  /*if(fel)
    return -1;*/
  return 0;
}


void
compute(
    float(*cell1)[3], int del1, float (*cell2)[3], 
    int del2, int start, long *final_index) 
{
#pragma omp parallel for reduction(+:final_index[:MAXINDEX])
  for (int ix = 0; ix < del1; ++ix) {
    float x1 = cell1[ix][0];
    float y1 = cell1[ix][1];
    float z1 = cell1[ix][2];
    for (int jx = (ix+1)*start; jx < del2; ++jx) {
      float dx = cell2[jx][0] - x1; 
      float dy = cell2[jx][1] - y1;
      float dz = cell2[jx][2] - z1;
      //#pragma omp atomic?
      final_index[(int)(sqrtf(dx*dx + dy*dy + dz*dz)*100 + 0.5f)] += 1; 
    }
  }
}

/* for (int index = 0; index < 10; ++index) //avläs test 
    printf("c%d = (%+.3f, %+.3f, %+.3f)\n", 
        index + 1, cell1[index][0], cell1[index][1], cell1[index][2]);*/
    
