#ifndef __ELIB_SIGNAL_H__
#define __ELIB_SIGNAL_H__

#include <elib/object.h>
#include <elib/etree.h>

typedef struct _eSignal     eSignal;
typedef struct _eSignalSlot eSignalSlot;
typedef ePointer            eSignalFunc;
typedef eint (*eSignalCall)(eHandle, eSignalFunc, eValist);

typedef enum {
	STYPE_DEFAULT 			= 1,
	STYPE_CONNECT			= 2,
	STYPE_ANYONE			= 3,
} eSignalType;

struct _eSignal {
	eGeneType   gtype;
	eSignalType stype;
	const echar *name;
	bool    prefix;
	euint32 offset;
	euint32 size;
};

struct _eSignalSlot {
	eint sig;
	bool lock;

	eSignalSlot *next;

	eVoidFunc func;
	ePointer  data;
	ePointer  data2;
	ePointer  data3;
	ePointer  data4;
};

eint e_signal_new(const char *, eGeneType, euint32, bool, euint, eSignalType);
eint e_signal_connect_new(const char *, eGeneType, euint);
bool e_signal_connect (eHandle, eint, eSignalFunc);
bool e_signal_connect1(eHandle, eint, eSignalFunc, ePointer);
bool e_signal_connect2(eHandle, eint, eSignalFunc, ePointer, ePointer);
bool e_signal_connect3(eHandle, eint, eSignalFunc, ePointer, ePointer, ePointer);
bool e_signal_connect4(eHandle, eint, eSignalFunc, ePointer, ePointer, ePointer, ePointer);
eint e_signal_emit(eHandle, eint, ...);
eint e_signal_emit_default(eHandle, eint, ...);
eint e_signal_emit_connect(eHandle, eint, ...);
eint e_signal_emit_valist (eHandle, eint, eValist);
void e_signal_lock(eHandle, eint);
void e_signal_unlock(eHandle hobj, eint sig);
eSignalSlot *e_signal_get_slot (eHandle, eint);
eSignalFunc  e_signal_get_func (eHandle, eint);
ePointer     e_signal_get_data (eHandle, eint);
ePointer     e_signal_get_data2(eHandle, eint);
ePointer     e_signal_get_data3(eHandle, eint);
ePointer     e_signal_get_data4(eHandle, eint);

#endif
