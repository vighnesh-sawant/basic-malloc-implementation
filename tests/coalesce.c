#include <stdlib.h>
#include <stdio.h>

int main()
{
  char * ptr1 = ( char * ) malloc ( 1200 );
  char * ptr2 = ( char * ) malloc ( 1200 );

  free( ptr1 );
  free( ptr2 );

  char * ptr3 = ( char * ) malloc ( 2048 );

  free( ptr3 );

  printf("Coalesce Test PASSED\n");
  return 0;
}
