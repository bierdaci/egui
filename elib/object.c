#include "memory.h"
#include "object.h"
#include "esignal.h"

#define BRANCH_NUM_MAX				0xff
#define NODE_LOWER_LEVEL(p) \
		((eDnaNode *)((euchar *)((eDnaNode *)p + 1) + ((eDnaNode *)p)->branch_size))

typedef struct _eDnaList  eDnaList;
typedef struct _BranchList {
	eDnaList *head;
	struct _BranchList *next;
} eBranchList;

struct _eDnaList {
	edna_t id;
	edna_t type;
	edna_t min, max;
	eint branch_num;
	eGene *owner;
	eBranchList *branch_head;
	struct _eDnaList *next;
	eDnaInfo info;
};

static void gene_add_to_library(eGene *, eGene *);
static eDnaList *dna_list_which_contain(eDnaList *, eDnaList *);
static eHandle object_new_valist(eGene *, eValist);

static eGene *__genetype_library[1];
#ifdef WIN32
static e_thread_mutex_t object_lock = {0};
#elif  linux
static e_thread_mutex_t object_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

static eDnaList *__dna_list_which_contain(eDnaList *list1, eDnaList *list2)
{
	eDnaList *n1 = list1;
	eDnaList *n2 = list2;
	eDnaList *p;

	while (n1 && n2) {
		if (n1->id != n2->id)
			break;
		n1 = n1->next;
		n2 = n2->next;
	}

	if ((n1 && !n2) || (!n1 && !n2))
		return list1;
	if (!n1 && n2)
		return list2;

	p = dna_list_which_contain(n1, n2);
	if (!p)
		return NULL;
	if (p == n1)
		return list1;
	if (p == n2)
		return list2;

	return NULL;
}

static eDnaList *__dna_combin_which_contain(eDnaList *com1, eDnaList *com2, eDnaList *sin)
{
	eBranchList *branch;
	eDnaList    *t;

	if (com1->id == sin->id) {
		t = __dna_list_which_contain(com1, sin);
		if (t == com1)
			return com1;
		return com2;
	}

	branch = com1->branch_head;
	while (branch) {
		if (branch->head->id == sin->id) {
			t = __dna_list_which_contain(branch->head, sin);
			if (t == branch->head)
				return com1;
			return com2;
		}
		branch = branch->next;
	}

	return NULL;
}

static eDnaList *dna_combin_which_contain(eDnaList *list1, eDnaList *list2)
{
	eDnaList *p1, *p2, *p = NULL;
	eBranchList *branch;

	if (list2->branch_num > list1->branch_num) {
		p1 = list2;
		p2 = list1;
	}
	else {
		p1 = list1;
		p2 = list2;
	}

	if (!(p = __dna_combin_which_contain(p1, p2, p2)))
		return NULL;

	branch = p2->branch_head;
	while (branch) {
		eDnaList *t = __dna_combin_which_contain(p1, p2, branch->head);
		if (!t || t != p)
			return NULL;
		branch = branch->next;
	}

	if (p == p1)
		return p1;
	if (p1->branch_num > p2->branch_num)
		return NULL;
	return p2;
}

static eDnaList *dna_combin_contain(eDnaList *com, eDnaList *list);
static eDnaList *dna_list_contain(eDnaList *list1, eDnaList *list2)
{
	eDnaList *n1 = list1;
	eDnaList *n2 = list2;

	while (n1 && n2 && n1->id == n2->id) {
		n1 = n1->next;
		n2 = n2->next;
	}

	if ((n1 && !n2) || (!n1 && !n2))
		return list1;

	if (!n1 && n2)
		return NULL;

	if (n1->branch_head && dna_combin_contain(n1, n2))
		return list1;
	return NULL;
}

static eDnaList *dna_combin_contain(eDnaList *com, eDnaList *list)
{
	eBranchList *branch;

	branch = com->branch_head;
	while (branch) {
		if (branch->head->id == list->id) {
			if (dna_list_contain(branch->head, list))
				return com;
			break;
		}
		branch = branch->next;
	}
	return NULL;
}

static eDnaList *dna_list_which_contain(eDnaList *list1, eDnaList *list2)
{
	if (!list1->branch_head && !list2->branch_head) {
		if (list1->id == list2->id)
			return __dna_list_which_contain(list1, list2);
		return NULL;
	}

	if (list1->branch_head && list2->branch_head)
		return dna_combin_which_contain(list1, list2);

	if (list1->branch_head)
		return dna_combin_contain(list1, list2);
	return dna_combin_contain(list2, list1);
}

