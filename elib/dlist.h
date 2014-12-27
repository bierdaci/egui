#ifndef __ELIB_DLIST_H
#define __ELIB_DLIST_H

#include <elib/memory.h>

typedef struct _eListNode {
    struct _eListNode *next;
    struct _eListNode *prev;
} eListNode_t;

typedef struct {
	int size;
	int count;
    eListNode_t *head;
    eListNode_t *tail;
	bool (*sort)(eListNode_t *, eListNode_t *);
} eList_t;

void e_dlist_init(eList_t *dlist, int size);
void e_dlist_insert_head(eList_t *dlist, eListNode_t *new);
void e_dlist_insert_tail(eList_t *dlist, eListNode_t *new);
void e_dlist_insert_after(eList_t *dlist, eListNode_t *node, eListNode_t *new);
void e_dlist_insert_before(eList_t *dlist, eListNode_t *node, eListNode_t *new);
void e_dlist_remove(eList_t *dlist, eListNode_t *node);
void e_dlist_free_node(eList_t *dlist, eListNode_t *node);
void e_dlist_empty(eList_t *dlist);

void e_dlist_add_data(eList_t *dlist, void *data);
void e_dlist_add_data_by_sort(eList_t *dlist, void *data);
void e_dlist_set_func(eList_t *dlist, bool (*sort)(eListNode_t *, eListNode_t *));

eListNode_t *e_dlist_alloc_node(eList_t *dlist);
void *e_dlist_get_data(eListNode_t *node);
void *e_dlist_get_index_data(eList_t *dlist, int index);

#endif
