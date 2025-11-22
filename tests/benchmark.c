
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_POINTERS (1 << 10)
#define NUM_CYCLES (1 << 10)
#define MAX_BLOCK_SIZE (1 << 12)
int main() {
  void* pointers[NUM_POINTERS];
  clock_t start = clock();
  for (unsigned short i = 0; i < NUM_CYCLES; i++) {
    for (unsigned short j = 0; j < NUM_POINTERS; j++) {
      pointers[j] = calloc(1, rand() % MAX_BLOCK_SIZE);
    }
    for (unsigned short j = 0; j < NUM_POINTERS; j++) {
      pointers[j] = realloc(pointers[j], rand() % MAX_BLOCK_SIZE);
    }
    for (unsigned short j = 0; j < NUM_POINTERS; j++) {
      free(pointers[j]);
    }
  }
  printf("Completed in %f seconds\n",
         (float)(clock() - start) / CLOCKS_PER_SEC);
}
