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

#define EVENT_QUEUE_MAX		20

eint egal_init(eint, char *const[]);
ebool egal_wait_event(GalEvent *);

eint egal_event_init(void);
void egal_add_event_to_queue(GalEvent *);
eint egal_get_event_from_queue(GalEvent *);
void egal_add_async_event_to_queue(GalEvent *);

#endif
