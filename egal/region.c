#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <elib/std.h>
#include "region.h"
#include "rect.h"

GalRegionSec *region_section_new(void);
void free_sechain(GalRegionSec *chain);
typedef void (*voidProcp1)(GalRegion *region,
		const GalRegionSec *r1, const GalRegionSec *r1end,
		const GalRegionSec *r2, const GalRegionSec *r2end,
		int y1, int y2);
typedef void (*voidProcp2)(GalRegion *region,
		const GalRegionSec *r, const GalRegionSec *rend,
		int y1, int y2);

#define EXTENTCHECK(r1, r2) \
	((r1)->x2 > (r2)->x1 && \
	 (r1)->x1 < (r2)->x2 && \
	 (r1)->y2 > (r2)->y1 && \
	 (r1)->y1 < (r2)->y2)

#define NEWREGIONSEC(region, prc) \
{\
	prc = region_section_new();\
	prc->next = NULL;\
	prc->prev = region->tail;\
	if (region->tail)\
		region->tail->next = prc;\
	region->tail = prc;\
	if (region->head == NULL)\
		region->head = prc;\
}

#define INSEGMENT(r, x, y) \
	((r).x2 >  (x) && \
	 (r).x1 <= (x) && \
	 (r).y2 >  (y) && \
	 (r).y1 <= (y))

static ebool section_is_intersect(const GalSection *psrc1, const GalSection *psrc2)
{
	int l, t, r, b;

	l = (psrc1->x1 > psrc2->x1) ? psrc1->x1 : psrc2->x1;
	t = (psrc1->y1 > psrc2->y1) ? psrc1->y1 : psrc2->y1;
	r = (psrc1->x2 < psrc2->x2) ? psrc1->x2 : psrc2->x2;
	b = (psrc1->y2 < psrc2->y2) ? psrc1->y2 : psrc2->y2;

	if (l >= r || t >= b)
		return efalse;

	return etrue;
}

static void set_section_empty(GalSection *sec)
{
    e_memset(sec, 0, sizeof(GalSection));
}

static void normalize_section(GalSection *sec)
{
	int temp;

	if (sec->x1 > sec->x2) {
		temp   = sec->x1;
		sec->x1 = sec->x2;
		sec->x2 = temp;
	}

	if (sec->y1 > sec->y2) {
		temp   = sec->y1;
		sec->y1 = sec->y2;
		sec->y2 = temp;
	}
}

static void section_to_rect(GalRect *prc, const GalSection *sec)
{
	prc->x = sec->x1;
	prc->y = sec->y1;
	prc->w = sec->x2 - sec->x1;
	prc->h = sec->y2 - sec->y1;
	egal_rect_normalize(prc);
}

static void rect_to_section(const GalRect *prc, GalSection *sec)
{
	sec->x1 = prc->x;
	sec->y1 = prc->y;
	sec->x2 = prc->x + prc->w;
	sec->y2 = prc->y + prc->h;
	normalize_section(sec);
}

static void offset_section(GalSection *sec, int x, int y)
{
    sec->x1 += x;
    sec->y1 += y;
    sec->x2 += x;
    sec->y2 += y;
}

ebool egal_region_pt_in(GalRegion *region, int x, int y)
{
	int y1;
	GalRegionSec *chain = region->head;

	if (!region->head || y >= region->tail->sec.y2 || y < region->head->sec.y1)
		return efalse;

	chain = region->head;
	while (chain && y >= chain->sec.y2)
		chain = chain->next;

	if (!chain) return efalse;

	y1 = chain->sec.y1;
	while (chain && chain->sec.y1 == y1) {
		if (INSEGMENT(chain->sec, x, y))
			return etrue;
		chain = chain->next;
	}

	return efalse;
}

