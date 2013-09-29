#include "memory.h"
#include "esignal.h"
#include "list.h"

static eTree *signal_tree = NULL;
static e_pthread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;

eint __signal_call_marshal(eObject *, eVoidFunc, eValist, eint);
eint __signal_call_marshal_1(eObject *, eVoidFunc, ePointer, eValist, eint);

#define SLOT			0
#define SLOT_FUNC		1
#define SLOT_DATA		2
#define SLOT_DATA2		3
#define SLOT_DATA3		4
#define SLOT_DATA4		5

static eint e_signal_compare_func(eConstPointer a, eConstPointer b)
{
	const echar *n1 = (const echar *)a;
	const echar *n2 = (const echar *)b;
	eint i = 0;

	while (n1[i] && n2[i]) {
		if (n2[i] > n1[i])
			return 1;
		else if (n2[i] < n1[i])
			return -1;
		else
			i++;
	}

	if (n1[i] && !n2[i])
		return -1;
	else if (!n1[i] && n2[i])
		return 1;
	else
		return 0;
}

static void e_signal_register(eSignal *signal)
{
	if (!signal_tree)
		signal_tree = e_tree_new(e_signal_compare_func);

	e_tree_insert(signal_tree, (ePointer)signal->name, (ePointer)signal);
}

/*
static eSignal *e_signal_find(const echar *name)
{
	return (eSignal *)e_tree_lookup(signal_tree, (eConstPointer)name);
}
*/

static bool e_signal_check_name(const echar *name)
{
	if (e_tree_lookup(signal_tree, (eConstPointer)name))
		return true;

	return false;
}

static bool e_signal_slot_insert_func(eObject *obj, eint sig, eSignalFunc func,
		ePointer data, ePointer data2, ePointer data3, ePointer data4)
{
	eSignalSlot *slot;

	e_pthread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig)
			break;
		slot = slot->next;
	}

	if (!slot) {
		slot = e_slice_new(eSignalSlot);
		slot->sig  = sig;
		slot->next = NULL;
		if (!obj->slot_head) {
			obj->slot_head = (ePointer)slot;
			obj->slot_tail = (ePointer)slot;
		}
		else {
			eSignalSlot *tail = (eSignalSlot *)obj->slot_tail;
			tail->next = slot;
			obj->slot_tail = (ePointer)slot;
		}
	}
	else if (slot->lock) {
		e_pthread_mutex_unlock(&obj->slot_lock);
		return false;
	}

	slot->func  = func;
	slot->data  = data;
	slot->data2 = data2;
	slot->data3 = data3;
	slot->data4 = data4;
	e_pthread_mutex_unlock(&obj->slot_lock);

	return true;
}

static ePointer e_signal_slot_get_data(eObject *obj, eint sig, eint slot_type)
{
	eSignalSlot *slot;

	e_pthread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig) {
			e_pthread_mutex_unlock(&obj->slot_lock);
			if (slot_type == SLOT)
				return slot;
			else if (slot_type == SLOT_FUNC)
				return &slot->func;
			else if (slot_type == SLOT_DATA)
				return slot->data;
			else if (slot_type == SLOT_DATA2)
				return slot->data2;
			else if (slot_type == SLOT_DATA3)
				return slot->data3;

			return slot->data4;
		}
		slot = slot->next;
	}

	e_pthread_mutex_unlock(&obj->slot_lock);
	return NULL;
}

bool e_signal_connect(eHandle hobj, eint sig, eSignalFunc func)
{
	eObject  *obj    = (eObject *)hobj;
	eSignal  *signal = (eSignal *)sig;
	eDnaNode *node;

	node = e_genetype_node(obj->gene, signal->gtype);
	if (!node)
		return false;

	if (!e_signal_slot_insert_func(obj, sig, func, NULL, NULL, NULL, NULL))
		return false;

	return true;
}

bool e_signal_connect1(eHandle hobj, eint sig, eSignalFunc func, ePointer data)
{
	eObject  *obj    = (eObject *)hobj;
	eSignal  *signal = (eSignal *)sig;
	eDnaNode *node;

	node = e_genetype_node(obj->gene, signal->gtype);
	if (!node)
		return false;

	if (!e_signal_slot_insert_func(obj, sig, func, data, NULL, NULL, NULL))
		return false;

	return true;
}

bool e_signal_connect2(eHandle hobj, eint sig, eSignalFunc func, ePointer data, ePointer data2)
{
	eObject  *obj    = (eObject *)hobj;
	eSignal  *signal = (eSignal *)sig;
	eDnaNode *node;

	node = e_genetype_node(obj->gene, signal->gtype);
	if (!node)
		return false;

	if (!e_signal_slot_insert_func(obj, sig, func, data, data2, NULL, NULL))
		return false;

	return true;
}

bool e_signal_connect3(eHandle hobj, eint sig, eSignalFunc func, ePointer data, ePointer data2, ePointer data3)
{
	eObject  *obj    = (eObject *)hobj;
	eSignal  *signal = (eSignal *)sig;
	eDnaNode *node;

	node = e_genetype_node(obj->gene, signal->gtype);
	if (!node)
		return false;

	if (!e_signal_slot_insert_func(obj, sig, func, data, data2, data3, NULL))
		return false;

	return true;
}

