#include <stdlib.h>
#include <string.h>
#include "elist.h"

void e_list_init(elist_t *lt, int size)
{
	lt->head  = 0;
	lt->tail  = 0;
	lt->sort  = 0;
	lt->count = 0;
	lt->size  = size;
}

elist_t *e_list_new(int size)
{
	elist_t *lt = malloc(sizeof(elist_t));
	e_list_init(lt, size);
	return lt;
}

void e_list_insert_head(elist_t *lt, enode_t *new)
{
	if (!lt->head) {
		new->next = 0;
		new->prev = 0;
		lt->head = new;
		lt->tail = new;
		lt->count = 1;
	}
	else {
		new->next  = lt->head;
		lt->head->prev = new;
		lt->head = new;
		new->prev  = 0;
		lt->count++;
	}
}

void e_list_insert_tail(elist_t *lt, enode_t *new)
{
	if (!lt->tail) {
		new->next = 0;
		new->prev = 0;
		lt->head = new;
		lt->tail = new;
		lt->count = 1;
	}
	else {
		new->prev  = lt->tail;
		lt->tail->next = new;
		lt->tail = new;
		new->next   = 0;
		lt->count++;
	}
}

void e_list_insert_after(elist_t *lt, enode_t *node, enode_t *new)
{
	if (!lt->head) {
		new->next = 0;
		new->prev = 0;
		lt->head = new;
		lt->tail = new;
		lt->count = 1;
	}
	else if (node) {
		new->prev = node;
		if (node->next) {
			new->next = node->next;
			node->next->prev = new;
		}
		else {
			new->next   = 0;
			lt->tail = new;
		}
		node->next = new;
		lt->count++;
	}
	else
		e_list_insert_head(lt, new);
}

void e_list_insert_before(elist_t *lt, enode_t *node, enode_t *new)
{
	if (!lt->head) {
		new->next = 0;
		new->prev = 0;
		lt->head = new;
		lt->tail = new;
		lt->count  = 1;
	}
	else if (node) {
		new->next = node;
		if (node->prev) {
			new->prev = node->prev;
			node->prev->next = new;
		}
		else {
			new->prev = 0;
			lt->head = new;
		}
		node->prev = new;
		lt->count++;
	}
	else
		e_list_insert_tail(lt, new);
}

void e_list_remove(elist_t *lt, enode_t *node)
{
	if (node == lt->tail && node == lt->head) {
		lt->head = 0;
		lt->tail = 0;
	}
	else if (node == lt->tail) {
		lt->tail = node->prev;
		node->prev->next = 0;
	}
	else if (node == lt->head) {
		lt->head = node->next;
		node->next->prev = 0;
	}
	else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	lt->count--;
}

enode_t *e_list_alloc_node(elist_t *lt, void *data)
{
	enode_t *node = malloc(sizeof(enode_t) + lt->size);
	memcpy(node + 1, data, lt->size);
	return node;
}

void e_list_free_node(elist_t *lt, enode_t *node)
{
	e_list_remove(lt, node);
	free(node);
}

void e_list_empty(elist_t *lt)
{
	enode_t *node = lt->head;
	while (node) {
		enode_t *t = node;
		node = node->next;
		free(t);
	}
	lt->count = 0;
	lt->head  = 0;
	lt->tail  = 0;
}

void e_list_add_by_copy(elist_t *lt, void *data)
{
	enode_t *node = e_list_alloc_node(lt, data);
	e_list_insert_tail(lt, node);
}

void e_list_add_by_copy_size(elist_t *lt, void *data, int size)
{
	enode_t *node = e_list_alloc_node(lt, data);
	e_list_insert_tail(lt, node);
}

void e_list_add_by_sort_copy_size(elist_t *lt, void *data, int size, int (*cb)(enode_t *, enode_t *))
{
	enode_t *node = lt->head;
	enode_t *new  = e_list_alloc_node(lt, data);

	while (node) {
		if (cb(node, new))
			break;
		node = node->next;
	}
	e_list_insert_before(lt, node, new);
}

void e_list_add_data(elist_t *lt, void *data)
{
	enode_t *node = e_list_alloc_node(lt, data);
	e_list_insert_tail(lt, node);
}