ebool egal_region_rect_in(GalRegion *region, const GalRect *prc)
{
	GalRegionSec *chain = region->head;
	GalSection    sec;
	ebool ret = efalse;

	rect_to_section(prc, &sec);

	if (chain && EXTENTCHECK(&region->bound, &sec)) {
		while (chain) {
			if (chain->sec.y2 <= sec.y1) {
				chain = chain->next;
				continue;
			}

			if (chain->sec.y1 >= sec.y2) {
				chain = chain->next;
				continue;
			}

			if (chain->sec.x2 <= sec.x1) {
				chain = chain->next;
				continue;
			}

			if (chain->sec.x1 >= sec.x2) {
				chain = chain->next;
				continue;
			}

			ret = etrue;
			break;
		}
	}

	return ret;
}

void egal_region_init(GalRegion *region)
{
	set_section_empty(&region->bound);
	region->head = NULL;
	region->tail = NULL;
}

void egal_region_get_bound(GalRegion *region, GalRect *prc)
{
	prc->x = region->bound.x1;
	prc->y = region->bound.y1;
	prc->w = region->bound.x2 - region->bound.x1;
	prc->h = region->bound.y2 - region->bound.y1;
	egal_rect_normalize(prc);
}

ebool egal_region_is_empty(const GalRegion * region)
{
	if (region->head == NULL)
		return etrue;

	return efalse;
}

void egal_region_empty(GalRegion *region)
{
	GalRegionSec *chain, *pt;

	chain = region->head;
	while (chain) {
		pt = chain->next;
		free_sechain(chain);
		chain = pt;
	}

	set_section_empty(&region->bound);
	region->head = NULL;
	region->tail = NULL;
}

GalRegion *egal_region_new(void)
{
	GalRegion *region = e_malloc(sizeof(GalRegion));
	egal_region_init(region);
	return region;
}


GalRegion *egal_region_rect(const GalRect *rect)
{
	GalRegion    *region = e_malloc(sizeof(GalRegion));
	GalRegionSec *rgnsec;

	rgnsec = region_section_new();
	rect_to_section(rect, &rgnsec->sec);
	rgnsec->next = NULL;
	rgnsec->prev = NULL;

	region->head  = region->tail = rgnsec;
	region->bound = rgnsec->sec;
	return region;
}

void egal_region_destroy(GalRegion *region)
{
	egal_region_empty(region);
	e_free(region);
}

ebool egal_region_set_rect(GalRegion *region, const GalRect *prc)
{
	GalRegionSec *chain;
	GalSection sec;

	if (RECT_IS_EMPTY(prc))
		return efalse;

	rect_to_section(prc, &sec);
	egal_region_empty(region);

	chain = region_section_new();
	if (chain == NULL)
		return efalse;

	chain->sec = sec;
	chain->next = NULL;
	chain->prev = NULL;

	region->head = region->tail = chain;
	region->bound = sec;

	return etrue;
}

ebool egal_region_copy(GalRegion *dreg, const GalRegion *sreg)
{
	GalRegionSec *pcr;
	GalRegionSec *new, *prev;

	if (dreg == sreg)
		return efalse;

	egal_region_empty(dreg);
	if (!(pcr = sreg->head))
		return etrue;

	new = region_section_new();

	dreg->head = new;
	new->sec = pcr->sec;

	prev = NULL;
	while (pcr->next) {
		new->next = region_section_new();
		new->prev = prev;

		prev = new;
		pcr = pcr->next;
		new = new->next;

		new->sec = pcr->sec;
	}

	new->prev = prev;
	new->next = NULL;
	dreg->tail = new;

	dreg->bound = sreg->bound; 

	return etrue;
}

GalRegion *egal_region_copy1(const GalRegion *region)
{
	GalRegion *new = egal_region_new();
	egal_region_copy(new, region);
	return new;
}

static void region_set_extents(GalRegion *region)
{
	GalRegionSec *chain;
	GalSection *sec;

	if (region->head == NULL) {
		region->bound.y1 = 0;
		region->bound.x1 = 0;
		region->bound.x2 = 0;
		region->bound.y2 = 0;
		return;
	}

	sec = &region->bound;

	sec->y1 = region->head->sec.y1;
	sec->x1 = region->head->sec.x1;
	sec->x2 = region->tail->sec.x2;
	sec->y2 = region->tail->sec.y2;

	chain = region->head;
	while (chain) {
		if (chain->sec.x1 < sec->x1)
			sec->x1 = chain->sec.x1;
		if (chain->sec.x2 > sec->x2)
			sec->x2 = chain->sec.x2;

		chain = chain->next;
	}
}

