#ifndef __GUI_FIXED_H
#define __GUI_FIXED_H

#include <egal/region.h>

#define GTYPE_FIXED						(egui_genetype_fixed())

eGeneType egui_genetype_fixed(void);
eHandle   egui_fixed_new(int w, int h);

#endif
