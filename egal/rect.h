#ifndef __GAL_RECT_H_
#define __GAL_RECT_H_

#include <egal/types.h>

typedef struct _GalSection   GalSection;
struct _GalSection {
	 int x1, y1;
	 int x2, y2;
};

#define RECT_IS_EMPTY(prc)  egal_rect_is_empty(prc)
#define RECT_NO_EMPTY(prc) !egal_rect_is_empty(prc)

void egal_rect_empty(GalRect *prc);
void egal_rect_set(GalRect *prc, int x, int y, int w, int h);
void egal_rect_inflate(GalRect *prc, int cx, int cy);
void egal_rect_offset(GalRect *prc, int x, int y);
bool egal_rect_is_empty(const GalRect *prc);
bool egal_rect_equal(const GalRect *prc1, const GalRect *prc2);
void egal_rect_normalize(GalRect *prc);
bool egal_rect_is_covered(const GalRect *prc1, const GalRect *prc2);
bool egal_rect_intersect(GalRect *pdrc, const GalRect *psrc1, const GalRect *psrc2);
bool egal_rect_is_intersect(const GalRect *psrc1, const GalRect *psrc2);
bool egal_rect_point_in(const GalRect *prc, int x, int y);
void egal_rect_get_bound(GalRect *,  const GalRect *, const GalRect *);

#endif
