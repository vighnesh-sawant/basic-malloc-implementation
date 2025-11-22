#include <stddef.h>
#include <string.h>
#include <unistd.h>
struct meta {
  size_t size;
  struct meta *next;
};

struct endmeta {
  size_t isFree;
  struct meta *start;
};

#define meta_size sizeof(struct meta) + sizeof(struct endmeta)

struct meta *head = NULL;
struct meta *last = NULL;

int isFree(struct meta *curr) {
  struct endmeta *end = (struct endmeta *)((void *)(curr + 1) + curr->size);
  return end->isFree;
}

void insertIntoList(struct meta *curr, struct meta **prev) {

  if (head == NULL) {
    head = curr;
  } else if (last == NULL) {
    head->next = curr;
    last = curr;
    *prev = head;
  } else {
    last->next = curr;
    last = curr;
  }
}
void coalesce(struct meta *curr) {}

void free(void *ptr) {

  if (ptr == NULL) {
    return;
  }
  struct meta *curr = (struct meta *)ptr;
  curr = curr - 1;
  curr->next = NULL;
  struct endmeta *end = (struct endmeta *)((void *)(curr + 1) + curr->size);
  end->isFree = 1;
  struct meta **prev = (struct meta **)(curr + 1);
  *prev = last;
  insertIntoList(curr, prev);
}

void removeFromList(struct meta *curr, struct meta *prev) {

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

    struct endmeta *end = (struct endmeta *)((void *)(curr + 1) + curr->size);
    end->start = curr;
    free((void *)(curr + 1));
  }
}

void *getSpace(size_t size) {

  if (head != NULL) {
    struct meta *curr = head;

    while (1) {
      if (curr->size >= size) {
        struct meta *prev = *(struct meta **)(curr + 1);
        removeFromList(curr, prev);
        breakChunk(curr, size);
        return (void *)(curr + 1); // change this

      } else {
        if (curr->next == NULL) {
          break;
        }

        curr = curr->next;
      }
    }
  }

  struct meta *request = (struct meta *)sbrk(size + meta_size);
  if ((void *)request == (void *)-1) {
    return NULL;
  }
  request->size = size;
  struct endmeta *end =
      (struct endmeta *)((void *)(request + 1) + request->size);
  end->start = request;
  end->isFree = 1;
  return (void *)(request + 1); // change this
}

size_t alignSize(size_t size) {
  int s = ((int)size + 16 - 1) / 16;
  s = s * 16;
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
    return (void *)(curr + 1); // change this
  }

  void *request = malloc(size);

  memcpy(request, ptr, curr->size);
  free(ptr);
  return request;
}
