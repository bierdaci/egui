#include <stdio.h>

#include "elogs.h"
#include "std.h"
#include "memory.h"
#include "etree.h"

#define MAX_GTREE_HEIGHT 40

typedef struct _eTreeNode  eTreeNode;

struct _eTree {
	eTreeNode        *root;
	eCompareDataFunc  key_compare;
	eDestroyNotify    key_destroy_func;
	eDestroyNotify    value_destroy_func;
	ePointer          key_compare_data;
	euint             nnodes;
};

struct _eTreeNode {
	ePointer   key;
	ePointer   value;
	eint8      balance;
	euint8     left_child;  
	euint8     right_child;
	eTreeNode *left;
	eTreeNode *right;
};

static eTreeNode* e_tree_node_new                   (ePointer       key,
		ePointer       value);
static void       e_tree_insert_internal            (eTree         *tree,
		ePointer       key,
		ePointer       value,
		ebool       replace);
static ebool   e_tree_remove_internal            (eTree         *tree,
		ePointer  key,
		ebool       steal);
static eTreeNode* e_tree_node_balance               (eTreeNode     *node);
static eTreeNode *e_tree_find_node                  (eTree         *tree,
		eConstPointer  key);
/*
static eint       e_tree_node_pre_order             (eTreeNode     *node,
		eTraverseFunc  traverse_func,
		ePointer       data);
static eint       e_tree_node_in_order              (eTreeNode     *node,
		eTraverseFunc  traverse_func,
		ePointer       data);
static eint       e_tree_node_post_order            (eTreeNode     *node,
		eTraverseFunc  traverse_func,
		ePointer       data);
*/
static ePointer   e_tree_node_search                (eTreeNode     *node,
		eCompareFunc   search_func,
		eConstPointer  data);
static eTreeNode* e_tree_node_rotate_left           (eTreeNode     *node);
static eTreeNode* e_tree_node_rotate_right          (eTreeNode     *node);

static eTreeNode* e_tree_node_new(ePointer key, ePointer value)
{
	eTreeNode *node = e_slice_new(eTreeNode);

	node->balance = 0;
	node->left = NULL;
	node->right = NULL;
	node->left_child = efalse;
	node->right_child = efalse;
	node->key = key;
	node->value = value;

	return node;
}

eTree* e_tree_new(eCompareFunc key_compare_func)
{
	e_return_val_if_fail(key_compare_func != NULL, NULL);

	return e_tree_new_full((eCompareDataFunc)key_compare_func, NULL, NULL, NULL);
}

eTree* e_tree_new_with_data(eCompareDataFunc key_compare_func, ePointer key_compare_data)
{
	e_return_val_if_fail(key_compare_func != NULL, NULL);

	return e_tree_new_full(key_compare_func, key_compare_data, NULL, NULL);
}

eTree * e_tree_new_full(
		eCompareDataFunc key_compare_func,
		ePointer         key_compare_data,
		eDestroyNotify   key_destroy_func,
		eDestroyNotify   value_destroy_func)
{
	eTree *tree;

	e_return_val_if_fail(key_compare_func != NULL, NULL);

	tree = e_malloc(sizeof(eTree));
	tree->root               = NULL;
	tree->key_compare        = key_compare_func;
	tree->key_destroy_func   = key_destroy_func;
	tree->value_destroy_func = value_destroy_func;
	tree->key_compare_data   = key_compare_data;
	tree->nnodes             = 0;

	return tree;
}

static INLINE eTreeNode * e_tree_first_node(eTree *tree)
{
	eTreeNode *tmp;

	if (!tree->root)
		return NULL;

	tmp = tree->root;

	while (tmp->left_child)
		tmp = tmp->left;

	return tmp;
}

static INLINE eTreeNode * e_tree_node_previous(eTreeNode *node)
{
	eTreeNode *tmp;

	tmp = node->left;

	if (node->left_child)
		while (tmp->right_child)
			tmp = tmp->right;

	return tmp;
}

static INLINE eTreeNode *e_tree_node_next(eTreeNode *node)
{
	eTreeNode *tmp;

	tmp = node->right;

	if (node->right_child)
		while (tmp->left_child)
			tmp = tmp->left;

	return tmp;
}

void e_tree_destroy(eTree *tree)
{
	eTreeNode *node;
	eTreeNode *next;

	e_return_if_fail(tree != NULL);

	node = e_tree_first_node(tree);

	while (node) {
		next = e_tree_node_next(node);

		if (tree->key_destroy_func)
			tree->key_destroy_func(node->key);
		if (tree->value_destroy_func)
			tree->value_destroy_func(node->value);
		e_slice_free(eTreeNode, node);

		node = next;
	}

	e_free(tree);
}

