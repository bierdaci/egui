#ifndef __GUI_SPINBTN_H
#define __GUI_SPINBTN_H

#include <elib/object.h>

#define GTYPE_SPINBTN				(egui_genetype_spinbtn())

#define GUI_SPINBTN_DATA(hobj)		((GuiSpinBtn *)e_object_type_data(hobj, GTYPE_SPINBTN))

typedef struct _GuiSpinBtn GuiSpinBtn;

struct _GuiSpinBtn {
	eHandle entry;
	eHandle vbox;
	eHandle bn1, bn2;

	efloat value;
	efloat min, max;
	efloat step_inc;
	efloat page_inc;
	eint   digits;

	eint   time;
	eTimer timer;
};

eGeneType egui_genetype_spinbtn(void);
eHandle egui_spinbtn_new(efloat, efloat, efloat, efloat, efloat, eint);

#endif