static eBranchList *branch_list_new(eDnaList *list)
{
	eBranchList *branch = (eBranchList *)e_slice_new(eDnaList); //eBranchList
	branch->next = NULL;
	branch->head = list;
	return branch;
}

static eDnaList *dna_list_merger(eDnaList **, eDnaList **);
static void __dna_list_insert_branch(eDnaList *com, eBranchList *branch, eDnaList *blist)
{
	eBranchList **pp;

	if (blist->id >= com->min && blist->id <= com->max) {
		eBranchList *b = com->branch_head;
		while (b) {
			eDnaList *t = b->head;
			if (t->id == blist->id) {
				b->head = dna_list_merger(&t, &blist);
				if (branch)
					branch->head = NULL;
				return;
			}
			b = b->next;
		}
	}

	if (!branch)
		branch = branch_list_new(blist);

	pp = &com->branch_head;
	while (*pp) {
		eDnaList *t = (*pp)->head;
		if (blist->id < t->id) {
			if (com->min > blist->id)
				com->min = blist->id;
			branch->next = *pp;
			*pp = branch;
			break;
		}
		pp = &(*pp)->next;
	}

	if (!(*pp)) {
		com->max = blist->id;
		*pp = branch;
	}

	com->branch_num++;
}

static void dna_list_insert_branch(eDnaList *com, eDnaList *list)
{
	if (list->branch_head) {
		eBranchList *branch = list->branch_head;
		while (branch) {
			eBranchList *t = branch;
			branch  = branch->next;
			t->next = NULL;

			__dna_list_insert_branch(com, t, t->head);
			if (!t->head)
				e_slice_free(eDnaList, t); //eBranchList
		}

		if (list->id != DNA_ID_NULL) {
			list->branch_num  = 0;
			list->branch_head = NULL;
			__dna_list_insert_branch(com, NULL, list);
		}
		else e_slice_free(eDnaList, list);
	}
	else
		__dna_list_insert_branch(com, NULL, list);
}

static eDnaList *dna_list_new(eDnaNode *node)
{
	eDnaList *newlist = e_slice_new(eDnaList);
	if (node) {
		newlist->id    = node->id;
		newlist->owner = node->owner;
		newlist->next  = NULL;
		newlist->min   = node->min;
		newlist->max   = node->max;
		newlist->branch_num  = node->branch_num;
		newlist->branch_head = NULL;
		if (node->id != DNA_ID_NULL)
			e_memcpy(&newlist->info, &node->info, sizeof(eDnaInfo));
	}
	else {
		newlist->id    = DNA_ID_NULL;
		newlist->next  = NULL;
		newlist->owner = NULL;
	}

	return newlist;
}

static void branch_list_empty(eBranchList *);
static void dna_list_free(eDnaList *dnalist)
{
	eDnaList *p = dnalist;
	while (p) {
		eDnaList *t = p;
		p = p->next;
		if (t->branch_head) {
			branch_list_empty(t->branch_head);
		}
		e_slice_free(eDnaList, t);
	}
}

static void branch_list_empty(eBranchList *blist)
{
	eBranchList *p = blist;
	while (p) {
		dna_list_free(p->head);
		p = p->next;
	}
	e_slice_free(eDnaList, blist); //eBranchList
}

static eDnaList *__dna_list_merger(eDnaList **_list1, eDnaList **_list2)
{
	eDnaList **list1, **list2;
	eDnaList  *list;
	eDnaList  *newlist = dna_list_new(NULL);

	if (!(*_list1)->branch_head && (*_list2)->branch_head) {
		list1 = _list2;
		list2 = _list1;
	}
	else {
		list1 = _list1;
		list2 = _list2;
	}

	list = *list1;
	if (!list->branch_head) {
		eBranchList *n = branch_list_new(list);

		newlist->branch_num  = 1;
		newlist->branch_head = n;
		newlist->min = list->id;
		newlist->max = list->id;
	}
	else {
		newlist->branch_num  = list->branch_num;
		newlist->branch_head = list->branch_head;
		newlist->min = list->min;
		newlist->max = list->max;
		if (list->id != DNA_ID_NULL) {
			eBranchList **p = &newlist->branch_head;
			eBranchList  *b = branch_list_new(list);

			list->branch_num  = 0;
			list->branch_head = NULL;
			while (*p) {
				p = &(*p)->next;
			}
			*p = b;

			newlist->branch_num++;
			newlist->max = list->id;
		}
		else
			e_slice_free(eDnaList, list);
	}

	dna_list_insert_branch(newlist, *list2);
	*list2 = NULL;
	*list1 = newlist;

	return newlist;
}

