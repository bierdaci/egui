#ifndef __ELIB_LIST_H__
#define __ELIB_LIST_H__

#include <elib/types.h>

typedef struct _elist_t elist_t;

struct _elist_t {
  ePointer data;
  elist_t *next;
  elist_t *prev;
};

elist_t* e_list_alloc(void);
void e_list_free(elist_t *list);
void e_list_free_1(elist_t *list);
elist_t* e_list_append(elist_t *list, ePointer data);
elist_t* e_list_prepend(elist_t *list, ePointer data);
elist_t* e_list_insert(elist_t *list, ePointer data, eint position);
elist_t* e_list_insert_before(elist_t *list, elist_t *sibling, ePointer data);
elist_t * e_list_concat(elist_t *list1, elist_t *list2);
elist_t* e_list_remove(elist_t *list, eConstPointer data);
elist_t* e_list_remove_all(elist_t *list, eConstPointer data);
elist_t* e_list_remove_link(elist_t *list, elist_t *link);
elist_t* e_list_delete_link(elist_t *list, elist_t *link);
elist_t* e_list_copy(elist_t *list);
elist_t* e_list_reverse(elist_t *list);
elist_t* e_list_nth(elist_t *list, euint n);
elist_t* e_list_nth_prev(elist_t *list, euint n);
ePointer e_list_nth_data(elist_t *list, euint n);
elist_t* e_list_find(elist_t *list, eConstPointer data);
elist_t* e_list_find_custom(elist_t *, eConstPointer, eCompareFunc);
eint e_list_position(elist_t *list, elist_t *link);
eint e_list_index(elist_t *list, eConstPointer data);
elist_t* e_list_last(elist_t *list);
elist_t* e_list_first(elist_t *list);
euint e_list_length(elist_t *list);
void e_list_foreach(elist_t *list, eFunc func, ePointer user_data);
elist_t* e_list_insert_sorted(elist_t *, ePointer, eCompareFunc);
elist_t* e_list_insert_sorted_with_data(elist_t  *, ePointer, eCompareDataFunc, ePointer);
elist_t *e_list_sort(elist_t *, eCompareFunc);
elist_t * e_list_sort_with_data(elist_t *, eCompareDataFunc, ePointer);

#endif
