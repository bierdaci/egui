#ifndef __M3D_STACK_H
#define __M3D_STACK_H

typedef enum {
    STACK_LOWER = 0,
    STACK_UPPER = 1,
} estackpos_t;

typedef struct estack_s estack_t;

typedef struct {
	void *data;
	void (*reco)(estack_t *, void *);
	void (*redo)(estack_t *, void *);
	void (*free)(estack_t *, void *, estackpos_t);
} estacknode_t;

struct estack_s {
	int num;
	int max;
	int offset;
	estacknode_t nodes[1];
};

estack_t *e_stack_new(int max);

void  e_stack_push (estack_t *, void *data,
			void (*reco)(estack_t *, void *),
			void (*redo)(estack_t *, void *),
			void (*free)(estack_t *, void *, estackpos_t));

void  e_stack_pop(estack_t *);

void  e_stack_reco(estack_t *);
void  e_stack_redo(estack_t *);

void  e_stack_empty(estack_t *);
void  e_stack_empty_lower(estack_t *);
void  e_stack_empty_upper(estack_t *);

void *e_stack_get_data(estack_t *);

#endif
