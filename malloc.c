#include <stddef.h>
#include <string.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta *next;
};

#define meta_size sizeof(struct meta)

struct meta *head = NULL;
struct meta *last = NULL;

void *getSpace(size_t size) {
  if (head != NULL) {
    struct meta *curr = head;
    struct meta *prev = head;
    while (1) {
      // Todo break the chunk if size >
      if (curr->size >= size) {
        if (curr == head) {
          head = curr->next;

          if (curr->next == NULL) {
            last = NULL;
          }
        } else {
          prev->next = curr->next;

          if (curr->next == NULL) {
            last = prev;
          }
        }
        return (void *)(curr + 1);

      } else {
        if (curr->next == NULL) {
          break;
        }

        curr = curr->next;
        prev = curr;
      }
    }
  }

  struct meta *request = (struct meta *)sbrk(size + meta_size);
  if ((void *)request == (void *)-1) {
    return NULL;
  }
  request->size = size;
  request->next = NULL;

  return (void *)(request + 1);
}

void free(void *ptr) {

  if (ptr == NULL) {
    return;
  }
  struct meta *curr = (struct meta *)ptr;
  curr = curr - 1;
  if (head == NULL) {
    head = curr;
  } else if (last == NULL) {
    head->next = curr;
    last = curr;
  } else {
    last->next = curr;
    last = curr;
  }
}

void *malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  return getSpace(size);
}

void *calloc(size_t noElem, size_t elemSize) {
  size_t size = noElem * elemSize; // check for overflow
  void *request = malloc(size);
  memset(request, 0, size);
  return request;
}

void *realloc(void *ptr, size_t size) {

  if (ptr == NULL) {
    return malloc(size);
  }
  struct meta *curr = (struct meta *)ptr;
  curr = curr - 1;

  if (curr->size >= size) {
    // implement spliting chunks
    return (void *)curr + 1;
  }

  void *request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
