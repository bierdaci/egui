#ifndef __WIN32_WINDOW_H
#define __WIN32_WINDOW_H

#include <egal/window.h>
#include <egal/font.h>
#include <egal/types.h>

typedef struct _GalDrawable32	GalDrawable32;
typedef struct _GalWindow32		GalWindow32;
typedef struct _GalCursor32		GalCursor32;
typedef struct _GalImage32		GalImage32;
typedef struct _GalPB32			GalPB32;

struct _GalDrawable32 {
	eint w, h;
	eint depth;
	HWND handle;
	ebool isdev;
	GalSurface surface;
};

struct _GalWindow32 {
	GalWindowAttr attr;
	eint x, y;
	eint w, h;

	eint min_w, min_h;

	GalWindow32 *parent;
	HWND hwnd;
	GalCursor cursor;
	BOOL is_enter;
	BOOL is_configure;
	DWORD ntype;
#ifdef _GAL_SUPPORT_OPENGL
	HDC hdc;
	HGLRC hrc;
#endif

	list_t list;
	list_t child_head;
	e_thread_mutex_t lock;

	eHandle attachmen;
};

struct _GalImage32 {
	GalImage image;
	HBITMAP  hbitmap;
	GalImage gimg;
	GalPB    pb;
	GalDrawable bitmap;
};

struct _GalPB32 {
	GalDrawable32 *draw;
	HDC hdc;
	HGDIOBJ oldbmp;
	HBRUSH  colorbrush;
	HPEN    pen;
	GalPBAttr attr;
	DWORD pen_style;
	DWORD pen_width;
	DWORD fg_color;
	DWORD font_color;
};

struct _GalCursor32 {
	GalCursorType type;
	echar   *name;
	HCURSOR hcursor;
};

GalFont w32_create_font(GalPattern *pattern);

eGeneType w32_genetype_pb(void);
eGeneType w32_genetype_window(void);
eGeneType w32_genetype_bitmap(void);
eGeneType w32_genetype_cursor(void);
eGeneType w32_genetype_drawable(void);

#define W32_PB_DATA(hobj)			((GalPB32       *)e_object_type_data(hobj, w32_genetype_pb()))
#define W32_WINDOW_DATA(hobj)		((GalWindow32   *)e_object_type_data(hobj, w32_genetype_window()))
#define W32_BITMAP_DATA(hobj)		((GalBitmap32   *)e_object_type_data(hobj, w32_genetype_bitmap()))
#define W32_CURSOR_DATA(hobj)		((GalCursor32   *)e_object_type_data(hobj, w32_genetype_cursor()))
#define W32_DRAWABLE_DATA(hobj)		((GalDrawable32 *)e_object_type_data(hobj, w32_genetype_drawable()))

#endif