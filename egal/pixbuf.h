#ifndef __GAL_PIXBUF_H
#define __GAL_PIXBUF_H

#include <egal/pixops.h>

typedef struct _GalPixbuf			GalPixbuf;
typedef struct _GalPixbufAnim		GalPixbufAnim;
typedef struct _GalPixbufFrame		GalPixbufFrame;

typedef enum {
	GAL_PIXBUF_FRAME_RETAIN,
	GAL_PIXBUF_FRAME_DISPOSE,
	GAL_PIXBUF_FRAME_REVERT
} GalPixbufFrameAction;

struct _GalPixbuf {
	int w, h;
	int rowbytes;
	int pixelbytes;
	bool alpha;
	euchar *pixels;
};

struct _GalPixbufFrame {
	GalPixbuf *pixbuf;

	int x_offset;
	int y_offset;
	int delay_time;
	int elapsed;

	GalPixbufFrameAction action;

	bool need_recomposite;
	bool bg_transparent;

	GalPixbuf *composited;
	GalPixbuf *revert;

	GalPixbufFrame *prev;
	GalPixbufFrame *next;
};

struct _GalPixbufAnim {
	int n_frames;
	int total_time;

	GalPixbufFrame *frames;
	GalPixbufFrame *frames_tail;

	int width, height;

	euchar bg_red;
	euchar bg_green;
	euchar bg_blue;

	int  loop;
	bool loading;
};

GalPixbuf *egal_pixbuf_new(bool alpha, eint w, eint h);
void       egal_pixbuf_free(GalPixbuf *);
GalPixbuf *egal_pixbuf_copy(GalPixbuf *);
GalPixbuf *egal_pixbuf_new_from_file(const echar *filename, efloat, efloat);
GalPixbuf *egal_pixbuf_new_subpixbuf(GalPixbuf *, eint xoffset, eint yoffset, eint width, eint height);
GalPixbufAnim *egal_pixbuf_anim_new_from_file(const echar *filename, efloat x_scale, efloat y_scale);
bool egal_pixbuf_load_header(const echar *filename, GalPixbuf *pixbuf);

euint8 pixbuf_read_pixel8(GalPixbuf *pixbuf, eint x, eint y);
euint16 pixbuf_read_pixel16(GalPixbuf *pixbuf, eint x, eint y);
euint32 pixbuf_read_pixel32(GalPixbuf *pixbuf, eint x, eint y);
void egal_pixbuf_copy_area(GalPixbuf *dst, eint sx, eint sy, eint w, eint h, GalPixbuf *src, eint dx, eint dy);

void egal_pixbuf_composite(GalPixbuf *dst,
		eint dst_x, eint dst_y, eint dst_w, eint dst_h,
		GalPixbuf *src,
		eint offset_x, eint offset_y,
		edouble scale_x, edouble scale_y,
		PixopsInterpType  interp_type);
void egal_pixbuf_fill(GalPixbuf *pixbuf, euint32 color);

#endif
