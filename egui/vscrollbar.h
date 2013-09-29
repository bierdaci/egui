#ifndef __GUI_VSCROLLBAR_H
#define __GUI_VSCROLLBAR_H

#include <egui/scrollbar.h>

#define GTYPE_VSCROLLBAR						(egui_genetype_vscrollbar())


eGeneType egui_genetype_vscrollbar(void);
eHandle egui_vscrollbar_new(bool);

#endif
