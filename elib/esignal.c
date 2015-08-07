#include "memory.h"
#include "esignal.h"
#include "list.h"

static eTree *signal_tree = NULL;
#ifdef WIN32
static e_thread_mutex_t signal_lock = {0};
#else
static e_thread_mutex_t signal_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

eint __signal_call_marshal(eObject *, eVoidFunc, eValist, struct _argsnode *, eint, eint);
eint __signal_call_marshal_1(eObject *, eVoidFunc, ePointer, eValist, struct _argsnode *, eint, eint);

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

static bool e_signal_slot_insert_func(eObject *obj, esig_t sig, eSignalFunc func,
		ePointer data, ePointer data2, ePointer data3, ePointer data4)
{
	eSignalSlot *slot;

	e_thread_mutex_lock(&obj->slot_lock);

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
		e_thread_mutex_unlock(&obj->slot_lock);
		return false;
	}

	slot->func  = func;
	slot->data  = data;
	slot->data2 = data2;
	slot->data3 = data3;
	slot->data4 = data4;
	e_thread_mutex_unlock(&obj->slot_lock);

	return true;
}

static ePointer e_signal_slot_get_data(eObject *obj, esig_t sig, eint slot_type)
{
	eSignalSlot *slot;

	e_thread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig) {
			e_thread_mutex_unlock(&obj->slot_lock);
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

	e_thread_mutex_unlock(&obj->slot_lock);
	return NULL;
}

bool e_signal_connect(eHandle hobj, esig_t sig, eSignalFunc func)
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

bool e_signal_connect1(eHandle hobj, esig_t sig, eSignalFunc func, ePointer data)
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

bool e_signal_connect2(eHandle hobj, esig_t sig, eSignalFunc func, ePointer data, ePointer data2)
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

bool e_signal_connect3(eHandle hobj, esig_t sig, eSignalFunc func, ePointer data, ePointer data2, ePointer data3)
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

bool e_signal_connect4(eHandle hobj, esig_t sig, eSignalFunc func,
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

static eint fmstr2node(const char *fmstr, struct _argsnode *node)
{
	const char *p = fmstr;
	eint n = 0;

	if (!p) return 0;

	while (*p) {
		if (*p != '%')
			return -1;
		p++;
		node[n].type = ATYPE_OTHER;
		if (p[0] == 'l') {
			p++;
			if (p[0] == 'l') {
				p++;
				node[n].size = sizeof(ellong);
				if (p[0] != 'd' && p[0] != 'u')
					return -1;
			}
			else if (p[0] == 'f') {
				node[n].type = ATYPE_FLOAT;
				node[n].size = sizeof(edouble);
			}
			else if (p[0] == 'd' || p[0] == 'u') {
				node[n].size = sizeof(elong);
			}
			else
				return -1;
		}
		else if (p[0] == 'f') {
			node[n].type = ATYPE_FLOAT;
			node[n].size = sizeof(efloat);
		}
		else if (p[0] == 'p') {
			node[n].size = sizeof(ePointer);
		}
		else if (p[0] == 'n' || p[0] == 'h') {
			node[n].size = sizeof(eHandle);
		}
		else if (p[0] == 'd' || p[0] == 'u') {
			node[n].size = sizeof(eint);
		}
		else
			return -1;

		p++;
		while (*p == ' ') p++;
		n++; 
	}
	return n;
}

static esig_t __signal_new(const char *name, eGeneType gtype,
		euint32 offset, bool prefix, eSignalType stype, const char *fmstr, eValist vp)
{
	eSignal *new;
	struct _argsnode tmp[256];

	e_thread_mutex_lock(&signal_lock);

	if (e_signal_check_name((const echar *)name)) {
		e_thread_mutex_unlock(&signal_lock);
		return 0;
	}

	new = e_malloc(sizeof(eSignal));
	new->name    = (const echar *)name;
	new->prefix  = prefix;
	new->gtype   = gtype;
	new->offset  = offset;
	new->size = 0;
	new->num  = fmstr2node(fmstr, tmp);
	if (new->num < 0)
		return -1;
	if (new->num > 0) {
		eint i;
		new->node = e_malloc(sizeof(struct _argsnode) * new->num);
		for (i = 0; i < new->num; i++) {
			new->node[i] = tmp[i];
			new->size   += ALIGN_WORD(tmp[i].size);
		}
	}
	if (stype < STYPE_DEFAULT || stype > STYPE_ANYONE)
		new->stype = STYPE_ANYONE;
	else
		new->stype = stype;

	e_signal_register(new);

	e_thread_mutex_unlock(&signal_lock);

	return (esig_t)new;
}

esig_t e_signal_new(const char *name, eGeneType gtype,
		euint32 offset, bool prefix, eSignalType stype, const char *fmstr, ...)
{
	if (fmstr) {
		eValist vp;
		e_va_start(vp, fmstr);
		return __signal_new(name, gtype, offset, prefix, stype, fmstr, vp);
	}
	return __signal_new(name, gtype, offset, prefix, stype, fmstr, 0);
}

esig_t e_signal_new_label(const char *name, eGeneType gtype, const char *fmstr, ...)
{
	if (fmstr) {
		eValist vp;
		e_va_start(vp, fmstr);
		return __signal_new(name, gtype, 0, false, STYPE_CONNECT, fmstr, vp);
	}
	return __signal_new(name, gtype, 0, false, STYPE_CONNECT, fmstr, 0);
}

static eint signal_call_marshal(eObject *obj, eSignal *signal, eVoidFunc func, eValist vp)
{
	return __signal_call_marshal(obj, func, vp, signal->node, signal->num, signal->size);
}

static eint signal_call_marshal_1(eObject *obj, eSignal *signal, eVoidFunc func, ePointer *data, eValist vp)
{
	return __signal_call_marshal_1(obj, func, data, vp, signal->node, signal->num, signal->size);
}

static eint e_signal_call(eHandle hobj, eSignal *signal, eSignalType type, eValist vp)
{
	eObject   *obj  = (eObject *)hobj;
	eDnaNode  *node = e_genetype_node(obj->gene, signal->gtype);
	eVoidFunc *func = NULL;

	if (!node)
		return 0;

	if ((type & STYPE_CONNECT) && (signal->stype & STYPE_CONNECT))
		func = e_signal_slot_get_data(obj, (esig_t)signal, SLOT_FUNC);

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

eint e_signal_emit_valist(eHandle hobj, esig_t sig, eValist vp)
{
	return e_signal_call(hobj, (eSignal *)sig, STYPE_ANYONE, vp);
}

eint e_signal_emit(eHandle hobj, esig_t sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_ANYONE, vp);
	e_va_end(vp);

	return retval;
}

eint e_signal_emit_default(eHandle hobj, esig_t sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_DEFAULT, vp);
	e_va_end(vp);

	return retval;
}

