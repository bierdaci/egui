#include <stdlib.h>

#include "image.h"

void egal_fill_image(GalImage *image, euint32 color, eint x, eint y, eint w, eint h)
{
    if (w == 0)
        w = image->w - x;
    if (h == 0)
        h = image->h- y;
    if (x + w > image->w)
        w = image->w - x;
    if (y + h > image->h)
        h = image->h - y;

    if (image->pixelbytes == 2) {
        euint16 *buf = (euint16 *)(image->pixels + image->rowbytes*y + x*image->pixelbytes);
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++)
                buf[x] = (euint16)color;
            buf += image->w;
        }
    }
    else if (image->pixelbytes == 4) {
        euint32 * buf = (euint32 *)(image->pixels + image->rowbytes*y + x*image->pixelbytes);
        for (y = 0; y < h; y++) {
            for (x = 0; x < w; x++)
                buf[x] = color;
            buf += image->w;
        }
    }
}

static void copy_image(GalImage *dst, 
		eint dx, eint dy,
		GalImage *src,
		eint sx, eint sy,
		eint w, eint h)
{
    eint x, y;
    euint32 *sbuf = (euint32 *)(src->pixels + src->rowbytes * sy + sx * src->pixelbytes);
    euint32 *dbuf = (euint32 *)(dst->pixels + dst->rowbytes * dy + dx * dst->pixelbytes);

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++)
            dbuf[x] = sbuf[x];
        dbuf += dst->w;
        sbuf += src->w;
    }
}

void egal_copy_image(GalImage *dst,
		eint dx, eint dy, eint w, eint h,
		GalImage *src, eint sx, eint sy)
{
	if (dx >= dst->w || dy >= dst->h)
		return;
    if (w == 0)
        w = src->w;
    if (h == 0)
        h = src->h;
    if (sx >= dst->w)
        return;
    if (sy >= dst->h)
        return;
    if (sx + w > src->w)
        w = src->w - sx;
    if (w > dst->w - dx)
        w = dst->w - dx;
    if (sy + h > src->h)
        h = src->h - sy;
    if (h > dst->h - dy)
        h = dst->h - dy;

	copy_image(dst, dx, dy, src, sx, sy, w, h);
}

static void image8_copy_from_pixbuf(GalImage *image,
		eint dx, eint dy, 
		GalPixbuf *pixbuf,
		eint sx, eint sy,
		eint w, eint h)
{
    eint x, y;
    euint8 *p;

	if (image->negative) {
		p = image->pixels + image->rowbytes * (dy + h - 1) + dx * image->pixelbytes;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel8(pixbuf, x + sx, y + sy);
			p -= image->w;
		}
	}
	else {
		p = image->pixels + image->rowbytes * dy + dx * image->pixelbytes;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel8(pixbuf, x + sx, y + sy);
			p += image->w;
		}
	}
}

static void image16_copy_from_pixbuf(GalImage *image,
		eint dx, eint dy, 
		GalPixbuf *pixbuf,
		eint sx, eint sy,
		eint w, eint h)
{
    eint x, y;
    euint16 *p;

	if (image->negative) {
		p = (euint16 *)(image->pixels + image->rowbytes * (dy + h - 1) + dx * image->pixelbytes);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel16(pixbuf, x + sx, y + sy);
			p -= image->w;
		}
	}
	else {
		p = (euint16 *)(image->pixels + image->rowbytes * dy + dx * image->pixelbytes);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel16(pixbuf, x + sx, y + sy);
			p += image->w;
		}
	}
}

static void image24_copy_from_pixbuf(GalImage *image,
		eint dx, eint dy,
		GalPixbuf *pixbuf,
		eint sx, eint sy,
		eint w, eint h)
{
    eint x, y;
    euint8 *p;

	if (image->negative) {
		p = image->pixels + image->rowbytes * (dy + h - 1) + dx * image->pixelbytes;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				euint32 u = pixbuf_read_pixel32(pixbuf, x + sx, y + sy);
				p[x * 3 + 0] = (0xff & u);
				p[x * 3 + 1] = (0xff00 & u) >> 8;
				p[x * 3 + 2] = (0xff0000 & u) >> 16;
			}
			p -= image->rowbytes;
		}
	}
	else {
		p = image->pixels + image->rowbytes * dy + dx * image->pixelbytes;
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++) {
				euint32 u = pixbuf_read_pixel32(pixbuf, x + sx, y + sy);
				p[x * 3 + 0] = (0xff & u);
				p[x * 3 + 1] = (0xff00 & u) >> 8;
				p[x * 3 + 2] = (0xff0000 & u) >> 16;
			}
			p += image->rowbytes;
		}
	}
}