static GalRegionSec *region_coalesce(GalRegion *region, GalRegionSec *prev_start, GalRegionSec *cur_start)
{
	GalRegionSec *new_start;
	GalRegionSec *prev_rect;
	GalRegionSec *cur_rect;
	GalRegionSec *temp;
	int cur_num;
	int prev_num;
	int bandtop;

	if (prev_start == NULL)
		prev_start = region->head;
	if (cur_start == NULL)
		cur_start = region->head;

	if (prev_start == cur_start)
		return prev_start;

	new_start = cur_rect = cur_start;

	prev_rect = prev_start;
	temp = prev_start;
	prev_num = 0;
	while (temp != cur_start) {
		prev_num ++;
		temp = temp->next;
	}

	cur_rect = cur_start;
	bandtop = cur_rect->sec.y1;
	cur_num = 0;
	while (cur_rect && (cur_rect->sec.y1 == bandtop)) {
		cur_num ++;
		cur_rect = cur_rect->next;
	}

	if (cur_rect) {
		temp = region->tail;
		while (temp->prev->sec.y1 == temp->sec.y1)
			temp = temp->prev;
		new_start = temp;
	}

	if ((cur_num == prev_num) && (cur_num != 0)) {
		cur_rect = cur_start;
		if (prev_rect->sec.y2 == cur_rect->sec.y1) {
			do {
				if (prev_rect->sec.x1 != cur_rect->sec.x1 ||
						prev_rect->sec.x2 != cur_rect->sec.x2)
					return new_start;
				prev_rect = prev_rect->next;
				cur_rect = cur_rect->next;
			} while (--prev_num);

			if (cur_rect == NULL)
				new_start = prev_start;

			cur_rect = cur_start;
			prev_rect = prev_start;
			do {
				prev_rect->sec.y2 = cur_rect->sec.y2;
				prev_rect = prev_rect->next;

				if (cur_rect->next)
					cur_rect->next->prev = cur_rect->prev;
				else
					region->tail = cur_rect->prev;
				if (cur_rect->prev)
					cur_rect->prev->next = cur_rect->next;
				else
					region->head = cur_rect->next;

				temp = cur_rect->next;
				free_sechain(cur_rect);
				cur_rect = temp;
			} while (--cur_num);
		}
	}
	return new_start;
}

