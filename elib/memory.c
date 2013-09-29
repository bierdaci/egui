#include "types.h"
#include "list.h"
#include "std.h"
#include "memory.h"

#define HEAP_ALLOC_MAX			(1024 * 512)
#define PAGE_SIZE				4096
#define HEAP_SIZE				(1024 * 1024)
#define CACHE_NUM				256
#define FREE_TABLES_SIZE		10
#define SLICE_TABLES_NUM		(256 / 8)

typedef struct _MemoryChain MemoryChain;
typedef struct _MemoryHeap  MemoryHeap;
typedef struct _HeapNode    HeapNode;

typedef enum {
	MTypeHeap,
	MTypeSlice,
	MTypeSys,
} MemoryType;

struct _HeapNode {
	ePointer  addr;
	euint     size :31;
	bool      is_alloc :1;
	HeapNode *all_next;
	HeapNode *all_prev;
	HeapNode *free_next;
	HeapNode *free_prev;
	HeapNode *alloc_next;
	HeapNode *alloc_prev;
	HeapNode *cache_next;
};

struct _MemoryChain {
	MemoryType  type;
	ePointer    addr;
	euint       size;
	MemoryChain *next;
};

struct _MemoryHeap {
	MemoryChain m;
	euint       free_max;
	HeapNode   *all_head;
	HeapNode   *all_tail;
	HeapNode   *free_tables_head[FREE_TABLES_SIZE];				//8 16 32 64 128 256 512 1024 2048 4096
	HeapNode   *free_tables_tail[FREE_TABLES_SIZE];
	HeapNode   *alloc_tables[HEAP_SIZE / PAGE_SIZE];
};

static MemoryChain *chain_head = NULL;
static HeapNode    *cache_head = NULL;
static e_pthread_mutex_t memory_lock = PTHREAD_MUTEX_INITIALIZER;
static e_pthread_mutex_t slice_lock  = PTHREAD_MUTEX_INITIALIZER;

static ePointer e_sys_malloc(euint size);
static void free_node_merge(MemoryHeap *heap, HeapNode *node);
static void slice_free_release(bool);

static void cache_node_add(void)
{
	HeapNode *node;
	cache_head = __calloc(CACHE_NUM, sizeof(HeapNode));
	eint i;
	
	node = cache_head;
	for (i = 0; i < CACHE_NUM - 1; i++) {
		node->cache_next = (HeapNode *)(node + 1);
		node = node->cache_next;
	}
}

static HeapNode *cache_node_get(void)
{
	HeapNode *node;
	if (!cache_head)
		cache_node_add();
	node = cache_head;
	cache_head = node->cache_next;
	return node;
}

static void cache_node_free(HeapNode *node)
{
	node->cache_next = cache_head;
	cache_head = node;
}

static inline void heap_all_node_insert_after(MemoryHeap *heap, HeapNode *node, HeapNode *new)
{
	STRUCT_LIST_INSERT_AFTER(HeapNode, heap->all_head, heap->all_tail, node, new, all_prev, all_next);
}

static inline void heap_all_node_insert_before(MemoryHeap *heap, HeapNode *node, HeapNode *new)
{
	STRUCT_LIST_INSERT_BEFORE(HeapNode, heap->all_head, heap->all_tail, node, new, all_prev, all_next);
}

/*
static void heap_all_node_insert(MemoryHeap *heap, HeapNode *new)
{
	HeapNode *node = heap->all_head;
	while (node && new->addr > node->addr)
		node = node->all_next;
	heap_all_node_insert_before(heap, node, new);
}
*/

static void heap_all_node_delete(MemoryHeap *heap, HeapNode *node)
{
	STRUCT_LIST_DELETE(heap->all_head, heap->all_tail, node, all_prev, all_next);
}

static void heap_alloc_tables_insert(MemoryHeap *heap, HeapNode *node)
{
	euint   index = ((echar *)node->addr - (echar *)heap->m.addr) / PAGE_SIZE;
	HeapNode **pp = &heap->alloc_tables[index];

	while (*pp && node->addr > (*pp)->addr)
		pp = &(*pp)->alloc_next;

	if (pp == &heap->alloc_tables[index])
		node->alloc_prev = NULL;
	else
		node->alloc_prev = (HeapNode *)((euint)pp - STRUCT_OFFSET(HeapNode, alloc_next));

	if (*pp)
		(*pp)->alloc_prev = node;

	node->alloc_next = *pp;
	*pp = node;
	node->is_alloc = true;
}

