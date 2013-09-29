#ifndef __GUI_SCROLLWIN_H
#define __GUI_SCROLLWIN_H

#include <elib/object.h>

#define GTYPE_SCROLLWIN					(egui_genetype_scrollwin())

#define GUI_SCROLLWIN_DATA(hobj)			((GuiScrollWin       *)e_object_type_data(hobj, GTYPE_SCROLLWIN))

typedef struct _GuiScrollWin GuiScrollWin;

struct _GuiScrollWin {
	eint offset_x;
	eint offset_y;
	eHandle vadj;
	eHandle hadj;
};

eGeneType egui_genetype_scrollwin(void);
eHandle   egui_scrollwin_new(void);

#endif
