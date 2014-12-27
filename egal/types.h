#ifndef __GAL_TYPES_H__
#define __GAL_TYPES_H__

#ifdef WIN32
#include <ewinconfig.h>
#else
#include <econfig.h>
#endif

#include <elib/types.h>

#ifdef _GAL_SUPPORT_OPENGL

#ifdef WIN32
#define _WIN32_WINNT	0x0500
#include <windows.h>
#include <io.h>
#include <gl/gl.h>
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib, "msimg32.lib")
#elif linux
#include <GL/glx.h>
#include <GL/glu.h>
#endif

#endif

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
