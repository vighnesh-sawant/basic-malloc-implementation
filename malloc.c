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
void breakChunk(struct meta *curr, size_t size) {

  int finalSize = curr->size - size -
                  meta_size; // int to prevent underflow, i was missing that.
  if (finalSize > 0) {

    curr->size = size;
    curr = curr + 1;
    void *ptr = (void *)curr;
    ptr = ptr + size;
    curr = (struct meta *)ptr;

    curr->size = (size_t)finalSize;
    curr->next = NULL;
    free((void *)(curr + 1));
  }
}

void *getSpace(size_t size) {

  if (head != NULL) {
    struct meta *curr = head;
    struct meta *prev = head;
    while (1) {
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
        breakChunk(curr, size);
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

size_t alignSize(size_t size) {
  int s = ((int)size + meta_size - 1) / meta_size;
  s = s * meta_size; // meta_size is always right size for aligned
  return (size_t)s;
}

void *malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size = alignSize(size);
  return getSpace(size);
}

void *calloc(size_t noElem, size_t elemSize) {
  if (noElem && elemSize > (size_t)-1 / noElem) {
    return NULL;
  }
  size_t size = (int)noElem * (int)elemSize;
  size = alignSize(size);
  void *request = malloc(size);
  memset(request, 0, size);
  return request;
}

void *realloc(void *ptr, size_t size) {
  size = alignSize(size);
  if (ptr == NULL) {
    return malloc(size);
  }
  struct meta *curr = (struct meta *)ptr;
  curr = curr - 1;

  if (curr->size >= size) {
    breakChunk(curr, size);
    return (void *)(curr + 1);
  }

  void *request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
