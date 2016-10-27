#ifndef __GAL_REGION_H
#define __GAL_REGION_H

#include <egal/types.h>
#include <egal/rect.h>

typedef struct _GalRegionSec GalRegionSec;

struct _GalRegionSec {
	GalSection    sec;
	GalRegionSec *next;
	GalRegionSec *prev;
};

struct _GalRegion {
    GalSection bound;
    GalRegionSec *head;
    GalRegionSec *tail;
};

#define REGION_IS_EMPTY(region) (!(region)->head)
#define REGION_NO_EMPTY(region) ( (region)->head)

void egal_region_bound_get(GalRegion *region, GalRect *prc);
ebool egal_region_pt_in(GalRegion *region, int x, int y);
ebool egal_region_rect_in(GalRegion *region, const GalRect *prc);
void egal_region_init(GalRegion *region);
ebool egal_region_is_empty(const GalRegion * region);
void egal_region_empty(GalRegion *region);

GalRegion *egal_region_rect(const GalRect *);
GalRegion *egal_region_new(void);
GalRegion *egal_region_copy1(const GalRegion *);
void egal_region_destroy(GalRegion *region);
ebool egal_region_set_rect(GalRegion *region, const GalRect *prc);
ebool egal_region_copy(GalRegion *dreg, const GalRegion *sreg);
ebool egal_region_intersect(GalRegion *dst, const GalRegion *src1, const GalRegion *src2);
ebool egal_region_subtract(GalRegion *rgnD, const GalRegion *rgnM, const GalRegion *rgnS);
ebool egal_region_union(GalRegion *dst, const GalRegion *src1, const GalRegion *src2);
ebool egal_region_xor(GalRegion *dst, const GalRegion *src1, const GalRegion *src2);
void egal_region_offset(GalRegion *region, int x, int y);
ebool egal_region_intersect_rect(GalRegion *dst, const GalRect *prc);

ebool egal_region_subtract_rect(GalRegion *dst, const GalRect *prc);
ebool egal_region_union_rect(GalRegion *dst, const GalRect *prc);

void egal_section_to_rect(GalRect *prc, const GalRegionSec *chain);

#endif