static void move_to_free_tables(MemoryHeap *heap, HeapNode *node)
{
	if (node->alloc_next)
		node->alloc_next->alloc_prev = node->alloc_prev;
	if (node->alloc_prev)
		node->alloc_prev->alloc_next = node->alloc_next;
	else {
		euint index = ((echar *)node->addr - (echar *)heap->m.addr) / PAGE_SIZE;
		heap->alloc_tables[index] = node->alloc_next;;
	}
	node->is_alloc = false;

	free_node_merge(heap, node);
}

static HeapNode *alloc_tables_find_node(MemoryHeap *heap, euint index, ePointer addr)
{
	HeapNode *node = heap->alloc_tables[index];
	while (node && node->addr != addr)
		node = node->alloc_next;
	return node;
}

static euint size_to_free_index(MemoryHeap *heap, euint size)
{
	euint j = (size - 1) >> 2;
	euint i;
	for (i = 0; i < FREE_TABLES_SIZE - 1; i++) {
		j >>= 1;
		if (!j) break;
	}
	return i;
}

static void heap_free_tables_insert(MemoryHeap *heap, HeapNode *node)
{
	euint   index = size_to_free_index(heap, node->size);
	HeapNode **pp = heap->free_tables_head + index;

	while (*pp && node->size > (*pp)->size)
		pp = &(*pp)->free_next;

	if (pp == heap->free_tables_head + index)
		node->free_prev = NULL;
	else
		node->free_prev = (HeapNode *)((euint)pp - STRUCT_OFFSET(HeapNode, free_next));

	if (*pp)
		(*pp)->free_prev = node;
	else
		heap->free_tables_tail[index] = node;

	node->free_next = *pp;
	*pp = node;

	node->is_alloc = false;
	if (node->size > heap->free_max)
		heap->free_max = node->size;
}

static void set_free_max(MemoryHeap *heap, HeapNode *node)
{
	if (node->size != heap->free_max || node->free_next)
		return;

	if (!node->free_prev) {
		eint i = FREE_TABLES_SIZE - 1;
		for (; i >= 0; i--)
		   	if (heap->free_tables_tail[i]) {
				heap->free_max = heap->free_tables_tail[i]->size;
				return;
			}
		heap->free_max = 0;
	}
	else
		heap->free_max = node->free_prev->size;
}

static void heap_free_tables_delete(MemoryHeap *heap, HeapNode *node)
{
	euint index = size_to_free_index(heap, node->size);

	if (node->free_next)
		node->free_next->free_prev = node->free_prev;
	else
		heap->free_tables_tail[index] = node->free_prev;

	if (node->free_prev) {
		if (heap->free_tables_tail[index] == node->free_prev)
			node->free_prev->free_next = NULL;
		else
			node->free_prev->free_next = node->free_next;
	}
	else {
		heap->free_tables_head[index] = node->free_next;;
		if (node->free_next)
			node->free_next->free_prev = NULL;
	}

	set_free_max(heap, node);
}

static HeapNode *free_tables_find_node(MemoryHeap *heap, euint size)
{
	eint  index = size_to_free_index(heap, size);
	HeapNode *p = NULL;

	for (; index < FREE_TABLES_SIZE; index++) {
		p = heap->free_tables_head[index];
		while (p && p->size < size)
			p = p->free_next;
		if (p) break;
	}

	return p;
}

static void free_node_merge(MemoryHeap *heap, HeapNode *node)
{
	HeapNode *n = node->all_next;
	HeapNode *p = node->all_prev;
	if (n && !n->is_alloc) {
		heap_all_node_delete(heap, n);
		heap_free_tables_delete(heap, n);
		node->size += n->size;
		cache_node_free(n);
	}
	if (p && !p->is_alloc) {
		heap_all_node_delete(heap, p);
		heap_free_tables_delete(heap, p);
		node->addr  = p->addr;
		node->size += p->size;
		cache_node_free(p);
	}
	heap_free_tables_insert(heap, node);
}