bool e_signal_connect4(eHandle hobj, eint sig, eSignalFunc func,
		ePointer data, ePointer data2, ePointer data3, ePointer data4)
{
	eObject  *obj    = (eObject *)hobj;
	eSignal  *signal = (eSignal *)sig;
	eDnaNode *node;

	node = e_genetype_node(obj->gene, signal->gtype);
	if (!node)
		return false;

	if (!e_signal_slot_insert_func(obj, sig, func, data, data2, data3, data4))
		return false;

	return true;
}

eint e_signal_new(const char *name, eGeneType gtype,
		euint32 offset, bool prefix, euint va_size, eSignalType stype)
{
	eSignal *new;

	e_pthread_mutex_lock(&signal_lock);

	if (e_signal_check_name((const echar *)name)) {
		e_pthread_mutex_unlock(&signal_lock);
		return 0;
	}

	new = e_malloc(sizeof(eSignal));
	new->name    = (const echar *)name;
	new->prefix  = prefix;
	new->gtype   = gtype;
	new->offset  = offset;
	new->size    = (va_size + sizeof(eulong) - 1) & ~(sizeof(euint) - 1);
	if (stype < STYPE_DEFAULT || stype > STYPE_ANYONE)
		new->stype = STYPE_ANYONE;
	else
		new->stype = stype;

	e_signal_register(new);

	e_pthread_mutex_unlock(&signal_lock);

	return (eint)new;
}

eint e_signal_connect_new(const char *name, eGeneType gtype, euint va_size)
{
	return e_signal_new(name, gtype, 0, false, va_size, STYPE_CONNECT);
}

static eint signal_call_marshal(eObject *obj, eSignal *signal, eVoidFunc func, eValist vp)
{
	return __signal_call_marshal(obj, func, vp, signal->size);
}

static eint signal_call_marshal_1(eObject *obj, eSignal *signal, eVoidFunc func, ePointer *data, eValist vp)
{
	return __signal_call_marshal_1(obj, func, data, vp, signal->size);
}

static eint e_signal_call(eHandle hobj, eSignal *signal, eSignalType type, eValist vp)
{
	eObject   *obj  = (eObject *)hobj;
	eDnaNode  *node = e_genetype_node(obj->gene, signal->gtype);
	eVoidFunc *func = NULL;

	if (!node)
		return 0;

	if ((type & STYPE_CONNECT) && (signal->stype & STYPE_CONNECT))
		func = e_signal_slot_get_data(obj, (eint)signal, SLOT_FUNC);

	if ((!func || !*func)
			&& (type & STYPE_DEFAULT)
			&& (signal->stype & STYPE_DEFAULT)) {
		func = (eVoidFunc *)(obj->gene->orders_base + node->info.orders_offset + signal->offset);
	}

	if (func && *func) {
		if (signal->prefix) {
			ePointer data;
			if (node->info.object_size > 0)
				data = (echar *)obj + node->info.object_offset;
			else
				data = NULL;
			return signal_call_marshal_1(obj, signal, *func, data, vp);
		}
		else
			return signal_call_marshal(obj, signal, *func, vp);
	}

	return 0;
}

eint e_signal_emit_valist(eHandle hobj, eint sig, eValist vp)
{
	return e_signal_call(hobj, (eSignal *)sig, STYPE_ANYONE, vp);
}

eint e_signal_emit(eHandle hobj, eint sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_ANYONE, vp);
	e_va_end(vp);

	return retval;
}

eint e_signal_emit_default(eHandle hobj, eint sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_DEFAULT, vp);
	e_va_end(vp);

	return retval;
}

eint e_signal_emit_connect(eHandle hobj, eint sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_CONNECT, vp);
	e_va_end(vp);

	return retval;
}

eSignalSlot *e_signal_get_slot(eHandle hobj , eint sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT);
}

ePointer e_signal_get_data(eHandle hobj, eint sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA);
}

ePointer e_signal_get_data2(eHandle hobj, eint sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA2);
}

ePointer e_signal_get_data3(eHandle hobj, eint sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA3);
}

ePointer e_signal_get_data4(eHandle hobj, eint sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA4);
}

eSignalFunc e_signal_get_func(eHandle hobj, eint sig)
{
	eVoidFunc *func = e_signal_slot_get_data((eObject *)hobj, sig, SLOT_FUNC);
	if (func) return *func;
	return NULL;
}

void e_signal_lock(eHandle hobj, eint sig)
{
	eObject *obj = (eObject *)hobj;
	eSignalSlot *slot;

	e_pthread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig)
			break;
		slot = slot->next;
	}

	if (!slot) {
		slot = e_slice_new(eSignalSlot);
		slot->next = NULL;
		if (!obj->slot_head) {
			obj->slot_head = (ePointer)slot;
			obj->slot_tail = (ePointer)slot;
		}
		else {
			eSignalSlot *tail = (eSignalSlot *)obj->slot_tail;
			tail->next = slot;
			obj->slot_tail = (ePointer)slot;
		}
		slot->sig  = sig;
		slot->func = NULL;
		slot->data = NULL;
	}
	slot->lock = true;

	e_pthread_mutex_unlock(&obj->slot_lock);
}

void e_signal_unlock(eHandle hobj, eint sig)
{
	eObject *obj = (eObject *)hobj;
	eSignalSlot *slot;

	e_pthread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig) {
			slot->lock = false;
			break;
		}
		slot = slot->next;
	}

	e_pthread_mutex_unlock(&obj->slot_lock);
}
