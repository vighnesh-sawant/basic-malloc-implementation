#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta* next;
  struct meta* preAdj;
  struct meta* nextAdj;
  size_t isFree;
  struct meta* prev;
};

#define meta_size sizeof(struct meta)

struct meta* head = NULL;
struct meta* last = NULL;

void setPrev(struct meta* curr, struct meta* prev) {
  if (curr != NULL) {
    curr->prev = prev;
  }
}

void removeFromList(struct meta* curr) {
  struct meta* prev = curr->prev;
  if (curr->isFree == 0) {
    return;
  }
  if (curr == head) {
    head = curr->next;
    setPrev(curr->next, NULL);
    if (curr->next == last) {
      last = NULL;
    }
  } else {
    if (prev == NULL) {
      return;
    }
    prev->next = curr->next;
    setPrev(curr->next, prev);
    if (curr == last) {
      last = prev;
    }
  }
}

void insertIntoList(struct meta* ptr) {
  if (ptr->size == 0) {
    return;
  }
  ptr->next = NULL;
  if (head == NULL) {
    head = ptr;
    setPrev(ptr, NULL);
  } else {
    if (last == NULL) {
      head->next = ptr;
      last = ptr;
      setPrev(ptr, head);
    } else {
      last->next = ptr;
      setPrev(ptr, last);
      last = ptr;
    }
  }
}

void free(void* ptr) {
  if (ptr == NULL) {
    return;
  }
  struct meta* curr = (struct meta*)ptr;
  curr = curr - 1;
  long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  if (curr->size > PAGE_SIZE - meta_size) {
    munmap((void*)curr, curr->size + meta_size);
    return;
  }
  curr->next = NULL;
  curr->isFree = 1;
  insertIntoList(curr);
  if (curr->nextAdj != NULL) {
    if (curr->nextAdj->isFree == 1) {
      removeFromList(curr->nextAdj);
      curr->size = curr->size + curr->nextAdj->size + meta_size;
      curr->nextAdj = curr->nextAdj->nextAdj;
      if (curr->nextAdj != NULL) {
        curr->nextAdj->preAdj = curr;
      }
    }
  }
  if (curr->preAdj != NULL) {
    if (curr->preAdj->isFree == 1) {
      removeFromList(curr);
      curr->preAdj->size = curr->size + curr->preAdj->size + meta_size;
      curr->preAdj->nextAdj = curr->nextAdj;
      if (curr->nextAdj != NULL) {
        curr->nextAdj->preAdj = curr->preAdj;
      }
    }
  }
}

void breakChunk(struct meta* curr, size_t size) {
  int finalSize = curr->size - size -
                  meta_size;  // int to prevent underflow, i was missing that.
  struct meta* prev = curr;
  if (finalSize > 0) {
    curr->size = size;
    curr = curr + 1;
    void* ptr = (void*)curr;
    ptr = ptr + size;
    curr = (struct meta*)ptr;
    prev->nextAdj = NULL;
    curr->preAdj = prev;
    curr->size = (size_t)finalSize;

    free((void*)(curr + 1));
  }
}

void* requestSpace(size_t size) {
  long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  long finalSize = (size + meta_size + PAGE_SIZE - 1) / PAGE_SIZE;
  finalSize = (finalSize)*PAGE_SIZE;
  finalSize = finalSize - meta_size;
  void* addr = mmap(NULL, finalSize, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (addr == MAP_FAILED) {
    return NULL;
  }

  struct meta* curr = (struct meta*)addr;
  curr->size = finalSize;
  curr->preAdj = NULL;
  curr->isFree = 0;
  curr->nextAdj = NULL;
  curr->next = NULL;
  return (void*)(curr + 1);
}

void* getSpace(size_t size) {
  long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  if (size > PAGE_SIZE - meta_size) {
    return requestSpace(size);
  }
  if (head != NULL) {
    struct meta* curr = head;

    while (1) {
      if (curr->size >= size) {
        removeFromList(curr);
        breakChunk(curr, size);
        return (void*)(curr + 1);

      } else {
        if (curr->next == NULL) {
          break;
        }
        if (curr == last) {
          break;
        }
        curr = curr->next;
      }
    }
  }
  void* request = requestSpace(size);
  struct meta* curr = (struct meta*)request;
  curr = curr - 1;
  breakChunk(curr, size);
  return request;
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
  if (size == 0) {
    // free(ptr);
    return NULL;
  }
  struct meta* curr = (struct meta*)ptr;
  curr = curr - 1;
  long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  if (curr->size >= size) {
    if (!(curr->size > PAGE_SIZE - meta_size)) {
      breakChunk(curr, size);
    }
    return (void*)(curr + 1);  // change this
  }

  void* request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