static MemoryHeap *memory_heap_new(void)
{
	MemoryHeap *heap = __malloc(sizeof(MemoryHeap));
	HeapNode   *node = cache_node_get();

	node->addr      = __malloc(HEAP_SIZE);
	node->size      = HEAP_SIZE;
	node->is_alloc  = false;
	node->all_prev  = NULL;
	node->all_next  = NULL;

	heap->m.type    = MTypeHeap;
	heap->m.addr    = node->addr;
	heap->m.next    = NULL;
	heap->m.size    = HEAP_SIZE;
	heap->free_max  = HEAP_SIZE;
	heap->all_head  = node;
	heap->all_tail  = node;
	e_memset(heap->alloc_tables, 0, HEAP_SIZE / PAGE_SIZE * sizeof(HeapNode *));
	e_memset(heap->free_tables_head, 0, sizeof(HeapNode *) * FREE_TABLES_SIZE);
	e_memset(heap->free_tables_tail, 0, sizeof(HeapNode *) * FREE_TABLES_SIZE);

	heap_free_tables_insert(heap, node);

	return heap;
}

static bool e_memory_init(void)
{
	MemoryChain *chain = (MemoryChain *)memory_heap_new();
	chain->next = chain_head;
	chain_head  = chain;
	return true;
}

#define CHAIN_FIND_HEAP \
chain = chain_head; \
while (chain) { \
	if (chain->type == MTypeHeap \
			&& ((MemoryHeap *)chain)->free_max >= size) \
		break; \
	chain = chain->next; \
}
static HeapNode *heap_get_free_node(eint size, bool is_slice)
{
	MemoryChain *chain;
	MemoryHeap  *heap = NULL;
	HeapNode    *node = NULL;

	CHAIN_FIND_HEAP;
	if (chain)
		heap = (MemoryHeap *)chain;
	else {
		slice_free_release(is_slice);
		CHAIN_FIND_HEAP;
		if (chain)
			heap = (MemoryHeap *)chain;
		else {
			heap = memory_heap_new();
			heap->m.next = chain_head;
			chain_head   = &heap->m;
		}
	}

	node = free_tables_find_node(heap, size);
	if (node) {
		heap_free_tables_delete(heap, node);
		if (node->size > size) {
			HeapNode *new = cache_node_get();
			new->addr     = node->addr;
			new->size     = size;
			node->size   -= size;
			node->addr    = (echar *)node->addr + size;
			heap_all_node_insert_before(heap, node, new);
			heap_free_tables_insert(heap, node);
			node = new;
		}
		heap_alloc_tables_insert(heap, node);
	}

	return node;
}

ePointer e_heap_malloc(eint size, bool is_slice)
{
	HeapNode *node;
	ePointer  addr = NULL;

	e_pthread_mutex_lock(&memory_lock);
	if (!chain_head && !e_memory_init()) {
		e_pthread_mutex_unlock(&memory_lock);
		return NULL;
	}

	if (size > HEAP_ALLOC_MAX)
		addr = e_sys_malloc(size);
	else {
		node = heap_get_free_node(size, is_slice);
		if (node)
			addr = node->addr;
	}

	e_pthread_mutex_unlock(&memory_lock);
	return addr;
}

