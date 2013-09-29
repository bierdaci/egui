#ifndef __GUI_DDLIST_H
#define __GUI_DDLIST_H

#include <elib/object.h>

#define GTYPE_DDLIST				(egui_genetype_ddlist())

#define GUI_DDLIST_DATA(hobj)		((GuiDDList *)e_object_type_data(hobj, GTYPE_DDLIST))

typedef struct _GuiDDList GuiDDList;

struct _GuiDDList {
	bool enter;
	const echar *strings;
	eHandle menu;
	eHandle item;
	eHandle head, tail;
};

eGeneType egui_genetype_ddlist(void);
eHandle   egui_ddlist_new(void);

#endif