static eDnaList *dna_list_merger(eDnaList **list1, eDnaList **list2)
{
	eDnaList **n1 = list1;
	eDnaList **n2 = list2;

	eDnaList *t = dna_list_which_contain(*list1, *list2);
	if (t) {
		if (t == *list1) {
			dna_list_free(*list2);
			*list2 = NULL;
		}
		else {
			dna_list_free(*list1);
			*list1 = NULL;
		}
		return t;
	}

	while (*n1 && *n2 && (*n1)->id == (*n2)->id) {
		n1 = &(*n1)->next;
		n2 = &(*n2)->next;
	}

	__dna_list_merger(n1, n2);
	if (!*n1) {
		dna_list_free(*list1);
		*list1 = NULL;
		return *list2;
	}

	dna_list_free(*list2);
	*list2 = NULL;

	return *list1;
}

static eDnaList* dna_node_to_list(eDnaNode *nodes, eint node_num)
{
	eDnaNode *node = nodes;
	eDnaList *head = NULL;
	eDnaList *tail;

	eint i;
	for (i = 0; i < node_num; i++) {
		eDnaList *list = dna_list_new(node);

		if (!head) {
			head = list;
			tail = head;
		}
		else {
			tail->next = list;
			tail = list;
		}

		if (node->branch_num != 0) {
			euchar      *base  = (euchar *)&node->index[node->branch_num];
			eBranchList *btail = NULL;

			eint j;
			for (j = 0; j < list->branch_num; j++) {
				eBranchIndex *index  = &node->index[j];
				eBranchList  *brlist = branch_list_new(NULL);

				if (!list->branch_head) {
					list->branch_head = brlist;
					btail = brlist;
				}
				else {
					btail->next = brlist;
					btail = brlist;
				}

				brlist->head = dna_node_to_list((eDnaNode *)(base + index->offset), index->node_num);
			}
		}

		node = NODE_LOWER_LEVEL(node);
	}

	return head;
}

static eDnaList *resolve_gene_dna(eGene *gene)
{
	return dna_node_to_list(gene->nodes, gene->node_num);
}

static eint dna_list_get_size(eDnaList *, eint *, eint *);
static eint dna_combin_get_size(eDnaList *list, eint *orders_size, eint *object_size)
{
	eint size = 0;
	eBranchList *branch = list->branch_head;

	while (branch) {
		size += dna_list_get_size(branch->head, orders_size, object_size);
		branch = branch->next;
	}
	return size;
}

static eint dna_list_get_size(eDnaList *dnalist, eint *orders_size, eint *object_size)
{
	eDnaList *p = dnalist;
	eint  size = 0;

	while (p) {
		if (p->branch_num > 0) {
			size += sizeof(eBranchIndex) * p->branch_num;
			size += dna_combin_get_size(p, orders_size, object_size);
		}
		if (p->id != DNA_ID_NULL) {
			*orders_size += p->info.orders_size;
			*object_size += p->info.object_size + sizeof(ePointer);
		}
		size += sizeof(eDnaNode);
		p = p->next;
	}
	return size;
}

static eint dna_list_get_num(eDnaList *dnalist)
{
	eint num = 0;
	while (dnalist) {
		num ++;
		dnalist = dnalist->next;
	}
	return num;
}

static eDnaList *dna_list_get_last(eDnaList *dnalist, eDnaList **parent)
{
	eDnaList *n = dnalist;
	eDnaList *p = NULL;

	while (n->next) {
		p = n;
		n = n->next;
	}

	if (n->id == DNA_ID_NULL)
		*parent = p;
	else
		*parent = n;

	return n;
}

static eDnaNode *dna_index_to_node(eDnaNode *nodes, eint node_num)
{
	eDnaNode *p = nodes;
	eint i;

	for (i = 0; i < node_num - 1; i++)
		p = NODE_LOWER_LEVEL(p);

	return p;
}

