#include "std.h"
#include "types.h"
#include "elist.h"

void e_list_push_allocator(ePointer dummy) { /* present for binary compat only */ }
void e_list_pop_allocator(void)           { /* present for binary compat only */ }

#if 1
#define _e_list_alloc()         e_slice_new(elist_t)
#define _e_list_alloc0()        e_slice_new(elist_t)
#define _e_list_free1(list)     e_slice_free(elist_t, list)
#else
#define _e_list_alloc()         e_malloc(sizeof(elist_t))
#define _e_list_alloc0()        e_malloc(sizeof(elist_t))
#define _e_list_free1(list)     e_free(list)
#endif


elist_t* e_list_alloc(void)
{
	return _e_list_alloc0();
}

void e_list_free(elist_t *list)
{
	elist_t *p = list;
	while (p) {
		elist_t *t = p;
		p = p->next;
		_e_list_free1(t);
	}
}

void e_list_free_1(elist_t *list)
{
	_e_list_free1(list);
}

elist_t* e_list_append(elist_t *list, ePointer data)
{
	elist_t *new_list;
	elist_t *last;

	new_list = _e_list_alloc();
	new_list->data = data;
	new_list->next = NULL;

	if (list) {
		last = e_list_last(list);
		last->next = new_list;
		new_list->prev = last;

		return list;
	}
	else {
		new_list->prev = NULL;
		return new_list;
	}
}

elist_t* e_list_prepend(elist_t *list, ePointer data)
{
	elist_t *new_list;

	new_list = _e_list_alloc();
	new_list->data = data;
	new_list->next = list;

	if (list) {
		new_list->prev = list->prev;
		if (list->prev)
			list->prev->next = new_list;
		list->prev = new_list;
	}
	else
		new_list->prev = NULL;

	return new_list;
}

elist_t* e_list_insert(elist_t *list, ePointer data, eint position)
{
	elist_t *new_list;
	elist_t *tmp_list;

	if (position < 0)
		return e_list_append(list, data);
	else if (position == 0)
		return e_list_prepend(list, data);

	tmp_list = e_list_nth(list, position);
	if (!tmp_list)
		return e_list_append(list, data);

	new_list = _e_list_alloc();
	new_list->data = data;
	new_list->prev = tmp_list->prev;
	if (tmp_list->prev)
		tmp_list->prev->next = new_list;
	new_list->next = tmp_list;
	tmp_list->prev = new_list;

	if (tmp_list == list)
		return new_list;
	else
		return list;
}

elist_t* e_list_insert_before(elist_t *list, elist_t *sibling, ePointer data)
{
	if (!list) {
		list = e_list_alloc();
		list->data = data;
		e_return_val_if_fail(sibling == NULL, list);
		return list;
	}
	else if (sibling) {
		elist_t *node;

		node = _e_list_alloc();
		node->data = data;
		node->prev = sibling->prev;
		node->next = sibling;
		sibling->prev = node;
		if (node->prev) {
			node->prev->next = node;
			return list;
		}
		else {
			e_return_val_if_fail(sibling == list, node);
			return node;
		}
	}
	else {
		elist_t *last;

		last = list;
		while (last->next)
			last = last->next;

		last->next = _e_list_alloc ();
		last->next->data = data;
		last->next->prev = last;
		last->next->next = NULL;

		return list;
	}
}

elist_t * e_list_concat(elist_t *list1, elist_t *list2)
{
	elist_t *tmp_list;

	if (list2) {
		tmp_list = e_list_last (list1);
		if (tmp_list)
			tmp_list->next = list2;
		else
			list1 = list2;
		list2->prev = tmp_list;
	}

	return list1;
}

elist_t* e_list_remove(elist_t *list, eConstPointer data)
{
	elist_t *tmp;

	tmp = list;
	while (tmp) {
		if (tmp->data != data)
			tmp = tmp->next;
		else {
			if (tmp->prev)
				tmp->prev->next = tmp->next;
			if (tmp->next)
				tmp->next->prev = tmp->prev;

			if (list == tmp)
				list = list->next;

			_e_list_free1(tmp);

			break;
		}
	}
	return list;
}

elist_t* e_list_remove_all(elist_t *list, eConstPointer data)
{
	elist_t *tmp = list;

	while (tmp) {
		if (tmp->data != data)
			tmp = tmp->next;
		else {
			elist_t *next = tmp->next;

			if (tmp->prev)
				tmp->prev->next = next;
			else
				list = next;
			if (next)
				next->prev = tmp->prev;

			_e_list_free1(tmp);
			tmp = next;
		}
	}
	return list;
}

static inline elist_t* _g_list_remove_link(elist_t *list, elist_t *link)
{
	if (link) {
		if (link->prev)
			link->prev->next = link->next;
		if (link->next)
			link->next->prev = link->prev;

		if (link == list)
			list = list->next;

		link->next = NULL;
		link->prev = NULL;
	}

	return list;
}

elist_t* e_list_remove_link(elist_t *list, elist_t *link)
{
	return _g_list_remove_link(list, link);
}

elist_t* e_list_delete_link(elist_t *list, elist_t *link)
{
	list = _g_list_remove_link(list, link);
	_e_list_free1(link);

	return list;
}

elist_t* e_list_copy(elist_t *list)
{
	elist_t *new_list = NULL;

	if (list) {
		elist_t *last;

		new_list = _e_list_alloc();
		new_list->data = list->data;
		new_list->prev = NULL;
		last = new_list;
		list = list->next;
		while (list) {
			last->next = _e_list_alloc();
			last->next->prev = last;
			last = last->next;
			last->data = list->data;
			list = list->next;
		}
		last->next = NULL;
	}

	return new_list;
}