eint e_signal_emit_connect(eHandle hobj, esig_t sig, ...)
{
	eValist vp;
	eint    retval;

	e_va_start(vp, sig);
	retval = e_signal_call(hobj, (eSignal *)sig, STYPE_CONNECT, vp);
	e_va_end(vp);

	return retval;
}

eSignalSlot *e_signal_get_slot(eHandle hobj , esig_t sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT);
}

ePointer e_signal_get_data(eHandle hobj, esig_t sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA);
}

ePointer e_signal_get_data2(eHandle hobj, esig_t sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA2);
}

ePointer e_signal_get_data3(eHandle hobj, esig_t sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA3);
}

ePointer e_signal_get_data4(eHandle hobj, esig_t sig)
{
	return e_signal_slot_get_data((eObject *)hobj, sig, SLOT_DATA4);
}

eSignalFunc e_signal_get_func(eHandle hobj, esig_t sig)
{
	eVoidFunc *func = e_signal_slot_get_data((eObject *)hobj, sig, SLOT_FUNC);
	if (func) return *func;
	return NULL;
}

void e_signal_lock(eHandle hobj, esig_t sig)
{
	eObject *obj = (eObject *)hobj;
	eSignalSlot *slot;

	e_thread_mutex_lock(&obj->slot_lock);

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

	e_thread_mutex_unlock(&obj->slot_lock);
}

void e_signal_unlock(eHandle hobj, esig_t sig)
{
	eObject *obj = (eObject *)hobj;
	eSignalSlot *slot;

	e_thread_mutex_lock(&obj->slot_lock);

	slot = (eSignalSlot *)obj->slot_head;
	while (slot) {
		if (slot->sig == sig) {
			slot->lock = false;
			break;
		}
		slot = slot->next;
	}

	e_thread_mutex_unlock(&obj->slot_lock);
}

void e_signal_init(void)
{
#ifdef WIN32
	e_thread_mutex_init(&signal_lock, NULL);
#endif
	SIG_FREE = e_signal_new("free",
			GTYPE_CELL,
			STRUCT_OFFSET(eCellOrders, free),
			false, 0, NULL);
}
