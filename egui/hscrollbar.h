#ifndef __GUI_HSCROLLBAR_H
#define __GUI_HSCROLLBAR_H

#include <egui/scrollbar.h>

#define GTYPE_HSCROLLBAR						(egui_genetype_hscrollbar())


eGeneType egui_genetype_hscrollbar(void);
eHandle egui_hscrollbar_new(bool);

#endif