static void region_op(
		GalRegion *newrgn,
		const GalRegion *reg1,
		const GalRegion *reg2,
		voidProcp1 overlap_func,
		voidProcp2 non_overlap_func1,
		voidProcp2 non_overlap_func2)
{
	GalRegion my_dst, *pdst;
	const GalRegionSec *r1, *r2;
	const GalRegionSec *r1end, *r2end;
	GalRegionSec *cur, *prev;
	int ybot, bot;
	int ytop, y1;

	r1 = reg1->head;
	r2 = reg2->head;

	if (newrgn == reg1 || newrgn == reg2) {
		egal_region_init(&my_dst);
		pdst = &my_dst;
	}
	else {
		egal_region_empty(newrgn);
		pdst = newrgn;
	}

	if (reg1->bound.y1 < reg2->bound.y1)
		ybot = reg1->bound.y1;
	else
		ybot = reg2->bound.y1;

	prev = pdst->head;

	do {
		cur = pdst->tail;

		r1end = r1;
		while (r1end && r1end->sec.y1 == r1->sec.y1)
			r1end = r1end->next;

		r2end = r2;
		while (r2end && r2end->sec.y1 == r2->sec.y1)
			r2end = r2end->next;

		if (r1->sec.y1 < r2->sec.y1) {
			y1 = MAX(r1->sec.y1, ybot);
			bot = MIN(r1->sec.y2, r2->sec.y1);

			if (y1 != bot && non_overlap_func1 != NULL)
				(*non_overlap_func1)(pdst, r1, r1end, y1, bot);

			ytop = r2->sec.y1;
		}
		else if (r2->sec.y1 < r1->sec.y1) {
			y1 = MAX(r2->sec.y1, ybot);
			bot = MIN(r2->sec.y2, r1->sec.y1);

			if (y1 != bot && non_overlap_func2 != NULL)
				(*non_overlap_func2)(pdst, r2, r2end, y1, bot);

			ytop = r1->sec.y1;
		}
		else {
			ytop = r1->sec.y1;
		}

		if (pdst->tail != cur)
			prev = region_coalesce(pdst, prev, cur);

		ybot = MIN(r1->sec.y2, r2->sec.y2);
		cur = pdst->tail;
		if (ybot > ytop)
			(*overlap_func)(pdst, r1, r1end, r2, r2end, ytop, ybot);

		if (pdst->tail != cur)
			prev = region_coalesce(pdst, prev, cur);

		if (r1->sec.y2 == ybot)
			r1 = r1end;
		if (r2->sec.y2 == ybot)
			r2 = r2end;
	} while (r1 && r2);

	cur = pdst->tail;
	if (r1) {
		if (non_overlap_func1 != NULL) {
			do {
				r1end = r1;
				while (r1end && r1end->sec.y1 == r1->sec.y1)
					r1end = r1end->next;
				(*non_overlap_func1)(pdst, r1, r1end, MAX(r1->sec.y1, ybot), r1->sec.y2);
				r1 = r1end;
			} while (r1);
		}
	}
	else if (r2 && non_overlap_func2) {
		do {
			r2end = r2;
			while (r2end && r2end->sec.y1 == r2->sec.y1)
				r2end = r2end->next;
			(*non_overlap_func2)(pdst, r2, r2end, MAX(r2->sec.y1, ybot), r2->sec.y2);
			r2 = r2end;
		} while (r2);
	}

	if (pdst->tail != cur)
		(void)region_coalesce(pdst, prev, cur);

	if (pdst != newrgn) {
		egal_region_empty(newrgn);
		*newrgn = my_dst;
	}
}

static void
region_intersect_O(GalRegion *region, const GalRegionSec *r1,
		const GalRegionSec *r1end, const GalRegionSec *r2,
		const GalRegionSec *r2end, int y1, int y2)
{
	int x1, x2;
	GalRegionSec *new;

	while (r1 != r1end && r2 != r2end) {
		x1  = MAX(r1->sec.x1, r2->sec.x1);
		x2 = MIN(r1->sec.x2, r2->sec.x2);

		if (x1 < x2) {
			NEWREGIONSEC(region, new);

			new->sec.y1 = y1;
			new->sec.x1 = x1;
			new->sec.x2 = x2;
			new->sec.y2 = y2;
		}

		if (r1->sec.x2 < r2->sec.x2) {
			r1 = r1->next;
		}
		else if (r2->sec.x2 < r1->sec.x2) {
			r2 = r2->next;
		}
		else {
			r1 = r1->next;
			r2 = r2->next;
		}
	}
}

static void
region_union_non_O(GalRegion *region, const GalRegionSec *r,
		const GalRegionSec *rend, int y1, int y2)
{
	GalRegionSec *new;

	while (r != rend) {
		NEWREGIONSEC(region, new);
		new->sec.y1 = y1;
		new->sec.x1 = r->sec.x1;
		new->sec.x2 = r->sec.x2;
		new->sec.y2 = y2;

		r = r->next;
	}
}

