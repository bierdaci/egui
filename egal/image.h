#ifndef __GAL_IMAGE_H__
#define __GAL_IMAGE_H__

#include <egal/window.h>
#include <egal/pixbuf.h>

GalImage *egal_image_new_from_pixbuf(GalPixbuf *pixbuf);
GalImage *egal_image_new_from_file(const echar *filename);

void egal_copy_image(GalImage *dst,
		eint dx, eint dy, eint w, eint h,
		GalImage *src, eint sx, eint sy);
void egal_copy_image_from_pixbuf(GalImage *image, eint dx, eint dy,
		GalPixbuf *pixbuf, eint sx, eint sy, eint w, eint h);
void egal_fill_image(GalImage *, euint32, eint, eint, eint, eint);
void egal_image_copy_from_pixbuf(GalImage *, eint, eint, GalPixbuf *, eint, eint, eint, eint);

#endif
