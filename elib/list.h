#ifndef _ELIB_LIST_H__
#define _ELIB_LIST_H__

#include <elib/types.h>

struct list_head {
    struct list_head * next;
    struct list_head * prev;
};

typedef struct list_head list_t;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static INLINE void __list_add(struct list_head * node,
    struct list_head * prev,
    struct list_head * next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static INLINE void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static INLINE void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}

static INLINE void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static INLINE void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
}

static INLINE int list_empty(struct list_head *head)
{
    return head->next == head;
}

static INLINE int list_no_empty(struct list_head *head)
{
    return head->next != head;
}

#define list_entry_base(ptr, type, member) \
    ((char *)(ptr) - (unsigned long)(&((type *)0)->member))

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_for_eachp(pos, head) \
    for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_after(pos, head) \
	for (pos = pos->next; pos != (head); pos = pos->next)

#define list_for_each_before(pos, head) \
	for (pos = pos->prev; pos != (head); pos = pos->prev)


#define STRUCT_LIST_INSERT_HEAD(type, _head, _tail, _new, _prev, _next) \
do { \
	if (!_head) { \
		_new->_next = NULL; \
		_new->_prev = NULL; \
		_head = _new; \
		_tail = _new; \
	} \
	else { \
		_new->_next  = _head; \
		_head->_prev = _new; \
		_head = _new; \
		_new->_prev  = NULL; \
	} \
} while (0)

#define STRUCT_LIST_INSERT_TAIL(type, _head, _tail, _new, _prev, _next) \
do { \
	if (!_tail) { \
		_new->_next = NULL; \
		_new->_prev = NULL; \
		_head = _new; \
		_tail = _new; \
	} \
	else { \
		_new->_prev  = _tail; \
		_tail->_next = _new; \
		_tail = _new; \
		_new->_next  = NULL; \
	} \
} while (0)

#define STRUCT_LIST_INSERT_AFTER(type, _head, _tail, _node, _new, _prev, _next) \
do { \
	if (!_head) { \
		_new->_next = NULL; \
		_new->_prev = NULL; \
		_head = _new; \
		_tail = _new; \
	} \
	else if (_node) { \
		_new->_prev = (type *)_node; \
		if (((type *)_node)->_next) { \
			_new->_next = ((type *)_node)->_next; \
			((type *)_node)->_next->_prev = _new; \
		} \
		else { \
			_new->_next  = NULL; \
			_tail = _new; \
		} \
		((type *)_node)->_next = _new; \
	} \
	else { \
		STRUCT_LIST_INSERT_HEAD(type, _head, _tail, _new, _prev, _next); \
	} \
} while (0)

#define STRUCT_LIST_INSERT_BEFORE(type, _head, _tail, _node, _new, _prev, _next) \
do { \
	if (!_head) { \
		_new->_next = NULL; \
		_new->_prev = NULL; \
		_head = _new; \
		_tail = _new; \
	} \
	else if (_node) { \
		_new->_next = (type *)_node; \
		if (((type *)_node)->_prev) { \
			_new->_prev = ((type *)_node)->_prev; \
			((type *)_node)->_prev->_next = _new; \
		} \
		else { \
			_new->_prev  = NULL; \
			_head = _new; \
		} \
		((type *)_node)->_prev = _new; \
	} \
	else { \
		STRUCT_LIST_INSERT_TAIL(type, _head, _tail, _new, _prev, _next); \
	} \
} while (0)

#define STRUCT_LIST_DELETE(_head, _tail, _node, _prev, _next) \
do { \
	if (_node == _tail && _node == _head) { \
		_head = NULL; \
		_tail = NULL; \
	} \
	else if (_node == _tail) { \
		_tail = _node->_prev; \
		_node->_prev->_next = NULL; \
	} \
	else if (_node == _head) { \
		_head = _node->_next; \
		_node->_next->_prev = NULL; \
	} \
	else { \
		_node->_prev->_next = _node->_next; \
		_node->_next->_prev = _node->_prev; \
	} \
} while (0)

#define olist_add_tail(_type, _head, _new, _next) \
	do { \
		_type **pp = &_head; \
		while (*pp) \
			pp = &(*pp)->_next; \
		_new->_next = NULL; \
		*pp = _new; \
	} while (0)

#define olist_add_head(_type, _head, _new, _next) \
	do { \
		_new->_next = _head; \
		_head = _new; \
	} while (0)

#endif

