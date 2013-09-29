#include <string.h>
#include "rect.h"

void egal_rect_empty(GalRect *prc)
{
	prc->w = 0;
	prc->h = 0;
}

void egal_rect_set(GalRect *prc, int x, int y, int w, int h)
{
    prc->x = x;
    prc->y = y;
    prc->w = w;
    prc->h = h;
	egal_rect_normalize(prc);
}

void egal_rect_inflate(GalRect *prc, int cx, int cy)
{
    prc->x -= cx;
    prc->y -= cy;
    prc->w += cx * 2;
    prc->h += cy * 2;
}

void egal_rect_offset(GalRect *prc, int x, int y)
{
    prc->x += x;
    prc->y += y;
}

bool egal_rect_is_empty(const GalRect *prc)
{
	if (prc->w == 0) return true;
	if (prc->h == 0) return true;
	return false;
}

bool egal_rect_equal(const GalRect *prc1, const GalRect *prc2)
{
	if (prc1->x != prc2->x) return false;
	if (prc1->y != prc2->y) return false;
	if (prc1->w != prc2->w) return false;
	if (prc1->h != prc2->h) return false;

	return true;
}

bool egal_rect_point_in(const GalRect *prc, int x, int y)
{
	int x1 = prc->x;
	int y1 = prc->y;
	int x2 = prc->x + prc->w;
	int y2 = prc->y + prc->h;
	if (x >= x1 && x < x2 && y >= y1 && y < y2)
		return true;
	return false;
}

void egal_rect_normalize(GalRect *prc)
{
	if (prc->w < 0) {
		prc->x += prc->w;
		prc->w = -prc->w;
	}
	if (prc->h < 0) {
		prc->y += prc->h;
		prc->h = -prc->h;
	}
}

bool egal_rect_is_covered(const GalRect *prc1, const GalRect *prc2)
{
	int x1 = prc1->x + prc1->w;
	int y1 = prc1->y + prc1->h;
	int x2 = prc2->x + prc2->w;
	int y2 = prc2->y + prc2->h;

	if (prc1->x < prc2->x || prc1->y < prc2->y || x1 > x2 || y1 > y2)
		return false;

	return true;
}

bool egal_rect_intersect(GalRect *pdrc, const GalRect *psrc1, const GalRect *psrc2)
{
	int l, t, r, b;
	int r1 = psrc1->x + psrc1->w;
	int b1 = psrc1->y + psrc1->h;
	int r2 = psrc2->x + psrc2->w;
	int b2 = psrc2->y + psrc2->h;
	l = (psrc1->x > psrc2->x) ? psrc1->x : psrc2->x;
	t = (psrc1->y > psrc2->y) ? psrc1->y : psrc2->y;
	r = (r1 < r2) ? r1 : r2;
	b = (b1 < b2) ? b1 : b2;

	if (l >= r || t >= b)
		return false;

	pdrc->x = l;
	pdrc->y = t;
	pdrc->w = r - l;
	pdrc->h = b - t;

	return true;
}

bool egal_rect_is_intersect(const GalRect *psrc1, const GalRect *psrc2)
{
	int l, t, r, b;
	int r1 = psrc1->x + psrc1->w;
	int b1 = psrc1->y + psrc1->h;
	int r2 = psrc2->x + psrc2->w;
	int b2 = psrc2->y + psrc2->h;

	l = (psrc1->x > psrc2->x) ? psrc1->x : psrc2->x;
	t = (psrc1->y > psrc2->y) ? psrc1->y : psrc2->y;
	r = (r1 < r2) ? r1 : r2;
	b = (b1 < b2) ? b1 : b2;

	if (l >= r || t >= b)
		return false;

	return true;
}

void egal_rect_get_bound(GalRect *pdrc,  const GalRect *psrc1, const GalRect *psrc2)
{
	GalSection src1, src2;
	int l, t, r, b;

	src1.x1 = psrc1->x;
	src1.y1 = psrc1->y;
	src1.x2 = psrc1->x + psrc1->w;
	src1.y2 = psrc1->y + psrc1->h;
	src2.x1 = psrc2->x;
	src2.y1 = psrc2->y;
	src2.x2 = psrc2->x + psrc2->w;
	src2.y2 = psrc2->y + psrc2->h;

	l = (src1.x1 < src2.x1) ? src1.x1 : src2.x1;
	t = (src1.y1 < src2.y1) ? src1.y1 : src2.y1;
	r = (src1.x2 > src2.x2) ? src1.x2 : src2.x2;
	b = (src1.y2 > src2.y2) ? src1.y2 : src2.y2;

	pdrc->x = l;
	pdrc->y = t;
	pdrc->w = r - l;
	pdrc->h = b - t;
	egal_rect_normalize(pdrc);
}
