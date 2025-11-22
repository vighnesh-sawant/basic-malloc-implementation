#include <stddef.h>
#include <string.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta* next;
};

#define meta_size sizeof(struct meta)

struct meta* head = NULL;

void insertIntoList(struct meta* ptr, struct meta** ptrPrev) {
  if (head == NULL) {
    head = ptr;
    *ptrPrev = NULL;
  } else {
    struct meta* curr = head;
    while (1) {
      if (ptr > curr) {
        struct meta* prev = *(struct meta**)(curr + 1);
        if (curr == head) {
          ptr->next = curr;
          *ptrPrev = NULL;
          *(struct meta**)(curr + 1) = ptr;
          head = ptr;
        } else {
          ptr->next = curr;

          *ptrPrev = prev;
          *(struct meta**)(curr + 1) = ptr;
          prev->next = ptr;
        }
        break;
      } else {
        if (curr->next == NULL) {
          break;
        }

        curr = curr->next;
      }
    }
  }
}

void free(void* ptr) {
  if (ptr == NULL) {
    return;
  }
  struct meta* curr = (struct meta*)ptr;
  curr = curr - 1;
  curr->next = NULL;
  struct meta** prev = (struct meta**)(curr + 1);
  insertIntoList(curr, prev);
}

void removeFromList(struct meta* curr, struct meta* prev) {
  if (curr == head) {
    head = curr->next;

  } else {
    prev->next = curr->next;
  }
}

void breakChunk(struct meta* curr, size_t size) {
  int finalSize = curr->size - size -
                  meta_size;  // int to prevent underflow, i was missing that.
  if (finalSize > 0) {
    curr->size = size;
    curr = curr + 1;
    void* ptr = (void*)curr;
    ptr = ptr + size;
    curr = (struct meta*)ptr;

    curr->size = (size_t)finalSize;

    free((void*)(curr + 1));
  }
}

void* getSpace(size_t size) {
  if (head != NULL) {
    struct meta* curr = head;

    while (1) {
      if (curr->size >= size) {
        struct meta* prev = *(struct meta**)(curr + 1);
        removeFromList(curr, prev);
        breakChunk(curr, size);
        return (void*)(curr + 1);

      } else {
        if (curr->next == NULL) {
          break;
        }

        curr = curr->next;
      }
    }
  }

  struct meta* request = (struct meta*)sbrk(size + meta_size);
  if ((void*)request == (void*)-1) {
    return NULL;
  }
  request->size = size;
  return (void*)(request + 1);
}

size_t alignSize(size_t size) {
  int s = ((int)size + 16 - 1) / 16;
  s = s * 16;
  return (size_t)s;
}

void* malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }
  size = alignSize(size);
  return getSpace(size);
}

void* calloc(size_t noElem, size_t elemSize) {
  if (noElem && elemSize > (size_t)-1 / noElem) {
    return NULL;
  }
  size_t size = (int)noElem * (int)elemSize;
  size = alignSize(size);
  void* request = malloc(size);
  memset(request, 0, size);
  return request;
}

void* realloc(void* ptr, size_t size) {
  size = alignSize(size);
  if (ptr == NULL) {
    return malloc(size);
  }
  struct meta* curr = (struct meta*)ptr;
  curr = curr - 1;

  if (curr->size >= size) {
    breakChunk(curr, size);
    return (void*)(curr + 1);  // change this
  }

  void* request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