static void image32_copy_from_pixbuf(GalImage *image,
		eint dx, eint dy,
		GalPixbuf *pixbuf,
		eint sx, eint sy,
		eint w, eint h)
{
    eint x, y;
    euint32 *p;

	if (image->negative) {
		p = (euint32 *)(image->pixels + image->rowbytes * (dy + h - 1) + dx * image->pixelbytes);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel32(pixbuf, x + sx, y + sy);
			p -= image->w;
		}
	}
	else {
		p = (euint32 *)(image->pixels + image->rowbytes * dy + dx * image->pixelbytes);
		for (y = 0; y < h; y++) {
			for (x = 0; x < w; x++)
				p[x] = pixbuf_read_pixel32(pixbuf, x + sx, y + sy);
			p += image->w;
		}
	}
}

void egal_image_copy_from_pixbuf(GalImage *image,
		eint dx, eint dy,
		GalPixbuf *pixbuf,
		eint sx, eint sy,
		eint w, eint h)
{
	if (dx >= image->w || dy >= image->h)
		return;
    if (w == 0)
        w = pixbuf->w;
    if (h == 0)
        h = pixbuf->h;
    if (sx >= image->w)
        return;
    if (sy >= image->h)
        return;
    if (sx + w > pixbuf->w)
        w = pixbuf->w - sx;
    if (w > image->w - dx)
        w = image->w - dx;
    if (sy + h > pixbuf->h)
        h = pixbuf->h - sy;
    if (h > image->h - dy)
        h = image->h - dy;

    if (image->pixelbytes == 1)
        image8_copy_from_pixbuf(image, dx, dy, pixbuf, sx, sy, w, h);
	else if (image->pixelbytes == 2)
        image16_copy_from_pixbuf(image, dx, dy, pixbuf, sx, sy, w, h);
	else if (image->pixelbytes == 3)
		image24_copy_from_pixbuf(image, dx, dy, pixbuf, sx, sy, w, h);
    else if (image->pixelbytes == 4)
        image32_copy_from_pixbuf(image, dx, dy, pixbuf, sx, sy, w, h);
}

void egal_clear_image(GalImage *image)
{
	eint i, n;

	if (!image->pixels)
		return;

	n = image->w * image->h;

	if (image->pixelbytes == 2) {
		euint16 *pixels16 = (euint16 *)image->pixels;
		for (i = 0; i < n; i++)
			pixels16[i] = 0;
	}
	else if (image->pixelbytes == 4) {
		euint32 * pixels32 = (euint32 *)image->pixels;
		for (i = 0; i < n; i++)
			pixels32[i] = 0;	
	}
}

GalImage *egal_image_new_from_pixbuf(GalPixbuf *pixbuf)
{
	GalImage *image;
	image = egal_image_new(pixbuf->w, pixbuf->h, pixbuf->alpha);
	egal_image_copy_from_pixbuf(image, 0, 0, pixbuf, 0, 0, pixbuf->w, pixbuf->h);
	return image;
}

GalImage *egal_image_new_from_file(const echar *filename)
{
	GalPixbuf *pixbuf;
	GalImage  *image;

	pixbuf = egal_pixbuf_new_from_file(filename, 1.0, 1.0);
	if (pixbuf == NULL)
		return NULL;

	image = egal_image_new(pixbuf->w, pixbuf->h, pixbuf->alpha);
	egal_image_copy_from_pixbuf(image, 0, 0, pixbuf, 0, 0, pixbuf->w, pixbuf->h);
	egal_pixbuf_free(pixbuf);

	return image;
}