static euint dna_list_to_node(eDnaNode *nodes, eDnaList *head, eint *orders_offset, eint *object_offset)
{
	eDnaList *p = head;
	eDnaNode *n = nodes;
	eint i;

	for (i = 0; p; p = p->next, i++) {
		n->id   = p->id;
		n->type = p->type;

		if (n->id != DNA_ID_NULL) {
			n->owner = p->owner;
			e_memcpy(&n->info, &p->info, sizeof(eDnaInfo));
			n->info.orders_offset = *orders_offset;
			n->info.object_offset = *object_offset;

			*orders_offset += n->info.orders_size;
			*object_offset += n->info.object_size + sizeof(ePointer);
		}

		if (p->branch_num > 0) {
			eBranchList *b = p->branch_head;
			eorder_t  base = (eorder_t)&n->index[p->branch_num];
			eint j;

			n->branch_num  = p->branch_num;
			n->branch_size = n->branch_num * sizeof(eBranchIndex);
			n->min = p->min;
			n->max = p->max;
			for (j = 0; b; b = b->next, j++) {
				eBranchIndex *index = &n->index[j];

				index->node_num = dna_list_get_num(b->head);
				index->offset   = base - (eorder_t)&n->index[p->branch_num];
				base += dna_list_to_node((eDnaNode *)base, b->head, orders_offset, object_offset);
			}
			n->branch_size += base - (eorder_t)&n->index[n->branch_num];
		}
		else {
			n->branch_num  = 0;
			n->branch_size = 0;
		}

		n = NODE_LOWER_LEVEL(n);
	}

	return (euint)((euchar *)n - (euchar *)nodes);
}

static void dna_node_init_orders(eGene *new, eDnaNode *nodes, eint node_num, eorder_t orders_base)
{
	eDnaNode *p = nodes;

	eint i;
	for (i = 0; i < node_num; i++) {
		if (p->branch_num > 0) {
			eorder_t base = (eorder_t)&p->index[p->branch_num];
			eint j;
			for (j = 0; j < p->branch_num; j++) {
				eBranchIndex *index = &p->index[j];
				dna_node_init_orders(new, (eDnaNode *)(base + index->offset), index->node_num, orders_base);
			}
		}
		if (p->id != DNA_ID_NULL && p->info.init_orders) {
			if (p->info.orders_size > 0)
				p->info.init_orders((eGeneType)new, new->orders_base + p->info.orders_offset);
			else
				p->info.init_orders((eGeneType)new, NULL);
		}
		p = NODE_LOWER_LEVEL(p);
	}
}

static eGeneType register_genetype_with_dnalist(eGeneInfo *geinfo, eDnaList *dnalist)
{
	eGene    *new;
	eDnaNode *node;
	eDnaList *parent, *last;

	eint orders_size = 0;
	eint object_size = 0;
	eint orders_offset = 0;
	eint object_offset = sizeof(eObject) + sizeof(ePointer);
	eint list_num, node_size;
	eint node_num;

	node_size = dna_list_get_size(dnalist, &orders_size, &object_size);
	list_num  = dna_list_get_num(dnalist);
	last      = dna_list_get_last(dnalist, &parent);
	node_num  = list_num;
	if (last == parent) {
		node_num ++;
		node_size += sizeof(eDnaNode);
	}

	if (!(new = e_calloc(1, sizeof(eGene) + node_size + orders_size + geinfo->orders_size)))
		return 0;

	new->nodes       = (eDnaNode *)((echar *)(new + 1));
	new->node_num    = node_num;
	new->orders_base = (eorder_t)new->nodes + node_size;
	new->child_seq   = 0;
	new->orders_size = orders_size + geinfo->orders_size;
	new->object_size = object_size + geinfo->object_size + sizeof(ePointer);
	new->child_index = NULL;
	e_thread_mutex_init(&new->lock, NULL);

	dna_list_to_node(new->nodes, dnalist, &orders_offset, &object_offset);

	new->last_node = dna_index_to_node(new->nodes, node_num);
	node = new->last_node;
	node->info.object_size = geinfo->object_size;
	node->info.orders_size = geinfo->orders_size;
	node->info.orders_offset = orders_offset;
	node->info.object_offset = object_offset;

	node->info.init_data   = geinfo->init_data;
	node->info.free_data   = geinfo->free_data;
	node->info.init_orders = geinfo->init_orders;

	node->id    = parent->owner->child_seq++;
	node->owner = new;
	if (list_num < node_num) {
		node->branch_num  = 0;
		node->branch_size = 0;
	}

	dna_node_init_orders(new, new->nodes, node_num, new->orders_base);

	if (geinfo->init_gene)
		geinfo->init_gene((eGeneType)new);

	gene_add_to_library(parent->owner, new);
	dna_list_free(dnalist);

	return (eGeneType)new;
}

