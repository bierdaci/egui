#ifndef __GUI_ANIM_H
#define __GUI_ANIM_H

#include <elib/object.h>
#include <egal/pixbuf.h>

#define GTYPE_ANIM					(egui_genetype_anim())
#define GUI_ANIM_DATA(hobj)			((GuiAnimData *)e_object_type_data(hobj, GTYPE_ANIM))

typedef struct _GuiAnimData GuiAnimData;

struct _GuiAnimData {
	GalPB  pb;
	eTimer timer;
	euint  elapsed;
	GalDrawable drawable;
	GalImage       *image;
	GalPixbufAnim  *anim;
	GalPixbufFrame *frame;
};

eGeneType egui_genetype_anim(void);
eHandle   egui_anim_new(GalPixbufAnim *);

#endif
