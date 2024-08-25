#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SFMT.h"

int main(int argc, char**argv){
  time_t t;
  int x;
  double y;
  sfmt_t sfmt;
  int i;

  time(&t);
  printf( "time value = %u\n", (unsigned int)t );
  printf("SFMT64\n");
  sfmt_init_gen_rand(&sfmt, (unsigned int)t);
  for(i = 0; i < 20; i++){
    x = sfmt_genrand_uint64(&sfmt);
    if (x < 0) {
      printf("minus: ");
      x = x * (-1);
    }
    printf("%d\n",x);
  }

  printf("SFMT32\n");
  sfmt_init_gen_rand(&sfmt, (unsigned int)t);
  for(i = 0; i < 20; i++){
    x = sfmt_genrand_uint32(&sfmt);
    printf("%d\n",x);
  }

  printf("SFMTL res53\n");
  sfmt_init_gen_rand(&sfmt, (unsigned int)t);
  for(i = 0; i < 20; i++){
    y = sfmt_genrand_res53(&sfmt);
    printf("%lf\n",y);
  }

  printf("rand\n");
  srand((unsigned int)t);
  for(i = 0; i < 20; i++){
    x = rand();
    printf("%d\n",x);
  }
  
}
