#ifndef __GAL_PIXMAP_GIF_H
#define __GAL_PIXMAP_GIF_H

#include <elib/types.h>
#include <egal/pixbuf.h>

typedef struct _eTimeVal                eTimeVal;
struct _eTimeVal {
	elong tv_sec;
	elong tv_usec;
};

typedef GalPixbufAnim                GalPixbufGifAnim;
typedef struct _GalPixbufGifAnimIter GalPixbufGifAnimIter;

struct _GalPixbufGifAnimIter {
	GalPixbufAnim    *gif_anim;
	eTimeVal          start_time;
	eTimeVal          current_time;
	eint              position;
	GalPixbufFrame   *current_frame;
	eint              first_loop_slowness;
};

void pixbuf_gif_anim_frame_composite(GalPixbufGifAnim *, GalPixbufFrame *);

#endif
