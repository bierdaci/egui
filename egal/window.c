#include "window.h"

GalWindowManager _gwm = {NULL, NULL, NULL, NULL, NULL};
static GalPB default_pbs[GalPBset + 1];

void egal_warp_pointer(GalWindow window, int sx, int sy, int sw, int sh, int dx, int dy)
{
	GAL_WINDOW_ORDERS(window)->warp_pointer(window, sx, sy, sw, sh, dx, dy);
}

void egal_get_pointer(GalWindow window, eint *rootx, eint *rooty, eint *winx, eint *winy, GalModifierType *mask)
{
	GAL_WINDOW_ORDERS(window)->get_pointer(window, rootx, rooty, winx, winy, mask);
}

GalGrabStatus egal_grab_pointer(GalWindow window, bool owner_events, GalCursor cursor)
{
	return GAL_WINDOW_ORDERS(window)->grab_pointer(window, owner_events, cursor);
}

GalGrabStatus egal_ungrab_pointer(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->ungrab_pointer(window);
}

GalGrabStatus egal_grab_keyboard(GalWindow window, bool owner_events)
{
	return GAL_WINDOW_ORDERS(window)->grab_keyboard(window, owner_events);
}

GalGrabStatus egal_ungrab_keyboard(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->ungrab_keyboard(window);
}

eHandle egal_window_get_attachment(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->get_attachment(window);
}

void egal_window_set_attachment(GalWindow window, eHandle obj)
{
	GAL_WINDOW_ORDERS(window)->set_attachment(window, obj);
}

void egal_window_set_geometry_hints(GalWindow window, GalGeometry *geom, GalWindowHints hints)
{
	GAL_WINDOW_ORDERS(window)->set_geometry_hints(window, geom, hints);
}

void egal_window_get_geometry_hints(GalWindow window, GalGeometry *geom, GalWindowHints *hints)
{
	GAL_WINDOW_ORDERS(window)->get_geometry_hints(window, geom, hints);
}

eint egal_window_put(GalWindow parent, GalWindow child, eint x, eint y)
{
	return GAL_WINDOW_ORDERS(parent)->window_put(parent, child, x, y);
}

eint egal_window_show(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->window_show(window);
}

eint egal_window_hide(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->window_hide(window);
}

eint egal_window_move(GalWindow window, eint x, eint y)
{
	return GAL_WINDOW_ORDERS(window)->window_move(window, x, y);
}

eint egal_window_configure(GalWindow window, GalRect *rect)
{
	return GAL_WINDOW_ORDERS(window)->window_configure(window, rect);
}

eint egal_window_resize(GalWindow window, eint w, eint h)
{
	return GAL_WINDOW_ORDERS(window)->window_resize(window, w, h);
}

eint egal_window_move_resize(GalWindow window, eint x, eint y, eint w, eint h)
{
	return GAL_WINDOW_ORDERS(window)->window_move_resize(window, x, y, w, h);
}

eint egal_window_raise(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->window_raise(window);
}

eint egal_window_lower(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->window_lower(window);
}

eint egal_window_remove(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->window_remove(window);
}

void egal_window_set_attr(GalWindow window, GalWindowAttr *attr)
{
	GAL_WINDOW_ORDERS(window)->set_attr(window, attr);
}

void egal_window_get_attr(GalWindow window, GalWindowAttr *attr)
{
	GAL_WINDOW_ORDERS(window)->get_attr(window, attr);
}

void egal_window_get_origin(GalWindow window, eint *x, eint *y)
{
	GAL_WINDOW_ORDERS(window)->get_origin(window, x, y);
}

eint egal_set_cursor(GalWindow window, GalCursor cursor)
{
	return GAL_WINDOW_ORDERS(window)->set_cursor(window, cursor);
}

GalCursor egal_get_cursor(GalWindow window)
{
	return GAL_WINDOW_ORDERS(window)->get_cursor(window);
}

eint egal_window_set_name(GalWindow window, const echar *name)
{
	return GAL_WINDOW_ORDERS(window)->set_name(window, name);
}

void egal_window_make_GL(GalWindow window)
{
	GAL_WINDOW_ORDERS(window)->make_GL(window);
}

void egal_pb_set_attr(GalPB pb, GalPBAttr *attr)
{
	GAL_PB_ORDERS(pb)->set_attr(pb, attr);
}

euint egal_set_font_color(GalPB pb, euint color)
{
	return GAL_PB_ORDERS(pb)->set_color(pb, color);
}

euint egal_get_font_color(GalPB pb)
{
	return GAL_PB_ORDERS(pb)->get_color(pb);
}

euint egal_set_foreground(GalPB pb, euint color)
{
	return GAL_PB_ORDERS(pb)->set_foreground(pb, color);
}

euint egal_set_background(GalPB pb, euint color)
{
	return GAL_PB_ORDERS(pb)->set_background(pb, color);
}

euint egal_get_foreground(GalPB pb)
{
	return GAL_PB_ORDERS(pb)->get_foreground(pb);
}

euint egal_get_background(GalPB pb)
{
	return GAL_PB_ORDERS(pb)->get_background(pb);
}

eint egal_get_depth(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->get_depth(drawable);
}

void egal_composite(GalDrawable dst, GalPB pb, int dx, int dy, GalDrawable src, GalPB pb2, int sx, int sy, int w, int h)
{
	GAL_DRAWABLE_ORDERS(dst)->composite(dst, pb, dx, dy, src, pb2, sx, sy, w, h);
}

void egal_composite_image(GalDrawable drawable, GalPB pb, int dx, int dy, GalImage *image, int sx, int sy, int w, int h)
{
	if (image->alpha)
		GAL_DRAWABLE_ORDERS(drawable)->composite_image(drawable, pb, dx, dy, image, sx, sy, w, h);
}

