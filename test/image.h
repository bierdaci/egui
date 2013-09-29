#ifndef __GUI_IMAGE_H
#define __GUI_IMAGE_H

#include <elib/object.h>

typedef struct _GuiImage GuiImage;

struct _GuiImage {
	GalImage *img;
};

#define GTYPE_IMAGE					(egui_genetype_image())
#define GUI_IMAGE_DATA(hobj)		((GuiImage *)e_object_type_data(hobj, GTYPE_IMAGE))

eHandle egui_image_new(const echar *filename);
#endif
