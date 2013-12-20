#ifndef __GUI_EVENT_H__
#define __GUI_EVENT_H__

#include <egal/egal.h>

#define GTYPE_EVENT						(egui_genetype_event())

#define GUI_EVENT_DATA(hobj)			((GuiEventData *)e_object_type_data((hobj), GTYPE_EVENT))
#define GUI_EVENT_ORDERS(hobj)			((GuiEventOrders *)e_object_type_orders((hobj), GTYPE_EVENT))

#define SIG_KEYDOWN						event_signal_keydown
#define SIG_KEYUP						event_signal_keyup
#define SIG_MOUSEMOVE					event_signal_mousemove
#define SIG_LBUTTONDOWN					event_signal_lbuttondown
#define SIG_LBUTTONUP					event_signal_lbuttonup
#define SIG_RBUTTONDOWN					event_signal_rbuttondown
#define SIG_RBUTTONUP					event_signal_rbuttonup
#define SIG_MBUTTONDOWN					event_signal_mbuttondown
#define SIG_MBUTTONUP					event_signal_mbuttonup
#define SIG_FOCUS_IN					event_signal_focus_in
#define SIG_FOCUS_OUT					event_signal_focus_out
#define SIG_ENTER						event_signal_enter
#define SIG_LEAVE						event_signal_leave
#define SIG_CLICKED						event_signal_clicked
#define SIG_2CLICKED					event_signal_2clicked

typedef struct _GuiEventOrders		GuiEventOrders;

struct _GuiEventOrders {
	eint (*keydown)    (eHandle, GalEventKey *);
	eint (*keyup)      (eHandle, GalEventKey *);

	eint (*mousemove)  (eHandle, GalEventMouse *);

	eint (*lbuttondown)(eHandle, GalEventMouse *);
	eint (*lbuttonup)  (eHandle, GalEventMouse *);

	eint (*rbuttondown)(eHandle, GalEventMouse *);
	eint (*rbuttonup)  (eHandle, GalEventMouse *);

	eint (*mbuttondown)(eHandle, GalEventMouse *);
	eint (*mbuttonup)  (eHandle, GalEventMouse *);

	eint (*enter)      (eHandle, eint, eint);
	eint (*leave)      (eHandle);

	eint (*focus_in)   (eHandle);
	eint (*focus_out)  (eHandle);

	eint (*clicked)    (eHandle, ePointer);
	eint (*dbclicked)  (eHandle, ePointer);

};

extern esig_t event_signal_keydown;
extern esig_t event_signal_keyup;
extern esig_t event_signal_mousemove;
extern esig_t event_signal_lbuttondown;
extern esig_t event_signal_lbuttonup;
extern esig_t event_signal_rbuttondown;
extern esig_t event_signal_rbuttonup;
extern esig_t event_signal_mbuttondown;
extern esig_t event_signal_mbuttonup;
extern esig_t event_signal_focus_in;
extern esig_t event_signal_focus_out;
extern esig_t event_signal_enter;
extern esig_t event_signal_leave;
extern esig_t event_signal_clicked;
extern esig_t event_signal_2clicked;

eGeneType egui_genetype_event(void);

#endif