void e_list_add_data_to_head(elist_t *lt, void *data)
{
	enode_t *node = e_list_alloc_node(lt, data);
	e_list_insert_head(lt, node);
}

void e_list_out_data_from_head(elist_t *lt, void *buf)
{
	if (lt->head) {
		if (buf)
			memcpy(buf, e_list_get_data(lt->head), lt->size);
		e_list_free_node(lt, lt->head);
	}
}

void e_list_out_data_from_tail(elist_t *lt, void *buf)
{
	if (lt->tail) {
		if (buf)
			memcpy(buf, e_list_get_data(lt->tail), lt->size);
		e_list_free_node(lt, lt->tail);
	}
}

void e_list_copy_data(elist_t *lt, enode_t *node, void *buf)
{
	memcpy(buf, e_list_get_data(node), lt->size);
}

void *e_list_next_data(elist_t *lt, void *data)
{
	if (data) {
		enode_t *node = (enode_t *)data - 1;
		return e_list_get_data(node->next);
	}
	else if (lt->head)
		return e_list_get_data(lt->head);

	return NULL;
}

void *e_list_prev_data(elist_t *lt, void *data)
{
	if (data) {
		enode_t *node = (enode_t *)data - 1;
		return e_list_get_data(node->prev);
	}
	else if (lt->tail)
		return e_list_get_data(lt->tail);

	return NULL;
}

void e_list_free_data(elist_t *lt, void *data)
{
	e_list_free_node(lt, (enode_t *)data - 1);
}

void e_list_add_data_by_sort(elist_t *lt, void *data)
{
	enode_t *node = lt->head;
	enode_t *new  = e_list_alloc_node(lt, data);

	if (lt->sort) {
		while (node) {
			if (lt->sort(node, new, lt->data))
				break;
			node = node->next;
		}
		e_list_insert_before(lt, node, new);
	}
	else
		e_list_insert_tail(lt, node);
}

void e_list_insert_data_before(elist_t *lt, enode_t *node, void *data)
{
	enode_t *new = e_list_alloc_node(lt, data);
	e_list_insert_before(lt, node, new);
}

void e_list_insert_data_after(elist_t *lt, enode_t *node, void *data)
{
	enode_t *new = e_list_alloc_node(lt, data);
	e_list_insert_after(lt, node, new);
}

void *e_list_get_data(enode_t *node)
{
	if (node)
		return (void *)(node + 1);
	return NULL;
}

void *e_list_get_index_data(elist_t *lt, int index)
{
	int i;
	enode_t *node;

	if (index == 0)
		return e_list_get_data(lt->head);
	if (index == lt->count - 1)
		return e_list_get_data(lt->tail);

	if (index <= lt->count / 2) {
		node = lt->head;
		for (i = 0; node && i < index; i++)
			node = node->next;
	}       
	else {
		node = lt->tail;
		for (i = lt->count - 1; node && i > index; i--)
			node = node->prev;
	}       

	if (node)
		return (void *)(node + 1);

	return NULL;
}

void e_list_set_func(elist_t *lt, int (*sort)(enode_t *, enode_t *, void *data), void *data)
{
	lt->sort = sort;
	lt->data = data;
}

void e_list_reverse(elist_t *lt)
{
	enode_t *last = NULL;
	enode_t *node = lt->head;

    while (node) {
        last = node;
        node = last->next;
        last->next = last->prev;
        last->prev = node;
    }

	last = lt->tail;
	lt->tail = lt->head;
	lt->head = last;
}

elist_t *e_list_copy(elist_t *src, elist_t *dst)
{
	enode_t *node = src->head;

	e_list_empty(dst);
	dst->size  = src->size;
	dst->count = src->count;
	dst->data  = src->data;

    while (node) {
		e_list_add_data(dst, e_list_get_data(node));
        node = node->next;
    }
	return dst;
}

elist_t *e_list_reverse_copy(elist_t *src, elist_t *dst)
{
	enode_t *node = src->tail;

	e_list_empty(dst);
	dst->size  = src->size;
	dst->count = src->count;
	dst->data  = src->data;

    while (node) {
		e_list_add_data(dst, e_list_get_data(node));
        node = node->prev;
    }
	return dst;
}
