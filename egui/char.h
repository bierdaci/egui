#ifndef __GUI_CHAR_H__
#define __GUI_CHAR_H__

#include <elib/object.h>

#define GTYPE_CHAR		(egui_genetype_char())

#define SIG_CHAR		char_signal_char

typedef struct _GuiCharOrders GuiCharOrders;

struct _GuiCharOrders {
	eint (*achar)(eHandle, echar);
};

extern esig_t char_signal_char;

eGeneType egui_genetype_char(void);
eHandle egui_char_new(int w, int h);

#endif
