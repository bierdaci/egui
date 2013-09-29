#include <stdlib.h>

#include "egal.h"
#include "pixbuf.h"

void egal_pixbuf_free(GalPixbuf *pixbuf)
{
	if (pixbuf->pixels)
		e_free(pixbuf->pixels);
	e_slice_free(GalPixbuf, pixbuf);
}

GalPixbuf *egal_pixbuf_new(bool alpha, eint w, eint h)
{
	GalPixbuf *pixbuf = e_slice_new(GalPixbuf);

	pixbuf->alpha = alpha;
	pixbuf->w = w;
	pixbuf->h = h;

	if (pixbuf->alpha)
		pixbuf->pixelbytes = 4;
	else
		pixbuf->pixelbytes = 3;
	pixbuf->rowbytes = (pixbuf->w * pixbuf->pixelbytes + 3) & ~3;
	pixbuf->pixels   = e_malloc(pixbuf->rowbytes * pixbuf->h);

	return pixbuf;
}

static euint32 pixbuf_read_pixel(GalPixbuf *pixbuf, eint x, eint y)
{
	euint8 *line = pixbuf->pixels + y * pixbuf->rowbytes;

	if (pixbuf->pixelbytes == 3) {
		euint32 pixel;
		((euchar *)&pixel)[0] = (line + x * 3)[0];
		((euchar *)&pixel)[1] = (line + x * 3)[1];
		((euchar *)&pixel)[2] = (line + x * 3)[2];
		((euchar *)&pixel)[3] = 0xff;
		return pixel;
	}
	return ((euint32 *)line)[x];
}

euint8 pixbuf_read_pixel8(GalPixbuf *pixbuf, eint x, eint y)
{
	//euint32 t = pixbuf_read_pixel(pixbuf, x, y);
	return 0;
}

euint16 pixbuf_read_pixel16(GalPixbuf *pixbuf, eint x, eint y)
{
	euint32 t = pixbuf_read_pixel(pixbuf, x, y);
	return ((t&0xf80000) >> 8) + ((t&0xfc00) >> 5) + ((t&0xf8) >> 3); 
}

euint32 pixbuf_read_pixel32(GalPixbuf *pixbuf, eint x, eint y)
{
	return pixbuf_read_pixel(pixbuf, x, y);
}

void egal_pixbuf_composite(GalPixbuf *dst,
		int dst_x, int dst_y,
		int dst_w, int dst_h,
		GalPixbuf *src,
		int offset_x, int offset_y,
		double scale_x, double scale_y,
		PixopsInterpType  interp_type)
{
	_pixops_composite(dst->pixels,
			dst->w, dst->h,
			dst->rowbytes,
			dst->pixelbytes,
			false,
			src->pixels,
			src->w, src->h,
			src->rowbytes,
			src->pixelbytes,
			(src->pixelbytes == 4),
			dst_x, dst_y,
			dst_w, dst_h,
			offset_x, offset_y, 
			scale_x, scale_y,
			interp_type,
			255);
}

void egal_pixbuf_fill(GalPixbuf *pixbuf, euint32 color)
{
	eint x, y;
	eint w = pixbuf->w;
	eint h = pixbuf->h;

	euint8 *p = pixbuf->pixels;
    if (pixbuf->pixelbytes == 3) {
        for (y = 0; y < h; y++) {
			euint8 *t = p;
            for (x = 0; x < w; x++, t += 3) {
				t[0] = color & 0xf;
				t[1] = (color & 0xf0) >> 8;
				t[2] = (color & 0xf00) >> 16;
			}
            p += pixbuf->rowbytes;
        }
    }
    else if (pixbuf->pixelbytes == 4) {
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++)
                ((euint32 *)p)[x] = color;
            p += pixbuf->rowbytes;
        }
    }
}

void egal_pixbuf_copy_area(GalPixbuf *dst, eint dx, eint dy, eint w, eint h, GalPixbuf *src, eint sx, eint sy)
{
    eint x, y;
    euint32 *s = (euint32 *)(src->pixels + src->rowbytes * sy + sx * src->pixelbytes);
    euint32 *d = (euint32 *)(dst->pixels + dst->rowbytes * dy + dx * dst->pixelbytes);

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++)
            d[x] = s[x];
        d += dst->w;
        s += src->w;
    }
}

GalPixbuf *egal_pixbuf_copy(GalPixbuf *pixbuf)
{
	GalPixbuf *new = egal_pixbuf_new(pixbuf->alpha, pixbuf->w, pixbuf->h);
	egal_pixbuf_copy_area(new, 0, 0, pixbuf->w, pixbuf->h, pixbuf, 0, 0);
	return new;
}

GalPixbuf *egal_pixbuf_new_subpixbuf(GalPixbuf *pixbuf, eint xoffset, eint yoffset, eint width, eint height)
{
	GalPixbuf *new = egal_pixbuf_new(pixbuf->alpha, width, height);
	egal_pixbuf_copy_area(new, xoffset, yoffset, width, height, pixbuf, 0, 0);
	return new;
}
