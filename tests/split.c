#include <stdio.h>
#include <stdlib.h>

int main() {
  char *ptr1 = (char *)malloc(2048);

  free(ptr1);

  char *ptr2 = (char *)malloc(1024);

  free(ptr2);
  printf("Split successfully!\n");
  return 0;
}