eGeneType e_register_genetype(eGeneInfo *geinfo, eGeneType gtype, ...)
{
	eDnaList *lists[PARENT_MAX];
	eDnaList *dlist;
	eGene *p  = (eGene *)gtype;
	eint   np = 0, i;

	eValist vp;

	e_va_start(vp, gtype);
	for (; p && np < PARENT_MAX; np++) {
		lists[np] = resolve_gene_dna(p);
		p = e_va_arg(vp, eGene *);
	}
	e_va_end(vp);

	if (np == 0)
		return 0;

	dlist = lists[0];
	for (i = 1; i < np; i++) {
		dlist = dna_list_merger(&dlist, &lists[i]);
	}

	return register_genetype_with_dnalist(geinfo, dlist);
}

static void gene_add_to_library(eGene *parent, eGene *new)
{
	e_thread_mutex_lock(&object_lock);

	if (!parent->child_index)
		parent->child_index = e_malloc(sizeof(eGene *));
	else
		parent->child_index = e_realloc(parent->child_index, sizeof(eGene *) * parent->child_seq);

	parent->child_index[new->last_node->id] = new;

	e_thread_mutex_unlock(&object_lock);

	return;
}

ebool e_gene_match(eGene *gene1, eGene *gene2)
{
	if (gene1->node_num != gene2->node_num)
		return efalse;

	if (!e_memcmp(gene1->nodes, gene2->nodes, sizeof(eDnaNode) * gene1->node_num))
		return etrue;

	return efalse;
}

static eDnaNode *__dnode_find_subset(eDnaNode *, eint, eDnaNode *, eint);
static eDnaNode *__dnode_branch_find_subset_cast(eDnaNode *nodes, eDnaNode *type, eint n2)
{
	euchar *base = (euchar *)&nodes->index[nodes->branch_num];

	elong i;
	for (i = 0; i < nodes->branch_num; i++) {
		eBranchIndex *index = &nodes->index[i];
		eDnaNode *t = (eDnaNode *)(base + index->offset);
		eDnaNode *v = __dnode_find_subset(t, index->node_num, type, n2);
		if (v)
			return v;
	}
	return (eDnaNode *)i;
}

static eDnaNode *__dnode_find_subset(eDnaNode *nodes, eint n1, eDnaNode *type, eint n2)
{
	eDnaNode *p1 = nodes;
	eDnaNode *p2 = type;

	elong i = 0;
	do {
		if (p2->id > p1->id)
			return (eDnaNode *)i;

		if (p2->id < p1->id) {
			if (!p1->branch_num)
				return (eDnaNode *)i;
			if (p2->id < p1->min || p2->id > p1->max)
				return (eDnaNode *)i;
			return __dnode_branch_find_subset_cast(p1, p2, n2 - i);
		}

		i++;
	} while (i < n1 && i < n2 && (p1 = NODE_LOWER_LEVEL(p1), p2 = NODE_LOWER_LEVEL(p2)));

	if (i < n2)
		return (eDnaNode *)i;
	return p1;
}

static eDnaNode *dnode_find_subset(eDnaNode *nodes, eint n1, eDnaNode *type, eint n2)
{
	eDnaNode *node = __dnode_find_subset(nodes, n1, type, n2);
	if (node && (eulong)node > BRANCH_NUM_MAX)
		return node;
	return NULL;
}

ePointer e_gene_orders_cast(eGeneType gtype)
{
	if (((eGene *)gtype)->last_node->info.orders_size == 0)
		return NULL;
	return ((eGene *)gtype)->orders_base + ((eGene *)gtype)->last_node->info.orders_offset;
}

eDnaNode *e_genetype_node(eGene *gene, eGeneType type)
{
	return dnode_find_subset(gene->nodes, gene->node_num, 
			((eGene *)type)->nodes, ((eGene *)type)->node_num);
}