void e_tree_insert(eTree *tree, ePointer key, ePointer value)
{
	e_return_if_fail(tree != NULL);

	e_tree_insert_internal(tree, key, value, efalse);
}

void e_tree_replace(eTree *tree, ePointer key, ePointer value)
{
	e_return_if_fail(tree != NULL);

	e_tree_insert_internal(tree, key, value, etrue);
}

static void e_tree_insert_internal(
		eTree *tree,
		ePointer  key,
		ePointer  value,
		ebool  replace)
{
	eTreeNode *node;
	eTreeNode *path[MAX_GTREE_HEIGHT];
	int idx;

	e_return_if_fail(tree != NULL);

	if (!tree->root) {
		tree->root = e_tree_node_new(key, value);
		tree->nnodes++;
		return;
	}

	idx = 0;
	path[idx++] = NULL;
	node = tree->root;

	while (1) {
		int cmp = tree->key_compare(key, node->key, tree->key_compare_data);

		if (cmp == 0) {
			if (tree->value_destroy_func)
				tree->value_destroy_func(node->value);

			node->value = value;

			if (replace) {
				if (tree->key_destroy_func)
					tree->key_destroy_func(node->key);

				node->key = key;
			}
			else {
				if (tree->key_destroy_func)
					tree->key_destroy_func(key);
			}

			return;
		}
		else if (cmp < 0) {
			if (node->left_child) {
				path[idx++] = node;
				node = node->left;
			}
			else {
				eTreeNode *child = e_tree_node_new(key, value);

				child->left = node->left;
				child->right = node;
				node->left = child;
				node->left_child = etrue;
				node->balance -= 1;

				tree->nnodes++;

				break;
			}
		}
		else {
			if (node->right_child) {
				path[idx++] = node;
				node = node->right;
			}
			else {
				eTreeNode *child = e_tree_node_new(key, value);

				child->right = node->right;
				child->left = node;
				node->right = child;
				node->right_child = etrue;
				node->balance += 1;

				tree->nnodes++;

				break;
			}
		}
	}

	while (1) {
		eTreeNode *bparent = path[--idx];
		ebool left_node = (bparent && node == bparent->left);
		e_assert(!bparent || bparent->left == node || bparent->right == node);

		if (node->balance < -1 || node->balance > 1) {
			node = e_tree_node_balance(node);
			if (bparent == NULL)
				tree->root = node;
			else if (left_node)
				bparent->left = node;
			else
				bparent->right = node;
		}

		if (node->balance == 0 || bparent == NULL)
			break;

		if (left_node)
			bparent->balance -= 1;
		else
			bparent->balance += 1;

		node = bparent;
	}
}

ebool e_tree_remove(eTree *tree, ePointer key)
{
	ebool removed;

	e_return_val_if_fail(tree != NULL, efalse);

	removed = e_tree_remove_internal(tree, key, efalse);

	return removed;
}

ebool e_tree_steal(eTree *tree, ePointer key)
{
	ebool removed;

	e_return_val_if_fail(tree != NULL, efalse);

	removed = e_tree_remove_internal(tree, key, etrue);

	return removed;
}

