#include <stdlib.h>
#include <string.h>

#include "dlist.h"

void e_dlist_init(eList_t *dlist, int size)
{
	dlist->head  = 0;
	dlist->tail  = 0;
	dlist->count = 0;
	dlist->size  = size;
}

void e_dlist_insert_head(eList_t *dlist, eListNode_t *new)
{
	if (!dlist->head) {
		new->next = 0;
		new->prev = 0;
		dlist->head = new;
		dlist->tail = new;
		dlist->count = 1;
	}
	else {
		new->next  = dlist->head;
		dlist->head->prev = new;
		dlist->head = new;
		new->prev  = 0;
		dlist->count++;
	}
}

void e_dlist_insert_tail(eList_t *dlist, eListNode_t *new)
{
	if (!dlist->tail) {
		new->next = 0;
		new->prev = 0;
		dlist->head = new;
		dlist->tail = new;
		dlist->count = 1;
	}
	else {
		new->prev  = dlist->tail;
		dlist->tail->next = new;
		dlist->tail = new;
		new->next   = 0;
		dlist->count++;
	}
}

void e_dlist_insert_after(eList_t *dlist, eListNode_t *node, eListNode_t *new)
{
	if (!dlist->head) {
		new->next = 0;
		new->prev = 0;
		dlist->head = new;
		dlist->tail = new;
		dlist->count = 1;
	}
	else if (node) {
		new->prev = node;
		if (node->next) {
			new->next = node->next;
			node->next->prev = new;
		}
		else {
			new->next   = 0;
			dlist->tail = new;
		}
		node->next = new;
		dlist->count++;
	}
	else
		e_dlist_insert_head(dlist, new);
}

void e_dlist_insert_before(eList_t *dlist, eListNode_t *node, eListNode_t *new)
{
	if (!dlist->head) {
		new->next = 0;
		new->prev = 0;
		dlist->head = new;
		dlist->tail = new;
		dlist->count  = 1;
	}
	else if (node) {
		new->next = node;
		if (node->prev) {
			new->prev = node->prev;
			node->prev->next = new;
		}
		else {
			new->prev = 0;
			dlist->head = new;
		}
		node->prev = new;
		dlist->count++;
	}
	else
		e_dlist_insert_tail(dlist, new);
}

void e_dlist_remove(eList_t *dlist, eListNode_t *node)
{
	if (node == dlist->tail && node == dlist->head) {
		dlist->head = 0;
		dlist->tail = 0;
	}
	else if (node == dlist->tail) {
		dlist->tail = node->prev;
		node->prev->next = 0;
	}
	else if (node == dlist->head) {
		dlist->head = node->next;
		node->next->prev = 0;
	}
	else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	dlist->count--;
}

eListNode_t *e_dlist_alloc_node(eList_t *dlist)
{
	eListNode_t *node = e_slice_alloc(sizeof(eListNode_t) + dlist->size);
	return node;
}

void e_dlist_free_node(eList_t *dlist, eListNode_t *node)
{
	e_dlist_remove(dlist, node);
	e_slice_free1(dlist->size, node);
}

void e_dlist_empty(eList_t *dlist)
{
	eListNode_t *node = dlist->head;
	while (node) {
		eListNode_t *t = node;
		node = node->next;
		free(t);
	}
	dlist->size  = 0;
	dlist->count = 0;
	dlist->head  = 0;
	dlist->tail  = 0;
}

void e_dlist_put_data(eList_t *dlist, void *data)
{
	eListNode_t *node = e_dlist_alloc_node(dlist);
	memcpy(node + 1, data, dlist->size);
	e_dlist_insert_tail(dlist, node);
}

void e_dlist_add_data_by_sort(eList_t *dlist, void *data)
{
	eListNode_t *node = dlist->head;
	eListNode_t *new  = e_dlist_alloc_node(dlist);
	memcpy(new + 1, data, dlist->size);

	if (dlist->sort) {
		while (node) {
			if (dlist->sort(node, new))
				break;
			node = node->next;
		}
		e_dlist_insert_before(dlist, node, new);
	}
	else
		e_dlist_insert_tail(dlist, new);
}

void *e_dlist_get_data(eListNode_t *node)
{
	if (node)
		return (void *)(node + 1);
	return NULL;
}

void *e_dlist_get_index_data(eList_t *dlist, int index)
{
	int i;
	eListNode_t *node;

	if (index == 0)
		return e_dlist_get_data(dlist->head);
	if (index == dlist->count - 1)
		return e_dlist_get_data(dlist->tail);

	if (index < dlist->count / 2) {
		node = dlist->head;
		for (i = 0; node && i < index; i++)
			node = node->next;
	}
	else {
		node = dlist->tail;
		for (i = dlist->count - 1; node && i > index; i--)
			node = node->prev;
	}

	return e_dlist_get_data(node);
}

void e_dlist_set_func(eList_t *dlist, bool (*sort)(eListNode_t *, eListNode_t *))
{
    dlist->sort = sort;
}
