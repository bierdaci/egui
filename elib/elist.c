#include "std.h"
#include "types.h"
#include "elist.h"

void e_list_init(elist_t *elist, int size)
{
	elist->head  = 0;
	elist->tail  = 0;
	elist->sort  = 0;
	elist->count = 0;
	elist->size  = size;
}

elist_t *e_list_new(int size)
{
	elist_t *elist = e_malloc(sizeof(elist_t));
	e_list_init(elist, size);
	return elist;
}

void e_list_insert_head(elist_t *elist, elistnode_t *new)
{
	if (!elist->head) {
		new->next = 0;
		new->prev = 0;
		elist->head = new;
		elist->tail = new;
		elist->count = 1;
	}
	else {
		new->next  = elist->head;
		elist->head->prev = new;
		elist->head = new;
		new->prev  = 0;
		elist->count++;
	}
}

void e_list_insert_tail(elist_t *elist, elistnode_t *new)
{
	if (!elist->tail) {
		new->next = 0;
		new->prev = 0;
		elist->head = new;
		elist->tail = new;
		elist->count = 1;
	}
	else {
		new->prev  = elist->tail;
		elist->tail->next = new;
		elist->tail = new;
		new->next   = 0;
		elist->count++;
	}
}

void e_list_insert_after(elist_t *elist, elistnode_t *node, elistnode_t *new)
{
	if (!elist->head) {
		new->next = 0;
		new->prev = 0;
		elist->head = new;
		elist->tail = new;
		elist->count = 1;
	}
	else if (node) {
		new->prev = node;
		if (node->next) {
			new->next = node->next;
			node->next->prev = new;
		}
		else {
			new->next   = 0;
			elist->tail = new;
		}
		node->next = new;
		elist->count++;
	}
	else
		e_list_insert_head(elist, new);
}

void e_list_insert_before(elist_t *elist, elistnode_t *node, elistnode_t *new)
{
	if (!elist->head) {
		new->next = 0;
		new->prev = 0;
		elist->head = new;
		elist->tail = new;
		elist->count  = 1;
	}
	else if (node) {
		new->next = node;
		if (node->prev) {
			new->prev = node->prev;
			node->prev->next = new;
		}
		else {
			new->prev = 0;
			elist->head = new;
		}
		node->prev = new;
		elist->count++;
	}
	else
		e_list_insert_tail(elist, new);
}

void e_list_remove(elist_t *elist, elistnode_t *node)
{
	if (node == elist->tail && node == elist->head) {
		elist->head = 0;
		elist->tail = 0;
	}
	else if (node == elist->tail) {
		elist->tail = node->prev;
		node->prev->next = 0;
	}
	else if (node == elist->head) {
		elist->head = node->next;
		node->next->prev = 0;
	}
	else {
		node->prev->next = node->next;
		node->next->prev = node->prev;
	}
	elist->count--;
}

elistnode_t *e_list_alloc_node(int size)
{
	elistnode_t *node = e_malloc(sizeof(elistnode_t) + size);
	return node;
}

void e_list_free_node(elist_t *elist, elistnode_t *node)
{
	e_list_remove(elist, node);
	e_free(node);
}

void e_list_empty(elist_t *elist)
{
	elistnode_t *node = elist->head;
	while (node) {
		elistnode_t *t = node;
		node = node->next;
		e_free(t);
	}
	elist->count = 0;
	elist->head  = 0;
	elist->tail  = 0;
}

void e_list_add_by_copy(elist_t *elist, void *data)
{
	elistnode_t *node = e_list_alloc_node(elist->size);
	memcpy(node + 1, data, elist->size);
	e_list_insert_tail(elist, node);
}

void e_list_add_by_copy_size(elist_t *elist, void *data, int size)
{
	elistnode_t *node = e_list_alloc_node(size);
	memcpy(node + 1, data, size);
	e_list_insert_tail(elist, node);
}