static ebool e_tree_remove_internal(eTree *tree, ePointer key, ebool steal)
{
	eTreeNode *node, *parent, *balance;
	eTreeNode *path[MAX_GTREE_HEIGHT];
	int idx;
	ebool left_node;

	e_return_val_if_fail(tree != NULL, efalse);

	if (!tree->root)
		return efalse;

	idx = 0;
	path[idx++] = NULL;
	node = tree->root;

	while (1) {
		int cmp = tree->key_compare(key, node->key, tree->key_compare_data);

		if (cmp == 0)
			break;
		else if (cmp < 0) {
			if (!node->left_child)
				return efalse;

			path[idx++] = node;
			node = node->left;
		}
		else {
			if (!node->right_child)
				return efalse;

			path[idx++] = node;
			node = node->right;
		}
	}

	balance = parent = path[--idx];
	e_assert(!parent || parent->left == node || parent->right == node);
	left_node = (parent && node == parent->left);

	if (!node->left_child)
	{
		if (!node->right_child) {
			if (!parent) {
				tree->root = NULL;
			}
			else if (left_node) {
				parent->left_child = efalse;
				parent->left = node->left;
				parent->balance += 1;
			}
			else {
				parent->right_child = efalse;
				parent->right = node->right;
				parent->balance -= 1;
			}
		}
		else {
			eTreeNode *tmp = e_tree_node_next (node);
			tmp->left = node->left;

			if (!parent) {
				tree->root = node->right;
			}
			else if (left_node) {
				parent->left = node->right;
				parent->balance += 1;
			}
			else {
				parent->right = node->right;
				parent->balance -= 1;
			}
		}
	}
	else {
		if (!node->right_child) {
			eTreeNode *tmp = e_tree_node_previous(node);
			tmp->right = node->right;

			if (parent == NULL) {
				tree->root = node->left;
			}
			else if (left_node) {
				parent->left = node->left;
				parent->balance += 1;
			}
			else {
				parent->right = node->left;
				parent->balance -= 1;
			}
		}
		else {
			eTreeNode *prev = node->left;
			eTreeNode *next = node->right;
			eTreeNode *nextp = node;
			int old_idx = idx + 1;
			idx++;

			while (next->left_child) {
				path[++idx] = nextp = next;
				next = next->left;
			}

			path[old_idx] = next;
			balance = path[idx];

			if (nextp != node) {
				if (next->right_child)
					nextp->left = next->right;
				else
					nextp->left_child = efalse;
				nextp->balance += 1;

				next->right_child = etrue;
				next->right = node->right;
			}
			else
				node->balance -= 1;

			while (prev->right_child)
				prev = prev->right;
			prev->right = next;

			next->left_child = etrue;
			next->left = node->left;
			next->balance = node->balance;

			if (!parent)
				tree->root = next;
			else if (left_node)
				parent->left = next;
			else
				parent->right = next;
		}
	}

	if (balance)
		while (1)
		{
			eTreeNode *bparent = path[--idx];
			e_assert(!bparent || bparent->left == balance || bparent->right == balance);
			left_node = (bparent && balance == bparent->left);

			if(balance->balance < -1 || balance->balance > 1)
			{
				balance = e_tree_node_balance (balance);
				if (!bparent)
					tree->root = balance;
				else if (left_node)
					bparent->left = balance;
				else
					bparent->right = balance;
			}

			if (balance->balance != 0 || !bparent)
				break;

			if (left_node)
				bparent->balance += 1;
			else
				bparent->balance -= 1;

			balance = bparent;
		}

	if (!steal) {
		if (tree->key_destroy_func)
			tree->key_destroy_func(node->key);
		if (tree->value_destroy_func)
			tree->value_destroy_func(node->value);
	}

	e_slice_free(eTreeNode, node);

	tree->nnodes--;

	return etrue;
}

ePointer e_tree_lookup(eTree *tree, eConstPointer key)
{
	eTreeNode *node;

	e_return_val_if_fail(tree != NULL, NULL);

	node = e_tree_find_node(tree, key);

	return node ? node->value : NULL;
}

ebool e_tree_lookup_extended(eTree *tree,
		ePointer        lookup_key,
		ePointer      *orig_key,
		ePointer      *value)
{
	eTreeNode *node;

	e_return_val_if_fail(tree != NULL, efalse);

	node = e_tree_find_node(tree, lookup_key);

	if (node) {
		if (orig_key)
			*orig_key = node->key;
		if (value)
			*value = node->value;
		return etrue;
	}
	else
		return efalse;
}

void e_tree_foreach(eTree *tree, eTraverseFunc func, ePointer user_data)
{
	eTreeNode *node;

	e_return_if_fail(tree != NULL);

	if (!tree->root)
		return;

	node = e_tree_first_node(tree);

	while (node) {
		if ((*func)(node->key, node->value, user_data))
			break;
		node = e_tree_node_next(node);
	}
}

/*
void e_tree_traverse(eTree *tree,
		eTraverseFunc  traverse_func,
		eTraverseType  traverse_type,
		ePointer       user_data)
{
	e_return_if_fail(tree != NULL);

	if (!tree->root)
		return;

	switch (traverse_type) {
		case G_PRE_ORDER:
			e_tree_node_pre_order(tree->root, traverse_func, user_data);
			break;

		case G_IN_ORDER:
			e_tree_node_in_order(tree->root, traverse_func, user_data);
			break;

		case G_POST_ORDER:
			e_tree_node_post_order(tree->root, traverse_func, user_data);
			break;

		case G_LEVEL_ORDER:
			e_warning("e_tree_traverse(): traverse type e_LEVEL_ORDER isn't implemented.");
			break;
	}
}
*/

ePointer e_tree_search(eTree *tree, eCompareFunc search_func, eConstPointer user_data)
{
	e_return_val_if_fail(tree != NULL, NULL);

	if (tree->root)
		return e_tree_node_search(tree->root, search_func, user_data);
	else
		return NULL;
}

eint e_tree_height(eTree *tree)
{
	eTreeNode *node;
	eint height;

	e_return_val_if_fail(tree != NULL, 0);

	if (!tree->root)
		return 0;

	height = 0;
	node = tree->root;

	while (1) {
		height += 1 + MAX(node->balance, 0);

		if (!node->left_child)
			return height;

		node = node->left;
	}
}