static void
region_union_O(GalRegion *region, const GalRegionSec *r1,
		const GalRegionSec *r1end, const GalRegionSec *r2,
		const GalRegionSec *r2end, int y1, int y2)
{
	GalRegionSec *new;

#define MERGESEC(r) \
	if ((region->head) &&  \
			(region->tail->sec.y1 == y1) &&  \
			(region->tail->sec.y2 == y2) &&  \
			(region->tail->sec.x2 >= r->sec.x1))  \
	{  \
		if (region->tail->sec.x2 < r->sec.x2)  \
			region->tail->sec.x2 = r->sec.x2;  \
	}  \
	else { \
		NEWREGIONSEC(region, new);  \
		new->sec.y1 = y1;  \
		new->sec.y2 = y2;  \
		new->sec.x1 = r->sec.x1;  \
		new->sec.x2 = r->sec.x2;  \
	}  \
	r = r->next;

	while (r1 != r1end && r2 != r2end) {
		if (r1->sec.x1 < r2->sec.x1) {
			MERGESEC(r1);
		}
		else {
			MERGESEC(r2);
		}
	}

	if (r1 != r1end) {
		do {
			MERGESEC(r1);
		} while (r1 != r1end);
	}
	else while (r2 != r2end) {
		MERGESEC(r2);
	}
}

static void region_subtract_non_O1(GalRegion *region,
		const GalRegionSec *r, const GalRegionSec *rend,
		int y1, int y2)
{
	GalRegionSec *new;

	while (r != rend) {
		NEWREGIONSEC(region, new);
		new->sec.x1 = r->sec.x1;
		new->sec.y1 = y1;
		new->sec.x2 = r->sec.x2;
		new->sec.y2 = y2;
		r = r->next;
	}
}

static void region_subtract_O(GalRegion *region,
		const GalRegionSec *r1, const GalRegionSec *r1end,
		const GalRegionSec *r2, const GalRegionSec *r2end,
		int y1, int y2)
{
	GalRegionSec *new;
	int x1;

	x1 = r1->sec.x1;
	while ((r1 != r1end) && (r2 != r2end)) {
		if (r2->sec.x2 <= x1) {
				r2 = r2->next;
		}
		else if (r2->sec.x1 <= x1) {
			x1 = r2->sec.x2;
			if (x1 >= r1->sec.x2) {
				r1 = r1->next;
				if (r1 != r1end)
					x1 = r1->sec.x1;
			}
			else
				r2 = r2->next;
		}
		else if (r2->sec.x1 < r1->sec.x2) {
			NEWREGIONSEC(region, new);
			new->sec.x1 = x1;
			new->sec.y1 = y1;
			new->sec.x2 = r2->sec.x1;
			new->sec.y2 = y2;
			x1 = r2->sec.x2;
			if (x1 >= r1->sec.x2) {
				r1 = r1->next;
				if (r1 != r1end)
					x1 = r1->sec.x1;
			}
			else {
				r2 = r2->next;
			}
		}
		else {
			if (r1->sec.x2 > x1) {
				NEWREGIONSEC(region, new);
				new->sec.x1 = x1;
				new->sec.y1 = y1;
				new->sec.x2 = r1->sec.x2;
				new->sec.y2 = y2;
			}
			r1 = r1->next;
			if (r1 != r1end)
				x1 = r1->sec.x1;
		}
	}

	while (r1 != r1end) {
		NEWREGIONSEC(region, new);
		new->sec.x1 = x1;
		new->sec.y1 = y1;
		new->sec.x2 = r1->sec.x2;
		new->sec.y2 = y2;
		r1 = r1->next;
		if (r1 != r1end)
			x1 = r1->sec.x1;
	}
}

ebool egal_region_intersect(GalRegion *dst, const GalRegion *src1, const GalRegion *src2)
{
	if (!src1->head || !src2->head  ||
			!EXTENTCHECK(&src1->bound, &src2->bound)) {
		egal_region_empty(dst);
		return efalse;
	}
	else
		region_op(dst, src1, src2, region_intersect_O, NULL, NULL);

	region_set_extents(dst);

	return etrue;
}

ebool egal_region_subtract(GalRegion *rgnD, const GalRegion *rgnM, const GalRegion *rgnS)
{
	if (!rgnM->head || !rgnS->head  ||
			!EXTENTCHECK(&rgnM->bound, &rgnS->bound)) {
		egal_region_copy(rgnD, rgnM);
		return etrue;
	}

	region_op(rgnD, rgnM, rgnS, region_subtract_O, region_subtract_non_O1, NULL);

	region_set_extents(rgnD);

	return etrue;
}