ePointer e_heap_realloc(ePointer addr, euint size)
{
	MemoryChain *chain = chain_head;
	HeapNode    *node  = NULL;
	euint        offset;

	if (!addr) return NULL;

	e_pthread_mutex_lock(&memory_lock);
	while (chain) {
		offset = (echar *)addr - (echar *)chain->addr;
		if (offset < chain->size)
			break;
		chain = chain->next;
	}
	if (!chain) abort();

	if (chain->type == MTypeSys) {
		chain->addr = __realloc(chain->addr, size);
		chain->size = size;
		addr = chain->addr;
	}
	else if (chain->type == MTypeHeap) {
		node = alloc_tables_find_node((MemoryHeap *)chain, offset / PAGE_SIZE, addr);
		if (node->size < size) {
			HeapNode *n = node->all_next;
			if (n && !n->is_alloc && node->size + n->size >= size) {
				eint lack  = size - node->size;
				heap_free_tables_delete((MemoryHeap *)chain, n);
				n->size   -= lack;
				n->addr    = (echar *)n->addr + lack;
				node->size = size;
				if (n->size > 0) {
					heap_free_tables_insert((MemoryHeap *)chain, n);
				}
				else {
					heap_all_node_delete((MemoryHeap *)chain, n);
					cache_node_free(n);
				}
			}
			else {
				HeapNode *t = heap_get_free_node(size, false);
				e_memcpy(t->addr, node->addr, node->size);
				move_to_free_tables((MemoryHeap *)chain, node);
				node = t;
			}
		}
		else if (size < node->size) {
			eint  over = node->size - size;
			node->size = size;
			if (node->all_next && !node->all_next->is_alloc) {
				HeapNode *n = node->all_next;
				euint index1 = size_to_free_index((MemoryHeap *)chain, n->size);
				euint index2 = size_to_free_index((MemoryHeap *)chain, n->size + over);
				if (index1 != index2) {
					heap_free_tables_delete((MemoryHeap *)chain, n);
					n->size += over;
					heap_free_tables_insert((MemoryHeap *)chain, n);
				}
				else {
					n->size += over;
					if (((MemoryHeap *)chain)->free_max < n->size)
						((MemoryHeap *)chain)->free_max = n->size;
				}
				n->addr  = (echar *)node->addr + size;
			}
			else {
				HeapNode *new = cache_node_get();
				new->addr     = (echar *)node->addr + size;
				new->size     = over;
				heap_free_tables_insert((MemoryHeap *)chain, new);
				heap_all_node_insert_after((MemoryHeap *)chain, node, new);
			}
		}
		addr = node->addr;
	}
	e_pthread_mutex_unlock(&memory_lock);

	return addr;
}

static void remove_sys_chain(MemoryChain *chain)
{
	MemoryChain **pp = &chain_head;
	while (*pp) {
		if (*pp == chain) {
			*pp = chain->next;
			break;
		}
		pp = &(*pp)->next;
	}
}

void e_heap_free(ePointer addr)
{
	MemoryChain *chain = chain_head;
	HeapNode    *node;
	euint        offset = 0;

	e_pthread_mutex_lock(&memory_lock);
	while (chain) {
		offset = (echar *)addr - (echar *)chain->addr;
		if (offset < chain->size)
			break;
		chain = chain->next;
	}

	if (chain) {
		if (chain->type == MTypeSys) {
			remove_sys_chain(chain);
			__free(chain);
		}
		else if (chain->type == MTypeHeap) {
			node = alloc_tables_find_node((MemoryHeap *)chain, offset / PAGE_SIZE, addr);
			move_to_free_tables((MemoryHeap *)chain, node);
		}
	}
	else abort();

	e_pthread_mutex_unlock(&memory_lock);
}

static ePointer e_sys_malloc(euint size)
{
	MemoryChain *chain = __malloc(sizeof(MemoryChain));
	chain->addr = __malloc(size);
	chain->type = MTypeSys;
	chain->size = size;
	chain->next = chain_head;
	chain_head  = chain;
	return chain->addr;
}

typedef struct _SliceNode SliceNode;
typedef struct _SliceHead SliceHead;

struct _SliceNode {
	euint16    index;
	euint16    free_num;
	euchar    *base;
	SliceNode *next;
	euchar     map[0];
};

struct _SliceHead {
	euint16    size;
	euint16    num;
	SliceNode *fill;
	SliceNode *half;
	SliceNode *free;
};

//level 0 1  2  3  4   5
//      8 16 32 64 128 256
static SliceHead *slice_tables[SLICE_TABLES_NUM];

static void slice_free_release(bool is_slice)
{
	SliceHead *head;
	eint i;

	if (!is_slice)
		e_pthread_mutex_lock(&slice_lock);

	for (i = 0; i < SLICE_TABLES_NUM; i++) {
		if (!slice_tables[i])
			continue;
		head = slice_tables[i];
		while (head->free && (head->half || head->free->next)) {
			SliceNode *n = head->free;
			head->free = n->next;
			e_free(n);
		}
	}

	if (!is_slice)
		e_pthread_mutex_unlock(&slice_lock);
}

