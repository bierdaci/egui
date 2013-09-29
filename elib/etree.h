#ifndef __ELIB_TREE_H__
#define __ELIB_TREE_H__

#include <elib/types.h>

typedef struct _eTree  eTree;
typedef bool (*eTraverseFunc)(ePointer key, ePointer value, ePointer data);

eTree* e_tree_new            (eCompareFunc      key_compare_func);
eTree* e_tree_new_with_data  (eCompareDataFunc  key_compare_func, ePointer key_compare_data);
eTree* e_tree_new_full       (eCompareDataFunc  key_compare_func, ePointer key_compare_data,
                                 eDestroyNotify key_destroy_func, eDestroyNotify value_destroy_func);
void e_tree_destroy          (eTree *tree);
void e_tree_insert           (eTree *tree, ePointer key, ePointer value);
void e_tree_replace          (eTree *tree, ePointer key, ePointer value);
bool e_tree_remove           (eTree *tree, ePointer key);
bool e_tree_steal            (eTree *tree, ePointer key);
ePointer e_tree_lookup       (eTree *tree, eConstPointer key);
bool e_tree_lookup_extended  (eTree *tree, ePointer lookup_key, ePointer *orig_key, ePointer *value);
void     e_tree_foreach      (eTree *tree, eTraverseFunc func, ePointer user_data);
ePointer e_tree_search       (eTree *tree, eCompareFunc search_func, eConstPointer user_data);
eint     e_tree_height       (eTree *tree);
eint     e_tree_nnodes       (eTree *tree);

#endif

