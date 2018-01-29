#ifndef __EGAL_H__
#define __EGAL_H__

#include <elib/elib.h>
#include <egal/types.h>
#include <egal/rect.h>
#include <egal/region.h>
#include <egal/window.h>
#include <egal/image.h>
#include <egal/font.h>
#include <egal/types.h>

#define EVENT_QUEUE_MAX		100

eint egal_init(eint, char *const[]);

eint egal_event_init(void);
eint egal_size_event(void);
eint egal_wait_event(Queue *);
void egal_add_event(GalEvent *);
eint egal_get_event(GalEvent *);
void egal_add_async_event(GalEvent *);

#endif