ebool egal_region_union(GalRegion *dst, const GalRegion *src1, const GalRegion *src2)
{
	if (src1 == src2 || !src1->head) {
		if (dst != src2)
			egal_region_copy(dst, src2);
		return etrue;
	}

	if (!src2->head) {
		if (dst != src1)
			egal_region_copy(dst, src1);
		return etrue;
	}

	if ((src1->head == src1->tail) &&
			(src1->bound.x1 <= src2->bound.x1) &&
			(src1->bound.y1 <= src2->bound.y1) &&
			(src1->bound.x2 >= src2->bound.x2) &&
			(src1->bound.y2 >= src2->bound.y2))
	{
		if (dst != src1)
			egal_region_copy(dst, src1);
		return etrue;
	}

	if ((src2->head == src2->tail) &&
			(src2->bound.x1 <= src1->bound.x1) &&
			(src2->bound.y1 <= src1->bound.y1) &&
			(src2->bound.x2 >= src1->bound.x2) &&
			(src2->bound.y2 >= src1->bound.y2))
	{
		if (dst != src2)
			egal_region_copy(dst, src2);
		return etrue;
	}

	region_op(dst, src1, src2, region_union_O, region_union_non_O, region_union_non_O);

	region_set_extents(dst);

	return etrue;
}

ebool egal_region_xor(GalRegion *dst, const GalRegion *src1, const GalRegion *src2)
{
	GalRegion tmpa, tmpb;

	egal_region_init(&tmpa);
	egal_region_init(&tmpb);

	egal_region_subtract(&tmpa, src1, src2);
	egal_region_subtract(&tmpb, src2, src1);
	egal_region_union(dst, &tmpa, &tmpb);

	egal_region_empty(&tmpa);
	egal_region_empty(&tmpb);

	return etrue;
}

ebool egal_region_union_rect(GalRegion *dst, const GalRect *prc)
{
	GalRegion region;
	GalRegionSec chain;

	if (RECT_IS_EMPTY(prc))
		return efalse;

	rect_to_section(prc, &chain.sec);
	chain.next = NULL;
	chain.prev = NULL;

	//region.type = SIMPLEREGION;
	region.bound = chain.sec;
	region.head = &chain;
	region.tail = &chain;

	egal_region_union(dst, dst, &region);

	return etrue;
}

ebool egal_region_intersect_rect(GalRegion *dst, const GalRect *prc)
{
	GalRegion region;
	GalRegionSec chain;

	if (RECT_IS_EMPTY(prc)) {
		egal_region_empty(dst);
		return etrue;
	}

	rect_to_section(prc, &chain.sec);
	chain.next = NULL;
	chain.prev = NULL;

	region.bound = chain.sec;
	region.head = &chain;
	region.tail = &chain;

	egal_region_intersect(dst, dst, &region);

	return etrue;
}

ebool egal_region_subtract_rect(GalRegion *dst, const GalRect *prc)
{
	GalRegion region;
	GalRegionSec chain;

	rect_to_section(prc, &chain.sec);
	if (RECT_IS_EMPTY(prc) || !section_is_intersect(&dst->bound, &chain.sec))
		return efalse;

	chain.next = NULL;
	chain.prev = NULL;

	region.bound = chain.sec;
	region.head = &chain;
	region.tail = &chain;

	egal_region_subtract(dst, dst, &region);

	return etrue;
}

void egal_region_offset(GalRegion *region, int x, int y)
{
	GalRegionSec *chain = region->head;

	while (chain) {
		offset_section(&chain->sec, x, y);
		chain = chain->next;
	}

	if (region->head)
		offset_section(&region->bound, x, y);
}

void egal_section_to_rect(GalRect *prc, const GalRegionSec *chain)
{
	section_to_rect(prc, &chain->sec);
}

GalRegionSec *region_section_new(void)
{
	return e_slice_new(GalRegionSec);
}

void free_sechain(GalRegionSec *sec)
{
	e_slice_free(GalRegionSec, sec);
}