elist_t* e_list_reverse(elist_t *list)
{
	elist_t *last;

	last = NULL;
	while (list) {
		last = list;
		list = last->next;
		last->next = last->prev;
		last->prev = list;
	}

	return last;
}

elist_t* e_list_nth(elist_t *list, euint n)
{
	while ((n-- > 0) && list)
		list = list->next;

	return list;
}

elist_t* e_list_nth_prev(elist_t *list, euint n)
{
	while ((n-- > 0) && list)
		list = list->prev;

	return list;
}

ePointer e_list_nth_data(elist_t *list, euint n)
{
	while ((n-- > 0) && list)
		list = list->next;

	return list ? list->data : NULL;
}

elist_t* e_list_find(elist_t *list, eConstPointer data)
{
	while (list) {
		if (list->data == data)
			break;
		list = list->next;
	}

	return list;
}

elist_t* e_list_find_custom(elist_t *list, eConstPointer data, eCompareFunc func)
{
	e_return_val_if_fail(func != NULL, list);

	while (list) {
		if (!func(list->data, data))
			return list;
		list = list->next;
	}

	return NULL;
}

eint e_list_position(elist_t *list, elist_t *link)
{
	eint i;

	i = 0;
	while (list) {
		if (list == link)
			return i;
		i++;
		list = list->next;
	}

	return -1;
}

eint e_list_index(elist_t *list, eConstPointer data)
{
	eint i;

	i = 0;
	while (list) {
		if (list->data == data)
			return i;
		i++;
		list = list->next;
	}

	return -1;
}

elist_t* e_list_last(elist_t *list)
{
	if (list) {
		while (list->next)
			list = list->next;
	}

	return list;
}

elist_t* e_list_first(elist_t *list)
{
	if (list) {
		while (list->prev)
			list = list->prev;
	}

	return list;
}

euint e_list_length(elist_t *list)
{
	euint length;

	length = 0;
	while (list) {
		length++;
		list = list->next;
	}

	return length;
}

void e_list_foreach(elist_t *list, eFunc func, ePointer user_data)
{
	while (list) {
		elist_t *next = list->next;
		(*func)(list->data, user_data);
		list = next;
	}
}

static elist_t*
e_list_insert_sorted_real(elist_t    *list,
		ePointer  data,
		eFunc     func,
		ePointer  user_data)
{
	elist_t *tmp_list = list;
	elist_t *new_list;
	eint cmp;

	e_return_val_if_fail(func != NULL, list);

	if (!list) {
		new_list = _e_list_alloc0();
		new_list->data = data;
		return new_list;
	}

	cmp = ((eCompareDataFunc)func)(data, tmp_list->data, user_data);

	while ((tmp_list->next) && (cmp > 0)) {
		tmp_list = tmp_list->next;

		cmp = ((eCompareDataFunc)func)(data, tmp_list->data, user_data);
	}

	new_list = _e_list_alloc0();
	new_list->data = data;

	if ((!tmp_list->next) && (cmp > 0)) {
		tmp_list->next = new_list;
		new_list->prev = tmp_list;
		return list;
	}

	if (tmp_list->prev) {
		tmp_list->prev->next = new_list;
		new_list->prev = tmp_list->prev;
	}
	new_list->next = tmp_list;
	tmp_list->prev = new_list;

	if (tmp_list == list)
		return new_list;
	else
		return list;
}

elist_t*
e_list_insert_sorted(elist_t  *list,
		ePointer      data,
		eCompareFunc  func)
{
	return e_list_insert_sorted_real(list, data, (eFunc) func, NULL);
}

elist_t*
e_list_insert_sorted_with_data(elist_t  *list,
		ePointer          data,
		eCompareDataFunc  func,
		ePointer          user_data)
{
	return e_list_insert_sorted_real(list, data, (eFunc)func, user_data);
}

static elist_t *
e_list_sort_merge(elist_t     *l1, 
		elist_t     *l2,
		eFunc     compare_func,
		ePointer  user_data)
{
	elist_t list, *l, *lprev;
	eint cmp;

	l = &list; 
	lprev = NULL;

	while (l1 && l2) {
		cmp = ((eCompareDataFunc)compare_func)(l1->data, l2->data, user_data);

		if (cmp <= 0) {
			l->next = l1;
			l1 = l1->next;
		} 
		else {
			l->next = l2;
			l2 = l2->next;
		}
		l = l->next;
		l->prev = lprev; 
		lprev = l;
	}
	l->next = l1 ? l1 : l2;
	l->next->prev = l;

	return list.next;
}

static elist_t* 
e_list_sort_real(elist_t *list,
		eFunc     compare_func,
		ePointer  user_data)
{
	elist_t *l1, *l2;

	if (!list) 
		return NULL;
	if (!list->next) 
		return list;

	l1 = list; 
	l2 = list->next;

	while ((l2 = l2->next) != NULL) {
		if ((l2 = l2->next) == NULL) 
			break;
		l1 = l1->next;
	}
	l2 = l1->next; 
	l1->next = NULL; 

	return e_list_sort_merge(e_list_sort_real(list, compare_func, user_data),
			e_list_sort_real(l2, compare_func, user_data),
			compare_func,
			user_data);
}

elist_t *e_list_sort(elist_t *list, eCompareFunc compare_func)
{
	return e_list_sort_real(list, (eFunc)compare_func, NULL);
}

elist_t *
e_list_sort_with_data(elist_t    *list,
		eCompareDataFunc  compare_func,
		ePointer          user_data)
{
	return e_list_sort_real(list, (eFunc)compare_func, user_data);
}
