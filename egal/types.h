#ifndef __GAL_TYPES_H__
#define __GAL_TYPES_H__

#ifdef WIN32
#include <ewinconfig.h>
#pragma comment(lib, "msimg32.lib") 
#pragma comment(lib, "imm32.lib")
#else
#include <econfig.h>
#endif

#include <elib/types.h>

typedef euint eGlyph;
typedef struct _GalPoint   GalPoint;
typedef struct _GalRect    GalRect;
typedef struct _GalSegment GalSegment;
typedef struct _GalSpan    GalSpan;
typedef struct _GalRegion  GalRegion;

struct _GalPoint {
	eint x;
	eint y;
};

struct _GalRect {
	eint x, y;
	eint w, h;
};

struct _GalSegment {
	eint x1, y1;
	eint x2, y2;
};

struct _GalSpan {
	eint x, y;
	eint w;
};


#endif
