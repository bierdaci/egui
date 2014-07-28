#ifndef __ELIB_LIST_H__
#define __ELIB_LIST_H__

#include <elib/types.h>

typedef struct _elistnode {
    struct _elistnode *next;
    struct _elistnode *prev;
} elistnode_t;

typedef struct {
	eint size;
	eint count;
    elistnode_t *head;
    elistnode_t *tail;
	eint (*sort)(elistnode_t *, elistnode_t *, void *);
	void *data;
} elist_t;

elist_t *e_list_new(int size);
void e_list_init(elist_t *elist, int size);
void e_list_insert_head(elist_t *elist, elistnode_t *new);
void e_list_insert_tail(elist_t *elist, elistnode_t *new);
void e_list_insert_after(elist_t *elist, elistnode_t *node, elistnode_t *new);
void e_list_insert_before(elist_t *elist, elistnode_t *node, elistnode_t *new);
void e_list_remove(elist_t *elist, elistnode_t *node);
void e_list_free_node(elist_t *elist, elistnode_t *node);
void e_list_empty(elist_t *elist);
void e_list_reverse(elist_t *elist);
void e_list_insert_data_before(elist_t *elist, elistnode_t *node, void *data);
void e_list_insert_data_after(elist_t *elist, elistnode_t *node, void *data);

elist_t *e_list_copy(elist_t *src, elist_t *dst);
elist_t *e_list_reverse_copy(elist_t *src, elist_t *dst);

void e_list_push_data(elist_t *elist, void *data);
void e_list_pop_data(elist_t *elist, void *data);

void e_list_add_data(elist_t *elist, void *data);
void e_list_add_data_by_sort(elist_t *elist, void *data);
void e_list_set_func(elist_t *elist, int (*sort)(elistnode_t *, elistnode_t *, void *), void *data);

elistnode_t *e_list_alloc_node(int size);
void *e_list_get_data(elistnode_t *node);
void *e_list_get_index_data(elist_t *elist, int index);
void *e_list_next_data(elist_t *elist, void *data);
void *e_list_prev_data(elist_t *elist, void *data);
void  e_list_free_data(elist_t *elist, void *data);

#endif

