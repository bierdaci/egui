#include "std.h"
#include "stack.h"

static void stack_free(estack_t *stack, estacknode_t *node, estackpos_t pos)
{
	if (node->data && node->free)
		node->free(stack, node->data, pos);
}

static void stack_reco(estack_t *stack, estacknode_t *node)
{
	if (node->data)
		node->reco(stack, node->data);
}

static void stack_redo(estack_t *stack, estacknode_t *node)
{
	if (node->data)
		node->redo(stack, node->data);
}

static void stack_push(estack_t *stack)
{
	int i;
	estacknode_t *node;

	stack->offset++;
	if (stack->offset == stack->num && stack->num == stack->max) {
		stack_free(stack, stack->nodes, STACK_LOWER);
		for (i = 0; i < stack->max - 1; i++) {
			stack->nodes[i] = stack->nodes[i + 1];
		}
		stack->offset--;
		stack->num--;
	}
	node = stack->nodes + stack->offset;

	if (stack->offset < stack->num) {
		for (i = stack->offset; i < stack->num; i++)
			stack_free(stack, stack->nodes + i, STACK_UPPER);
	}
	stack->num = stack->offset + 1;

	memset(node, 0, sizeof(estacknode_t));
}

estack_t *e_stack_new(int max)
{
	estack_t *stack;

	if (max < 2)
		return NULL;

	stack = e_calloc(sizeof(estack_t) + sizeof(estacknode_t) * (max - 1), 1);
	stack->max = max;
	stack->num = 1;
	stack->offset = 0;

	return stack;
}

void e_stack_push(estack_t *stack, void *data,
		void (*reco)(estack_t *, void *),
		void (*redo)(estack_t *, void *),
		void (*free)(estack_t *, void *, estackpos_t))
{
	estacknode_t *node;

	stack_push(stack);
	node = stack->nodes + stack->offset;
	node->free = free;
	node->reco = reco;
	node->redo = redo;

	node->data = data;
}

void e_stack_pop(estack_t *stack)
{
	if (stack->offset > 0) {
		stack_free(stack, stack->nodes + stack->offset, STACK_UPPER);
		stack->offset--;
		stack->num = stack->offset + 1;
	}
}

void *e_stack_get_data(estack_t *stack)
{
	return stack->nodes[stack->offset].data;
}

void e_stack_empty(estack_t *stack)
{
	int i;

	for (i = 0; i < stack->offset; i++)
		stack_free(stack, stack->nodes + i, STACK_LOWER);
	for (i = stack->num - 1; i > stack->offset; i--)
		stack_free(stack, stack->nodes + i, STACK_UPPER);

	stack->nodes[0] = stack->nodes[stack->offset];
	stack->offset = 0;
	stack->num = 1;
}

void e_stack_empty_lower(estack_t *stack)
{
	int i;
	for (i = 0; i < stack->offset; i++)
		stack_free(stack, stack->nodes + i, STACK_LOWER);
	stack->num -= stack->offset;
	memcpy(stack->nodes,
			stack->nodes + stack->offset,
			sizeof(estacknode_t) * stack->num);
	stack->offset = 0;
}

void e_stack_empty_upper(estack_t *stack)
{
	int i;
	for (i = stack->num - 1; i > stack->offset; i--)
		stack_free(stack, stack->nodes + i, STACK_UPPER);
	stack->num = stack->offset + 1;
}

void e_stack_reco(estack_t *stack)
{
	estacknode_t *node;

	if (stack->offset == 0)
		return;

	node = stack->nodes + stack->offset--;
	stack_reco(stack, node);
}

void e_stack_redo(estack_t *stack)
{
	estacknode_t *node;

	if (stack->offset + 1 == stack->num)
		return;

	node = stack->nodes + ++stack->offset;

	stack_redo(stack, node);
}