ebool e_genetype_check(eGeneType gene, eGeneType type)
{
	return e_genetype_node((eGene *)gene, type) ? etrue : efalse;
}

ePointer e_type_orders(eGeneType gtype)
{
	eGene *g = (eGene *)gtype;
	return g->orders_base + g->last_node->info.orders_offset;
}

ePointer e_genetype_orders(eGeneType gene, eGeneType type)
{
	eDnaNode *node = e_genetype_node((eGene *)gene, type);
	if (!node || node->info.orders_size == 0)
		return NULL;
	return ((eGene *)gene)->orders_base + node->info.orders_offset;
}

static void cell_init_orders(eGeneType new, ePointer this)
{
	((eCellOrders *)this)->free  = e_free;
	((eCellOrders *)this)->alloc = e_malloc;;
}

eGeneType e_genetype_cell(void)
{
	static eDnaNode node;
	static eGene    gene      = {NULL};
	static eCellOrders orders = {NULL, NULL, NULL, NULL, NULL, NULL};

	if (gene.nodes == NULL) {
		node.id          = 0;
		node.type        = 0;
		node.branch_num  = 0;
		node.owner       = &gene;

		node.info.object_size = sizeof(eCell);
		node.info.orders_size = sizeof(eCellOrders);
		node.info.init_data   = NULL;
		node.info.free_data   = NULL;
		node.info.init_orders = cell_init_orders;
		node.info.object_offset = sizeof(eObject) + sizeof(ePointer);
		node.info.orders_offset = 0;

		gene.nodes     = &node;
		gene.last_node = &node;
		gene.node_num  = 1;
		gene.child_seq = 0;
		gene.object_size = sizeof(eCell);
		gene.orders_size = sizeof(eCellOrders);
		gene.orders_base = (eorder_t)&orders;

		__genetype_library[0] = &gene;
	}

	return (eGeneType)&gene;
}

eHandle e_object_new(eGeneType gtype, ...)
{
	eHandle hobj;
	eValist vp;

	e_va_start(vp, gtype);
	hobj = object_new_valist((eGene *)gtype, vp);
	e_va_end(vp);

	return hobj;
}

eHandle e_object_refer(eHandle hobj)
{
	eCellOrders *orders = e_object_type_orders(hobj, GTYPE_CELL);

	((eObject *)hobj)->ref_count++;

	if (orders->refer)
		orders->refer(hobj);

	return hobj;
}

static void call_node_free(eObject *obj, eDnaNode *node, eint num)
{
	if (node->branch_num > 0) {
		euchar *base = (euchar *)&node->index[node->branch_num];
		eint j;
		for (j = 0; j < node->branch_num; j++) {
			eBranchIndex *index = &node->index[j];
			eDnaNode *n = (eDnaNode *)(base + index->offset);
			call_node_free(obj, n, index->node_num - 1);
		}
	}

	if (node->id == DNA_ID_NULL)
		return;

	if (num > 0)
		call_node_free(obj, (eDnaNode *)((euchar *)(node + 1) + node->branch_size), num - 1);

	if (node->info.free_data) {
		if (node->info.object_size > 0)
			node->info.free_data((eHandle)obj, (euchar *)obj + node->info.object_offset);
		else
			node->info.free_data((eHandle)obj, NULL);
	}
}

void e_object_unref(eHandle hobj)
{
	eObject     *obj = (eObject *)hobj;
	eCellOrders *ors = (eCellOrders *)obj->gene->orders_base;

	if (obj->ref_count <= 0)
		abort();

	if (ors->unref)
		ors->unref(hobj);

	if (obj->ref_count <= 0)
		abort();

	if (--obj->ref_count == 0) {
		e_signal_emit(hobj, SIG_RELEASE);
		call_node_free(obj, obj->gene->nodes, obj->gene->node_num - 1);
		ors->free(obj);
	}
}

