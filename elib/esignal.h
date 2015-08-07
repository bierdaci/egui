#ifndef __ELIB_SIGNAL_H__
#define __ELIB_SIGNAL_H__

#include <elib/object.h>
#include <elib/etree.h>

typedef eulong				esig_t;
typedef struct _eSignal     eSignal;
typedef struct _eSignalSlot eSignalSlot;
typedef ePointer            eSignalFunc;
typedef eint (*eSignalCall)(eHandle, eSignalFunc, eValist);

typedef enum {
	STYPE_DEFAULT 			= 1,
	STYPE_CONNECT			= 2,
	STYPE_ANYONE			= 3,
} eSignalType;

typedef enum {
	ATYPE_OTHER				= 0,
	ATYPE_FLOAT				= 1,
} eArgsType;

struct _argsnode {
	eint type;
	eint size;
};

struct _eSignal {
	eGeneType   gtype;
	eSignalType stype;
	const echar *name;
	bool    prefix;
	euint32 offset;
	euint16 num;
	euint16 size;
	struct _argsnode *node;
};

struct _eSignalSlot {
	esig_t sig;
	bool lock;

	eSignalSlot *next;

	eVoidFunc func;
	ePointer  data;
	ePointer  data2;
	ePointer  data3;
	ePointer  data4;
};

extern int SIG_FREE;

esig_t e_signal_new(const char *, eGeneType, euint32, bool, eSignalType, const char *, ...);
esig_t e_signal_new_label(const char *, eGeneType, const char *, ...);
bool e_signal_connect (eHandle, esig_t, eSignalFunc);
bool e_signal_connect1(eHandle, esig_t, eSignalFunc, ePointer);
bool e_signal_connect2(eHandle, esig_t, eSignalFunc, ePointer, ePointer);
bool e_signal_connect3(eHandle, esig_t, eSignalFunc, ePointer, ePointer, ePointer);
bool e_signal_connect4(eHandle, esig_t, eSignalFunc, ePointer, ePointer, ePointer, ePointer);
eint e_signal_emit(eHandle, esig_t, ...);
eint e_signal_emit_default(eHandle, esig_t, ...);
eint e_signal_emit_connect(eHandle, esig_t, ...);
eint e_signal_emit_valist (eHandle, esig_t, eValist);
void e_signal_lock(eHandle, esig_t);
void e_signal_unlock(eHandle hobj, esig_t sig);
eSignalSlot *e_signal_get_slot (eHandle, esig_t);
eSignalFunc  e_signal_get_func (eHandle, esig_t);
ePointer     e_signal_get_data (eHandle, esig_t);
ePointer     e_signal_get_data2(eHandle, esig_t);
ePointer     e_signal_get_data3(eHandle, esig_t);
ePointer     e_signal_get_data4(eHandle, esig_t);

void e_signal_init(void);
#endif