eint e_tree_nnodes(eTree *tree)
{
	e_return_val_if_fail(tree != NULL, 0);

	return tree->nnodes;
}

static eTreeNode *e_tree_node_balance(eTreeNode *node)
{
	if (node->balance < -1) {
		if (node->left->balance > 0)
			node->left = e_tree_node_rotate_left(node->left);
		node = e_tree_node_rotate_right(node);
	}
	else if (node->balance > 1) {
		if (node->right->balance < 0)
			node->right = e_tree_node_rotate_right(node->right);
		node = e_tree_node_rotate_left (node);
	}

	return node;
}

static eTreeNode *e_tree_find_node(eTree *tree, eConstPointer key)
{
	eTreeNode *node;
	eint cmp;

	node = tree->root;
	if (!node)
		return NULL;

	while (1) {
		cmp = tree->key_compare(key, node->key, tree->key_compare_data);
		if (cmp == 0) {
			return node;
		}
		else if (cmp < 0) {
			if (!node->left_child)
				return NULL;

			node = node->left;
		}
		else {
			if (!node->right_child)
				return NULL;

			node = node->right;
		}
	}
}

/*
static eint e_tree_node_pre_order(eTreeNode *node, eTraverseFunc traverse_func, ePointer data)
{
	if ((*traverse_func)(node->key, node->value, data))
		return etrue;

	if (node->left_child) {
		if (e_tree_node_pre_order(node->left, traverse_func, data))
			return etrue;
	}

	if (node->right_child) {
		if (e_tree_node_pre_order(node->right, traverse_func, data))
			return etrue;
	}

	return efalse;
}

static eint e_tree_node_in_order(eTreeNode *node, eTraverseFunc traverse_func, ePointer data)
{
	if (node->left_child) {
		if (e_tree_node_in_order(node->left, traverse_func, data))
			return etrue;
	}

	if ((*traverse_func)(node->key, node->value, data))
		return etrue;

	if (node->right_child) {
		if (e_tree_node_in_order(node->right, traverse_func, data))
			return etrue;
	}

	return efalse;
}

static eint e_tree_node_post_order(eTreeNode *node, eTraverseFunc traverse_func, ePointer data)
{
	if (node->left_child) {
		if (e_tree_node_post_order(node->left, traverse_func, data))
			return etrue;
	}

	if (node->right_child) {
		if (e_tree_node_post_order(node->right, traverse_func, data))
			return etrue;
	}

	if ((*traverse_func)(node->key, node->value, data))
		return etrue;

	return efalse;
}
*/

static ePointer e_tree_node_search(eTreeNode *node, eCompareFunc search_func, eConstPointer data)
{
	eint dir;

	if (!node)
		return NULL;

	while (1) {
		dir = (*search_func)(node->key, data);
		if (dir == 0) {
			return node->value;
		}
		else if (dir < 0) { 
			if (!node->left_child)
				return NULL;

			node = node->left;
		}
		else {
			if (!node->right_child)
				return NULL;

			node = node->right;
		}
	}
}

static eTreeNode* e_tree_node_rotate_left(eTreeNode *node)
{
	eTreeNode *right;
	eint a_bal;
	eint b_bal;

	right = node->right;

	if (right->left_child)
		node->right = right->left;
	else {
		node->right_child = efalse;
		node->right = right;
		right->left_child = etrue;
	}
	right->left = node;

	a_bal = node->balance;
	b_bal = right->balance;

	if (b_bal <= 0) {
		if (a_bal >= 1)
			right->balance = b_bal - 1;
		else
			right->balance = a_bal + b_bal - 2;
		node->balance = a_bal - 1;
	}
	else {
		if (a_bal <= b_bal)
			right->balance = a_bal - 2;
		else
			right->balance = b_bal - 1;
		node->balance = a_bal - b_bal - 1;
	}

	return right;
}

static eTreeNode* e_tree_node_rotate_right(eTreeNode *node)
{
	eTreeNode *left;
	eint a_bal;
	eint b_bal;

	left = node->left;

	if (left->right_child) {
		node->left = left->right;
	}
	else {
		node->left_child = efalse;
		node->left = left;
		left->right_child = etrue;
	}
	left->right = node;

	a_bal = node->balance;
	b_bal = left->balance;

	if (b_bal <= 0) {
		if (b_bal > a_bal)
			left->balance = b_bal + 1;
		else
			left->balance = a_bal + 2;
		node->balance = a_bal - b_bal + 1;
	}
	else {
		if (a_bal <= -1)
			left->balance = b_bal + 1;
		else
			left->balance = a_bal + b_bal + 2;
		node->balance = a_bal + 1;
	}

	return left;
}