static void __object_set_offset(eDnaNode *nodes, eint node_num, euchar *object_base)
{
	eDnaNode *p = nodes;

	eint i;
	for (i = 0; i < node_num; i++) {
		if (p->id != DNA_ID_NULL) {
			euint32 *offset = (euint32 *)(object_base + p->info.object_offset - sizeof(ePointer));
			*offset = p->info.object_offset;
		}
		if (p->branch_num > 0) {
			eBranchIndex *index = p->index;
			eint j;
			for (j = 0; j < p->branch_num; j++) {
				eDnaNode *n = (eDnaNode *)((euchar *)&index[p->branch_num] + index[j].offset);
				__object_set_offset(n, index[j].node_num, object_base);
			}
		}
		p = NODE_LOWER_LEVEL(p);
	}
}

static void object_set_offset(eGene *gene, euchar *object_base)
{
	__object_set_offset(gene->nodes, gene->node_num, object_base);
}

static eint invoke_node_init(eObject *obj, eDnaNode *nodes, eint n, eint max)
{
	eDnaNode *node = nodes;
	eint retval = 0;

	if (node->branch_num > 0) {
		euchar *base = (euchar *)&node->index[node->branch_num];

		eint j;
		for (j = 0; j < node->branch_num; j++) {
			eBranchIndex *index = &node->index[j];
			invoke_node_init(obj,  (eDnaNode *)(base + index->offset), 0, index->node_num);
		}
	}

	if (node->id != DNA_ID_NULL && node->info.init_data) {
		if (node->info.object_size > 0)
			retval = node->info.init_data((eHandle)obj, (euchar *)obj + node->info.object_offset);
		else
			retval = node->info.init_data((eHandle)obj, NULL);
		if (retval < 0)
			return retval;
	}

	if (++n < max) {
		retval = invoke_node_init(obj, NODE_LOWER_LEVEL(node), n, max);
		if (retval < 0) {
			if (node->info.free_data) {
				if (node->info.object_size > 0)
					node->info.free_data((eHandle)obj, (euchar *)obj + node->info.object_offset);
				else
					node->info.free_data((eHandle)obj, NULL);
			}
		}
	}

	return retval;
}

static eHandle object_new_valist(eGene *gene, eValist vp)
{
	eCellOrders *ors = (eCellOrders *)gene->orders_base;
	eObject     *obj;

	obj = ors->alloc(sizeof(eObject) + gene->object_size);
	memset(obj, 0, sizeof(eObject) + gene->object_size);
	obj->gene = gene;
	e_thread_mutex_init(&obj->slot_lock, NULL);

	object_set_offset(gene, (euchar *)obj);

	if (invoke_node_init(obj, gene->nodes, 0, gene->node_num) < 0) {
		e_free(obj);
		return 0;
	}

	obj->ref_count = 1;
	if (ors->refer)
		ors->refer((eHandle)obj);

	if (ors->init)
		ors->init((eHandle)obj, vp);

	return (eHandle)obj;
}

ebool e_valid_gene(eGene *gene)
{
	eDnaNode *p = gene->nodes;
	eGene *up;
	eint i;

	if (p->id != 0)
		return efalse;

	up = __genetype_library[0];
	for (i = 1; i < gene->node_num; i++) {
		if (p->id > up->child_seq - 1)
			break;
		up = up->child_index[p->id];
		p  = NODE_LOWER_LEVEL(p);
	}

	if (i == gene->node_num && up == gene)
		return etrue;
	return efalse;
}

ebool e_object_valid(eObject *obj)
{
	return e_valid_gene(obj->gene);
}

ePointer e_object_type_data(eHandle hobj, eGeneType type)
{
	eGene    *gene = ((eObject *)hobj)->gene;
	eDnaNode *node = dnode_find_subset(gene->nodes,
			gene->node_num, ((eGene *)type)->nodes, ((eGene *)type)->node_num);
	if (!node || node->info.object_size == 0)
		return NULL;

	return (ePointer *)((echar *)hobj + node->info.object_offset);
}

ebool e_object_type_check(eHandle hobj, eGeneType type)
{
	if (e_genetype_check((eGeneType)((eObject *)hobj)->gene, type))
		return etrue;
	return efalse;
}

ePointer e_object_type_orders(eHandle hobj, eGeneType type)
{
	return e_genetype_orders((eGeneType)((eObject *)hobj)->gene, type);
}

void e_object_init(void)
{
#ifdef WIN32
	e_thread_mutex_init(&object_lock, NULL);
#endif
}

void e_timer_init(void);
ebool e_init(void)
{
	e_memory_init();
	e_object_init();
	e_signal_init();
	e_timer_init();
	return etrue;
}
