#ifndef __ELIB_TIMER_H__
#define __ELIB_TIMER_H__

#include <elib/types.h>

typedef eHandle		eTimer;
typedef eint (*eTimerFunc)(eTimer, euint, ePointer);

void   e_timer_init(void);
void   e_timer_loop(void);
eTimer e_timer_add(euint interval, eTimerFunc func, ePointer args);
void   e_timer_del(eTimer timer);
#endif
