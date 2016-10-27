#ifndef __GAL_PIXMAP_IO_H
#define __GAL_PIXMAP_IO_H

#include <egal/pixbuf.h>

#define SCALE_SHIFT				16
#define SCALE_MASK				0xffff

typedef struct _GalPixbufModule GalPixbufModule;
typedef struct _PixbufContext   PixbufContext;
typedef void (*GalPixbufModuleUpdatedFunc)(GalPixbuf *, eint, eint, eint, eint, ePointer);

struct _GalPixbufModule {
	const char *name;
	ebool        fill;

	void       (*fill_vtable)(GalPixbufModule *);
	ePointer   (*load_begin)(void);
	GalPixbuf* (*load_end)(ePointer);
	ebool       (*load_increment)(ePointer, const euchar *, size_t);
	ebool       (*load_header)(GalPixbuf *, ePointer, euint);
	ePointer   (*anim_load_begin)(void);
	GalPixbufAnim* (*anim_load_end)(ePointer);
	//ebool       (*load_animation)(void);
	//save;
	//save_to_callback;
};

struct _PixbufContext {
	eint y;
	eint w, h;
	eint negative;

	euint32 rowbytes;
	euint32 shift_w, shift_h;
	efloat  x_scale, y_scale;
	euint32 ny;
	euint32 dx, dy;
	euint32 x_step, y_step;
	euint32 over_y, quota_y;

	void (*set_pixel)(euchar *, euint8, euint8, euint8, euint8);
	ebool (*set_y)(PixbufContext *, eint);
	void (*set_x)(PixbufContext *, eint, euint8, euint8, euint8, euint8);
	eint (*init )(PixbufContext *, eint, eint, eint, eint);

	euchar    *pixels;
	GalPixbuf *pixbuf;
};

#endif