void e_list_add_by_sort_copy_size(elist_t *elist, void *data, int size, int (*cb)(elistnode_t *, elistnode_t *))
{
	elistnode_t *node = elist->head;
	elistnode_t *new  = e_list_alloc_node(size);
	memcpy(new + 1, data, size);

	while (node) {
		if (cb(node, new))
			break;
		node = node->next;
	}
	e_list_insert_before(elist, node, new);
}

void e_list_add_data(elist_t *elist, void *data)
{
	elistnode_t *node = e_list_alloc_node(elist->size);
	memcpy(node + 1, data, elist->size);
	e_list_insert_tail(elist, node);
}

void e_list_push_data(elist_t *elist, void *data)
{
	elistnode_t *node = e_list_alloc_node(elist->size);
	memcpy(node + 1, data, elist->size);
	e_list_insert_head(elist, node);
}

void e_list_pop_data(elist_t *elist, void *data)
{
	if (elist->head) {
		if (data)
			memcpy(data, e_list_get_data(elist->head), elist->size);
		e_list_free_node(elist, elist->head);
	}
}

void *e_list_next_data(elist_t *elist, void *data)
{
	if (data) {
		elistnode_t *node = (elistnode_t *)data - 1;
		return e_list_get_data(node->next);
	}
	else if (elist->head)
		return e_list_get_data(elist->head);

	return NULL;
}

void *e_list_prev_data(elist_t *elist, void *data)
{
	if (data) {
		elistnode_t *node = (elistnode_t *)data - 1;
		return e_list_get_data(node->prev);
	}
	else if (elist->tail)
		return e_list_get_data(elist->tail);

	return NULL;
}

void e_list_free_data(elist_t *elist, void *data)
{
	e_list_free_node(elist, (elistnode_t *)data - 1);
}

void e_list_add_data_by_sort(elist_t *elist, void *data)
{
	elistnode_t *node = elist->head;
	elistnode_t *new  = e_list_alloc_node(elist->size);

	memcpy(new + 1, data, elist->size);

	if (elist->sort) {
		while (node) {
			if (elist->sort(node, new, elist->data))
				break;
			node = node->next;
		}
		e_list_insert_before(elist, node, new);
	}
	else
		e_list_insert_tail(elist, node);
}

void e_list_insert_data_before(elist_t *elist, elistnode_t *node, void *data)
{
	elistnode_t *new = e_list_alloc_node(elist->size);
	memcpy(new + 1, data, elist->size);
	e_list_insert_before(elist, node, new);
}

void e_list_insert_data_after(elist_t *elist, elistnode_t *node, void *data)
{
	elistnode_t *new = e_list_alloc_node(elist->size);
	memcpy(new + 1, data, elist->size);
	e_list_insert_after(elist, node, new);
}

void *e_list_get_data(elistnode_t *node)
{
	if (node)
		return (void *)(node + 1);
	return NULL;
}

void *e_list_get_index_data(elist_t *elist, int index)
{
	int i;
	elistnode_t *node;

	if (index == 0)
		return e_list_get_data(elist->head);
	if (index == elist->count - 1)
		return e_list_get_data(elist->tail);

	if (index <= elist->count / 2) {
		node = elist->head;
		for (i = 0; node && i < index; i++)
			node = node->next;
	}       
	else {
		node = elist->tail;
		for (i = elist->count - 1; node && i > index; i--)
			node = node->prev;
	}       

	if (node)
		return (void *)(node + 1);

	return NULL;
}

void e_list_set_func(elist_t *elist, int (*sort)(elistnode_t *, elistnode_t *, void *data), void *data)
{
	elist->sort = sort;
	elist->data = data;
}

void e_list_reverse(elist_t *list)
{
	elistnode_t *last = NULL;
	elistnode_t *node = list->head;

    while (node) {
        last = node;
        node = last->next;
        last->next = last->prev;
        last->prev = node;
    }

	last = list->tail;
	list->tail = list->head;
	list->head = last;
}

elist_t *e_list_copy(elist_t *src, elist_t *dst)
{
	elistnode_t *node = src->head;

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
	elistnode_t *node = src->tail;

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
