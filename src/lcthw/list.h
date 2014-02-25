#ifndef lcthw_List_h
#define lcthw_List_h

#include <stdlib.h>

struct ListNode;

typedef struct ListNode {
    struct ListNode *next;
    struct ListNode *prev;
    void *value;
} ListNode;

typedef struct List {
    int count;
    ListNode *first;
    ListNode *last;
} List;

List *List_create();
void List_destroy(List *list);
void List_clear(List *list);
void List_clear_destroy(List *list);

#define List_count(A) ((A)->count)
#define List_first(A) ((A)->first != NULL ? (A)->first->value : NULL)
#define List_last(A) ((A)->last != NULL ? (A)->last->value : NULL)

void List_push(List *list, void *value);
void *List_pop(List *list);

void List_shift(List *list, void *value);
void *List_unshift(List *list);

void *List_remove(List *list, ListNode *node);
void List_remove_all(List *list, void *value);

#define LIST_FOREACH(L, S, M, V) ListNode *_next = NULL;\
    ListNode *V = NULL;\
    for(V = L->S, _next = V != NULL ? V->M : NULL;   \
        V != NULL;\
        V = _next, _next = _next != NULL ? _next->M : NULL)

#endif
