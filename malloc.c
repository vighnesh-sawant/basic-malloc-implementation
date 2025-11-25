#include <stddef.h>
#include <string.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta* next;
};

#define meta_size sizeof(struct meta)

struct meta* head = NULL;

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

void insertHelper(struct meta* curr, struct meta* ptr) {
  if (curr == head) {
    ptr->next = curr;
    setPrev(ptr, NULL);
    setPrev(curr, ptr);
    head = ptr;
  } else {
    ptr->next = curr;
    struct meta* prev = *(struct meta**)(curr + 1);
    prev->next = ptr;
    setPrev(curr, ptr);
    setPrev(ptr, prev);
  }
}

void insertIntoList(struct meta* ptr) {
  if (head == NULL) {
    head = ptr;
    setPrev(ptr, NULL);
  } else {
    struct meta* curr = head;
    while (1) {
      if (ptr < curr) {
        struct meta* end = (struct meta*)((void*)(ptr + 1) + ptr->size);

        insertHelper(curr, ptr);
        if (end == curr) {
          removeFromList(curr);
          ptr->size = ptr->size + curr->size + meta_size;
          break;
        }
        if (curr != head) {
          struct meta* prev = *(struct meta**)(curr + 1);
          struct meta* end = (struct meta*)((void*)(prev + 1) + prev->size);
          if (end == ptr) {
            removeFromList(ptr);
            prev->size = ptr->size + meta_size + prev->size;
          }
        }
        break;
      } else {
        if (curr->next == NULL) {
          struct meta* end = (struct meta*)((void*)(curr + 1) + curr->size);
          if (end == ptr) {
            curr->size = curr->size + ptr->size + meta_size;
            break;
          }
          curr->next = ptr;
          setPrev(ptr, curr);
          ptr->next = NULL;
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
  insertIntoList(curr);
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

void* requestSpace(size_t size) {
  // long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  // if (size > PAGE_SIZE - meta_size) {
  //}
  struct meta* request = (struct meta*)sbrk(size + meta_size);
  if ((void*)request == (void*)-1) {
    return NULL;
  }
  request->size = size;
  return (void*)(request + 1);
}

void* getSpace(size_t size) {
  // long PAGE_SIZE = sysconf(_SC_PAGESIZE);
  // if (size > PAGE_SIZE - meta_size) {
  // requestSpace(size);
  //}
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
  return requestSpace(size);
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

  if (curr->size >= size) {
    breakChunk(curr, size);
    return (void*)(curr + 1);  // change this
  }

  void* request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
