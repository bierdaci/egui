#ifndef __GUI_BUTTON_H
#define __GUI_BUTTON_H

#include <elib/object.h>

#define GTYPE_BUTTON					(egui_genetype_button())
#define GTYPE_LABEL_BUTTON				(egui_genetype_label_button())
#define GTYPE_RADIO_BUTTON				(egui_genetype_radio_button())
#define GTYPE_CHECK_BUTTON				(egui_genetype_check_button())

#define GUI_BUTTON_DATA(hobj)			((GuiButton *)e_object_type_data(hobj, GTYPE_BUTTON))

typedef struct _GuiButton  GuiButton;
typedef struct _RadioGroup RadioGroup;

struct _GuiButton {
	bool enter;
	bool down;
	bool key_down;
	const echar *label;
	union {
		bool check;
		RadioGroup *group;
	} p;
};

struct _RadioGroup {
	eHandle bn;
};

eGeneType egui_genetype_button(void);
eGeneType egui_genetype_label_button(void);
eGeneType egui_genetype_radio_button(void);

eHandle   egui_button_new(int w, int h);
eHandle   egui_label_button_new(const echar *);
eHandle   egui_radio_button_new(const echar *, eHandle);

#endif