static void half_add_tail(SliceHead *head, SliceNode *node)
{
	SliceNode **pp = &head->half;
	while (*pp)
		pp = &(*pp)->next;
	node->next = NULL;
	*pp = node;
}

static ePointer slice_half_alloc(SliceHead *head)
{
	SliceNode *node = head->half;
	ePointer   addr;

	if (!node) return NULL;

	addr = node->base + node->index * head->size;

	node->index = node->map[node->index];
	node->free_num--;
	if (node->free_num == 0) {
		head->half = node->next;
		node->next = head->fill;
		head->fill = node;
	}
	return addr;
}

static ePointer slice_free_alloc(SliceHead *head)
{
	SliceNode *node;
	ePointer   addr;

	if (head->free) {
		node = head->free;
		head->free = node->next;
		addr = node->base + node->index * head->size;
		node->index = node->map[node->index];
		node->free_num--;
	}
	else {
		node = e_heap_malloc(sizeof(SliceNode) + head->num + head->num * head->size, true);
		node->base  = (echar *)(node + 1) + head->num;
		node->index = 1;
		for (node->free_num = 1; node->free_num < head->num - 1; node->free_num++)
			node->map[node->free_num] = node->free_num + 1;
		addr = node->base;
	}
	half_add_tail(head, node);

	return addr;
}

static inline SliceHead *slice_head_new(euint type, euint level)
{
	SliceHead *head = e_heap_malloc(sizeof(SliceHead), true);
	head->num  = 8 << (level > 5 ? 5 : level);
	head->size = type * 8;
	head->fill = NULL;
	head->half = NULL;
	head->free = NULL;
	return head;
}

void e_slice_create(euint type, euint level)
{
	type = (type + 7) / 8;
	if (type > SLICE_TABLES_NUM || slice_tables[type])
		return;

	e_pthread_mutex_lock(&slice_lock);
	slice_tables[type] = slice_head_new(type, level);
	e_pthread_mutex_unlock(&slice_lock);
}

ePointer e_slice_alloc(euint size)
{
	SliceHead *head;
	ePointer   addr;

	euint type = (size + 7) / 8;
	if (type > SLICE_TABLES_NUM)
		return e_malloc(size);

	e_pthread_mutex_lock(&slice_lock);

	if (!slice_tables[type])
		slice_tables[type] = slice_head_new(type, 2);

	head = slice_tables[type];
	addr = slice_half_alloc(head);
	if (!addr)
		addr = slice_free_alloc(head);

	e_pthread_mutex_unlock(&slice_lock);
	return addr;
}

void e_slice_free1(euint size, ePointer addr)
{
	SliceHead *head;
	SliceNode *node;
	SliceNode **pp;
	euint      index;

	euint type = (size + 7) / 8;
	if (type > SLICE_TABLES_NUM) {
		e_heap_free(addr);
		return;
	}

	e_pthread_mutex_lock(&slice_lock);

	head = slice_tables[type];
	pp   = &head->fill;
	while (*pp) {
		index = ((euint)addr - (euint)(*pp)->base) / head->size;
		if (index < head->num)
			break;
		pp = &(*pp)->next;
	}
	node = *pp;
	if (node) {
		*pp = node->next;
		node->next = head->half;
		head->half = node;
		node->free_num++;
	}

	if (!node) {
		pp = &head->half;
		while (*pp) {
			index = ((euint)addr - (euint)(*pp)->base) / head->size;
			if (index < head->num)
				break;
			pp = &(*pp)->next;
		}
		if (!*pp) abort();

		node = *pp;
		node->free_num++;
		if (node->free_num == head->num) {
			*pp = node->next;
			node->next = head->free;
			head->free = node;
		}
		else if (node->next && node->free_num > node->next->free_num) {
			*pp = node->next;
			 pp = &(*pp)->next;
			while (*pp && node->free_num > (*pp)->free_num)
				pp = &(*pp)->next;
			node->next = *pp;
			*pp = node;
		}
	}

	node->map[index] = node->index;
	node->index = index;

	e_pthread_mutex_unlock(&slice_lock);
}
