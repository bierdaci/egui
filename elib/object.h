#ifndef __ELIB_CELL_H
#define __ELIB_CELL_H

#include <elib/types.h>
#include <elib/std.h>

#define GTYPE_CELL				(e_genetype_cell())
#define CELL_ORDERS(hobj)		((eCellOrders *)e_object_type_data(hobj, GTYPE_CELL))

#define DNA_ARGS_MAX				128
#define PARENT_MAX					100
#define DNA_ID_NULL					((edna_t)-1)

#define OBJECT_OFFSET(addr) \
	((eHandle)(((echar *)(addr)) - *(((eulong *)(addr)) - 1)))

#define OBJECT_STRUCT_OFFSET(addr, type, offset) \
	(OBJECT_OFFSET(((echar *)(addr) - STRUCT_OFFSET(type, offset))))

typedef struct _eCell			eCell;
typedef struct _eCellOrders		eCellOrders;
typedef struct _eObject			eObject;
typedef struct _eGene			eGene;
typedef struct _eGeneInfo		eGeneInfo;
typedef struct _eDnaNode		eDnaNode;
typedef struct _eDnaInfo		eDnaInfo;
typedef struct _eBranchIndex	eBranchIndex;

typedef euchar *				eorder_t;
typedef eulong					eGeneType;
typedef euint16					edna_t;

struct _eBranchIndex {
	edna_t offset;
	edna_t node_num;
};

struct _eDnaInfo {
	euint32 object_size;
	euint32 orders_size;

	eint (*init_data)(eHandle, ePointer);
	void (*free_data)(eHandle, ePointer);
	void (*init_orders)(eGeneType, ePointer);

	euint32 object_offset;
	euint32 orders_offset;
};

struct _eDnaNode {
	edna_t id;
	edna_t type;

	eDnaInfo info;
	eGene   *owner;

	edna_t branch_num;
	edna_t branch_size;
	edna_t min, max;

	eBranchIndex index[0];
};

struct _eGene {
	eDnaNode *nodes;
	eDnaNode *last_node;
	euint16 node_num;
	euint16 child_seq;
	euint32 object_size;
	euint32 orders_size;
	eorder_t orders_base;
	struct _eGene **child_index;
	e_thread_mutex_t lock;
};

struct _eObject {
	eGene    *gene;
	ePointer *slot_head;
	ePointer *slot_tail;
	e_thread_mutex_t slot_lock;
	eint ref_count;
};

struct _eGeneInfo {
	euint32 orders_size;
	void (*init_orders)(eGeneType, ePointer);

	euint32 object_size;
	eint (*init_data)(eHandle, ePointer);
	void (*free_data)(eHandle, ePointer);

	void (*init_gene)(eGeneType);
};

struct _eCell {
#ifdef WIN32
	int a;
#endif
};

struct _eCellOrders {
	void (*refer)(eHandle);
	void (*unref)(eHandle);
	eint (* init)(eHandle, eValist vp);
	void (* free)(eHandle);
};

eGeneType e_genetype_cell(void);

eHandle e_object_new(eGeneType, ...);
eHandle e_object_refer(eHandle);
void    e_object_unref(eHandle);

ePointer e_object_type_data(eHandle, eGeneType);
ePointer e_object_type_orders(eHandle, eGeneType);
ePointer e_type_orders(eGeneType);
ePointer e_genetype_orders(eGeneType, eGeneType);
ebool e_genetype_check(eGeneType, eGeneType);
ebool e_object_type_check(eHandle, eGeneType);
eDnaNode *e_genetype_node(eGene *, eGeneType);


eGeneType e_register_genetype(eGeneInfo *, eGeneType, ...);
ebool e_object_valid(eObject *);

void e_object_init(void);

#endif
