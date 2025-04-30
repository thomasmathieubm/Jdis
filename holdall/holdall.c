//  holdall.c : partie implantation du module holdall.

#include "holdall.h"

typedef struct choldall choldall;

struct choldall {
  void *ref;
  choldall *next;
};

struct holdall {
  choldall *head;
#if defined HOLDALL_PUT_TAIL
  choldall **tailptr;
#endif
  size_t count;
};

holdall *holdall_empty() {
  holdall *ha = malloc(sizeof *ha);
  if (ha == nullptr) {
    return nullptr;
  }
  ha->head = nullptr;
#if defined HOLDALL_PUT_TAIL
  ha->tailptr = &ha->head;
#endif
  ha->count = 0;
  return ha;
}

void holdall_dispose(holdall **haptr) {
  if (*haptr == nullptr) {
    return;
  }
  choldall *p = (*haptr)->head;
  while (p != nullptr) {
    choldall *t = p;
    p = p->next;
    free(t);
  }
  free(*haptr);
  *haptr = nullptr;
}

int holdall_put(holdall *ha, void *ref) {
  choldall *p = malloc(sizeof *p);
  if (p == nullptr) {
    return -1;
  }
  p->ref = ref;
#if defined HOLDALL_PUT_TAIL
  p->next = nullptr;
  *ha->tailptr = p;
  ha->tailptr = &p->next;
#else
  p->next = ha->head;
  ha->head = p;
#endif
  ha->count += 1;
  return 0;
}

size_t holdall_count(holdall *ha) {
  return ha->count;
}

int holdall_apply(holdall *ha,
    int (*fun)(void *)) {
  for (const choldall *p = ha->head; p != nullptr; p = p->next) {
    int r = fun(p->ref);
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

int holdall_apply_context(holdall *ha,
    void *context, void *(*fun1)(void *context, void *ptr),
    int (*fun2)(void *ptr, void *resultfun1)) {
  for (const choldall *p = ha->head; p != nullptr; p = p->next) {
    int r = fun2(p->ref, fun1(context, p->ref));
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

int holdall_apply_context2(holdall *ha,
    void *context1, void *(*fun1)(void *context1, void *ptr),
    void *context2, int (*fun2)(void *context2, void *ptr, void *resultfun1)) {
  for (const choldall *p = ha->head; p != nullptr; p = p->next) {
    int r = fun2(context2, p->ref, fun1(context1, p->ref));
    if (r != 0) {
      return r;
    }
  }
  return 0;
}

#if defined HOLDALL_EXT && defined WANT_HOLDALL_EXT != 0

static choldall *split(choldall *ha) {
    choldall* f = ha;
    choldall* s = ha;
    while (f != NULL && f->next != NULL) {
        f = f->next->next;
        if (f != NULL) {
            s = s->next;
        }
    }
    choldall* t = s->next;
    s->next = NULL;
    return t;
}

static choldall *merge(choldall *first, choldall *second,
  int (*compar)(const void *, const void *)){
  if (first == NULL) {
    return second;
  }
  if (second == NULL) {
    return first;
  }
  if (compar(first->ref, second->ref) < 0) {
    first->next = merge(first->next, second, compar);
    return first;
  }
  else {
    second->next = merge(first, second->next, compar);
    return second;
  }
}

static choldall *merge_sort(choldall *ha,
  int (*compar)(const void *, const void *)) {
  if (ha == NULL || ha->next == NULL) {
    return ha;
  }
  choldall *second = split(ha);
  ha = merge_sort(ha, compar);
  second = merge_sort(second, compar);
  return merge(ha, second, compar);
}

void holdall_sort(holdall *ha, int (*compar)(const void *, const void *)) {
  ha->head = merge_sort(ha->head, compar);
}

#endif