void egal_composite_subwindow(GalDrawable drawable, int x, int y, int w, int h)
{
	GAL_DRAWABLE_ORDERS(drawable)->composite_subwindow(drawable, x, y, w, h);
}

void egal_draw_drawable(GalDrawable drawable, GalPB pb, int dx, int dy, GalDrawable src, GalPB spb, int sx, int sy, int w, int h)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_drawable(drawable, pb, dx, dy, src, spb, sx, sy, w, h);
}

void egal_draw_image(GalDrawable drawable, GalPB pb, int dx, int dy, GalImage *image, int sx, int sy, int w, int h)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_image(drawable, pb, dx, dy, image, sx, sy, w, h);
}

void egal_draw_point(GalDrawable drawable, GalPB pb, int x, int y)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_point(drawable, pb, x, y);
}

void egal_draw_line(GalDrawable drawable, GalPB pb, int x1, int y1, int x2, int y2)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_line(drawable, pb, x1, y1, x2, y2);
}

void egal_draw_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_rect(drawable, pb, x, y, w, h);
}

void egal_draw_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GAL_DRAWABLE_ORDERS(drawable)->draw_arc(drawable, pb, x, y, w, h, angle1, angle2);
}

void egal_fill_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GAL_DRAWABLE_ORDERS(drawable)->fill_rect(drawable, pb, x, y, w, h);
}

void egal_fill_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GAL_DRAWABLE_ORDERS(drawable)->fill_arc(drawable, pb, x, y, w, h, angle1, angle2);
}

GalColormap *egal_get_colormap(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->get_colormap(drawable);
}

GalVisual *egal_get_visual(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->get_visual(drawable);
}

eint egal_get_visual_info(GalDrawable drawable, GalVisualInfo *info)
{
	return GAL_DRAWABLE_ORDERS(drawable)->get_visual_info(drawable, info);
}

eint egal_drawable_get_mark(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->get_mark(drawable);
}

GalSurface egal_refer_surface(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->refer_surface(drawable);
}

ePointer egal_surface_private(GalDrawable drawable)
{
	return GAL_DRAWABLE_ORDERS(drawable)->refer_surface_private(drawable);
}

eGeneType egal_genetype(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {0, NULL, 0, NULL, NULL, NULL};
		gtype = e_register_genetype(&info, GTYPE_CELL, NULL);
	}
	return gtype;
}

eGeneType egal_genetype_pb(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GalPBOrders),
			NULL,
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

eGeneType egal_genetype_drawable(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GalDrawableOrders),
			NULL, 
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

eGeneType egal_genetype_window(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GalWindowOrders),
			NULL, 
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

eGeneType egal_genetype_cursor(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GalCursorOrders),
			NULL, 
			0,
			NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

GalWindow egal_root_window(void)
{
	return _gwm.root_window;
}

GalImage *egal_image_new(eint w, eint h, bool alpha)
{
	if (_gwm.image_new)
		return _gwm.image_new(w, h, alpha);
	return NULL;
}

void egal_image_free(GalImage *image)
{
	if (_gwm.image_free)
		_gwm.image_free(image);
}

GalDrawable egal_drawable_new(eint w, eint h, bool alpha)
{
	if (_gwm.drawable_new)
		return _gwm.drawable_new(w, h, alpha);
	return 0;
}

GalPB egal_pb_new(GalDrawable drawable, GalPBAttr *attr)
{
	if (_gwm.pb_new)
		return _gwm.pb_new(drawable, attr);
	return 0;
}

GalWindow egal_window_new(GalWindowAttr *attributes)
{
	if (_gwm.window_new)
		return _gwm.window_new(attributes);
	return 0;
}

GalCursor egal_cursor_new(GalCursorType type)
{
	if (_gwm.cursor_new)
		return _gwm.cursor_new(type);
	return 0;
}

GalCursor egal_cursor_new_name(const echar *name)
{
	if (_gwm.cursor_new_name)
		return _gwm.cursor_new_name(name);
	return 0;
}

GalCursor egal_cursor_new_pixbuf(GalPixbuf *pixbuf, eint x, eint y)
{
	if (_gwm.cursor_new_pixbuf)
		return _gwm.cursor_new_pixbuf(pixbuf, x, y);
	return 0;
}

GalWindow egal_window_wait_event(GalEvent *event, bool recv)
{
	if (_gwm.wait_event)
		return _gwm.wait_event(event, recv);
	return 0;
}

void egal_window_get_event(GalEvent *event)
{
	if (_gwm.get_event)
		_gwm.get_event(event);
}

#ifdef linux
int x11_wmananger_init(GalWindowManager *);
#elif WIN32
int w32_wmananger_init(GalWindowManager *);
#endif
eint egal_window_init(void)
{
#ifdef linux
	eint (*window_init)(GalWindowManager *) = x11_wmananger_init;
#elif WIN32
	eint (*window_init)(GalWindowManager *) = w32_wmananger_init;
#endif

	if (window_init && !window_init(&_gwm)) {
#ifdef linux
		eint i;
		for (i = 0; i < GalPBset+1; i++) {
			GalPBAttr attribute;
			attribute.func = i;
#ifdef _GAL_SUPPORT_CAIRO
			attribute.use_cairo = true;
#endif
			default_pbs[i] = egal_pb_new(0, &attribute);
		}
#endif
		return 0;
	}

	return -1;
}

GalPB egal_default_pb(void)
{
	return e_object_refer(default_pbs[GalPBcopy]);
}

GalPB egal_type_pb(GalPBFunc func)
{
	return e_object_refer(default_pbs[func]);
}
