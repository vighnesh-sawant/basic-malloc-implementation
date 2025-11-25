#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta* next;
  struct meta* pre;
  struct meta* nex;
};

struct bt {
  size_t isFree;
  struct meta* curr;
};
#define meta_size sizeof(struct meta) + sizeof(struct bt)

struct meta* head = NULL;
struct meta* last = NULL;

void setPrev(struct meta* curr, struct meta* prev) {
  if (curr != NULL) {
    *(struct meta**)(curr + 1) = prev;
  }
}

void removeFromList(struct meta* curr) {
  struct meta* prev = *(struct meta**)(curr + 1);
  if (curr == head) {
    head = curr->next;
    setPrev(curr->next, NULL);
  } else {
    prev->next = curr->next;
    setPrev(curr->next, prev);
  }
}

void insertIntoList(struct meta* ptr) {
  if (ptr->size == 0) {
    return;
  }
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
    munmap((void*)curr, curr->size);
    return;
  }
  curr->next = NULL;
  struct bt* end = (struct bt*)((void*)(curr + 1) + curr->size);
  end->isFree = 1;
  insertIntoList(curr);
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
    prev->nex = curr;
    curr->pre = prev;
    curr->size = (size_t)finalSize;

    free((void*)(curr + 1));
  }
}

void* requestSpace(size_t size) {
  void* addr = mmap(NULL, size + meta_size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (addr == MAP_FAILED) {
    return NULL;
  }
  struct meta* curr = (struct meta*)addr;
  curr->size = size;
  struct bt* end = (struct bt*)((void*)(curr + 1) + curr->size);
  end->isFree = 0;
  end->curr = curr;
  curr->pre = NULL;
  curr->nex = NULL;
  return (void*)(curr + 1);
}

void* getSpace(size_t size) {
  long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  if (size > PAGE_SIZE - meta_size) {
    requestSpace(size);
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
    free(ptr);
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
