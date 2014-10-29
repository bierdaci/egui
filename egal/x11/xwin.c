#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/extensions/composite.h>
#include <X11/extensions/Xrender.h>
#include <X11/Xcursor/Xcursor.h>

#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>

#include "window.h"

#ifdef _GAL_SUPPORT_OPENGL
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#endif

#define X11_PB_DATA(hobj)			((GalPBX11       *)e_object_type_data(hobj, x11_genetype_pb()))
#define X11_WINDOW_DATA(hobj)		((GalWindowX11   *)e_object_type_data(hobj, x11_genetype_window()))
#define X11_PIXMAP_DATA(hobj)		((GalPixmapX11   *)e_object_type_data(hobj, x11_genetype_pixmap()))
#define X11_CURSOR_DATA(hobj)		((GalCursorX11   *)e_object_type_data(hobj, x11_genetype_cursor()))
#define X11_DRAWABLE_DATA(hobj)		((GalDrawableX11 *)e_object_type_data(hobj, x11_genetype_drawable()))
#define X11_SURFACE_DATA(hobj)		((GalSurfaceX11  *)e_object_type_data(hobj, x11_genetype_surface()))

static eGeneType x11_genetype_pb(void);
static eGeneType x11_genetype_window(void);
static eGeneType x11_genetype_pixmap(void);
static eGeneType x11_genetype_cursor(void);
static eGeneType x11_genetype_drawable(void);
static eGeneType x11_genetype_surface(void);

static void x11_drawable_init(GalDrawable, Window, eint, eint, eint);
static GalWindow   x11_wait_event(GalEvent *);
static GalWindow   x11_window_new(GalWindowAttr *);
static GalDrawable x11_drawable_new(eint, eint, bool);
static GalImage   *x11_image_new(eint, eint, bool);
static GalPB x11_pb_new(GalDrawable, GalPBAttr *);
static GalCursor x11_cursor_new(GalCursorType);
static GalCursor x11_cursor_new_name(const echar *);
static GalCursor x11_cursor_new_pixbuf(GalPixbuf *, eint, eint);
static void x11_image_free(GalImage *);
static void x11_composite(GalDrawable, int, int, GalDrawable, int, int, int, int);
static void x11_composite_image(GalDrawable, int, int, GalImage *, int, int, int, int);
static void x11_composite_subwindow(GalWindow, int, int, int, int);
static void x11_draw_drawable(GalDrawable, GalPB, int, int, GalDrawable, int, int, int, int);

typedef struct _GalDrawableX11	GalDrawableX11;
typedef struct _GalSurfaceX11	GalSurfaceX11;
typedef struct _GalWindowX11	GalWindowX11;
typedef struct _GalPixmapX11	GalPixmapX11;
typedef struct _GalCursorX11	GalCursorX11;
typedef struct _GalImageX11		GalImageX11;
typedef struct _GalPBX11		GalPBX11;
typedef struct _GalVisualX11	GalVisualX11;
typedef struct _GalColormapX11	GalColormapX11;

struct _GalDrawableX11 {
	eint w, h;
	eint depth;

	Window  xid;
	Picture picture;

	GalScreen   *screen;
	GalColormap *colormap;
	GalVisual   *visual;

	GalSurface   surface;
};

struct _GalWindowX11 {
	GalWindowAttr attr;
	eint x, y;
	eint w, h;

	GalWindowX11 *parent;
	Window xid;
	GalCursor cursor;

	list_t list;
	list_t child_head;
	e_thread_mutex_t lock;

	eHandle attachmen;
};

struct _GalPixmapX11 {
};

struct _GalSurfaceX11 {
	cairo_t         *cr;
	cairo_surface_t *surface;
};

struct _GalImageX11 {
	GalImage gimg;
	XImage  *ximg;
	GalPB    pb;
	GalDrawable pixmap;
};

struct _GalPBX11 {
	GalDrawableX11 *xwin;
	XGCValues values;
	GC        gc;
	GalPBAttr attr;
};

struct _GalCursorX11 {
	Cursor xid;
	GalCursorType type;
	echar *name;
};

struct _GalVisualX11 {
	GalVisual visual;
	Visual *xvid;
};

struct _GalColormapX11 {
	GalColormap map;
	Colormap xmap;
};

#ifdef _GAL_SUPPORT_OPENGL
typedef struct _GalGLXContext GalGLXContext;
struct _GalGLXContext {
	Window  xid;
	GLXContext    context;
	GalWindowX11 *current;
};
static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None};
//static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE, 24, None};
static GalGLXContext *x11_glc;
#endif
static GalWindow      root_window;
static GalVisualX11   x11_visual;
static GalColormapX11 x11_cmap;
static GalWindowX11  *x11_root;
static XVisualInfo   *x11_vinfo;
static Display       *x11_dpy;
static eTree         *x11_tree;
static e_thread_mutex_t tree_lock;
static const cairo_user_data_key_t x11_cairo_key;
static struct {
	eint xkey;
	eint gkey;
} x11_keymap[] = {
	{XK_Shift_L,		GAL_KC_Shift_L},
	{XK_Shift_R,		GAL_KC_Shift_R},
	{XK_Control_L,		GAL_KC_Control_L},
	{XK_Control_R,		GAL_KC_Control_R},
	{XK_Caps_Lock,		GAL_KC_Caps_Lock},
	{XK_Shift_Lock,		GAL_KC_Shift_Lock},
	{XK_Meta_L,			GAL_KC_Meta_L},
	{XK_Meta_R,			GAL_KC_Meta_R},
	{XK_Alt_L,			GAL_KC_Alt_L},
	{XK_Alt_R,			GAL_KC_Alt_R},
	{XK_Super_L,		GAL_KC_Super_L},
	{XK_Super_R,		GAL_KC_Super_R},
	{XK_Hyper_L,		GAL_KC_Hyper_L},
	{XK_Hyper_R,		GAL_KC_Hyper_R},
	{XK_Scroll_Lock,	GAL_KC_Scroll_Lock},
	{XK_Num_Lock,		GAL_KC_Num_Lock},
	{XK_Sys_Req,		GAL_KC_Sys_Req},
	{XK_Escape,			GAL_KC_Escape},
	{XK_Delete,			GAL_KC_Delete},
	{XK_Home,			GAL_KC_Home},
	{XK_Left,			GAL_KC_Left},
	{XK_Up,				GAL_KC_Up},
	{XK_Right,			GAL_KC_Right},
	{XK_Down,			GAL_KC_Down},
	{XK_Page_Up,		GAL_KC_Page_Up},
	{XK_Page_Down,		GAL_KC_Page_Down},
	{XK_End,			GAL_KC_End},
	{XK_Begin,			GAL_KC_Begin},
	{XK_F1,				GAL_KC_F1},
	{XK_F2,				GAL_KC_F2},
	{XK_F3,				GAL_KC_F3},
	{XK_F4,				GAL_KC_F4},
	{XK_F5,				GAL_KC_F5},
	{XK_F6,				GAL_KC_F6},
	{XK_F7,				GAL_KC_F7},
	{XK_F8,				GAL_KC_F8},
	{XK_F9,				GAL_KC_F9},
	{XK_F10,			GAL_KC_F10},
	{XK_F11,			GAL_KC_F11},
	{XK_F12,			GAL_KC_F12},
	{XK_BackSpace,		GAL_KC_BackSpace},
	{XK_Tab,			GAL_KC_Tab},
	{XK_Return,			GAL_KC_Enter},
	{XK_space,			GAL_KC_space},
	{XK_exclam,			GAL_KC_exclam},
	{XK_quotedbl,		GAL_KC_quotedbl},
	{XK_numbersign,		GAL_KC_numbersign},
	{XK_dollar,			GAL_KC_dollar},
	{XK_percent,		GAL_KC_percent},
	{XK_ampersand,		GAL_KC_ampersand},
	{XK_apostrophe,		GAL_KC_apostrophe},
	{XK_parenleft,		GAL_KC_parenleft},
	{XK_parenright,		GAL_KC_parenright},
	{XK_asterisk,		GAL_KC_asterisk},
	{XK_plus,			GAL_KC_plus},
	{XK_comma,			GAL_KC_comma},
	{XK_minus,			GAL_KC_minus},
	{XK_period,			GAL_KC_period},
	{XK_slash,			GAL_KC_slash},
	{XK_0,				GAL_KC_0},
	{XK_1,				GAL_KC_1},
	{XK_2,				GAL_KC_2},
	{XK_3,				GAL_KC_3},
	{XK_4,				GAL_KC_4},
	{XK_5,				GAL_KC_5},
	{XK_6,				GAL_KC_6},
	{XK_7,				GAL_KC_7},
	{XK_8,				GAL_KC_8},
	{XK_9,				GAL_KC_9},
	{XK_colon,			GAL_KC_colon},
	{XK_semicolon,		GAL_KC_semicolon},
	{XK_less,			GAL_KC_less},
	{XK_equal,			GAL_KC_equal},
	{XK_greater,		GAL_KC_greater},
	{XK_question,		GAL_KC_question},
	{XK_at,				GAL_KC_at},
	{XK_A,				GAL_KC_A},
	{XK_B,				GAL_KC_B},
	{XK_C,				GAL_KC_C},
	{XK_D,				GAL_KC_D},
	{XK_E,				GAL_KC_E},
	{XK_F,				GAL_KC_F},
	{XK_G,				GAL_KC_G},
	{XK_H,				GAL_KC_H},
	{XK_I,				GAL_KC_I},
	{XK_J,				GAL_KC_J},
	{XK_K,				GAL_KC_K},
	{XK_L,				GAL_KC_L},
	{XK_M,				GAL_KC_M},
	{XK_N,				GAL_KC_N},
	{XK_O,				GAL_KC_O},
	{XK_P,				GAL_KC_P},
	{XK_Q,				GAL_KC_Q},
	{XK_R,				GAL_KC_R},
	{XK_S,				GAL_KC_S},
	{XK_T,				GAL_KC_T},
	{XK_U,				GAL_KC_U},
	{XK_V,				GAL_KC_V},
	{XK_W,				GAL_KC_W},
	{XK_X,				GAL_KC_X},
	{XK_Y,				GAL_KC_Y},
	{XK_Z,				GAL_KC_Z},
	{XK_bracketleft,	GAL_KC_bracketleft},
	{XK_backslash,		GAL_KC_backslash},
	{XK_bracketright,	GAL_KC_bracketright},
	{XK_asciicircum,	GAL_KC_asciicircum},
	{XK_underscore,		GAL_KC_underscore},
	{XK_grave,			GAL_KC_grave},
	{XK_a,				GAL_KC_a},
	{XK_b,				GAL_KC_b},
	{XK_c,				GAL_KC_c},
	{XK_d,				GAL_KC_d},
	{XK_e,				GAL_KC_e},
	{XK_f,				GAL_KC_f},
	{XK_g,				GAL_KC_g},
	{XK_h,				GAL_KC_h},
	{XK_i,				GAL_KC_i},
	{XK_j,				GAL_KC_j},
	{XK_k,				GAL_KC_k},
	{XK_l,				GAL_KC_l},
	{XK_m,				GAL_KC_m},
	{XK_n,				GAL_KC_n},
	{XK_o,				GAL_KC_o},
	{XK_p,				GAL_KC_p},
	{XK_q,				GAL_KC_q},
	{XK_r,				GAL_KC_r},
	{XK_s,				GAL_KC_s},
	{XK_t,				GAL_KC_t},
	{XK_u,				GAL_KC_u},
	{XK_v,				GAL_KC_v},
	{XK_w,				GAL_KC_w},
	{XK_x,				GAL_KC_x},
	{XK_y,				GAL_KC_y},
	{XK_z,				GAL_KC_z},
	{XK_braceleft,		GAL_KC_braceleft},
	{XK_bar,			GAL_KC_bar},
	{XK_braceright,		GAL_KC_braceright},
	{XK_asciitilde,		GAL_KC_asciitilde},
	{XK_ISO_Left_Tab,   GAL_KC_Tab},
};

static eint x11_compare_func(eConstPointer a, eConstPointer b)
{
	if ((elong)a > (elong)b) return  1;
	if ((elong)a < (elong)b) return -1;
	return 0;
}

static eint x11_window_get_attr(GalWindow, GalWindowAttr *);
int x11_wmananger_init(GalWindowManager *wm)
{
	GalWindowAttr attributes;
	GalDrawableX11 *xdraw;

#ifndef _GAL_SUPPORT_OPENGL
	int vnum;
	XVisualInfo   vinfo;
#endif

	XInitThreads();
	x11_tree = e_tree_new(x11_compare_func);
	e_thread_mutex_init(&tree_lock, NULL);

	if (!(x11_dpy = XOpenDisplay(0))) {
		printf("Could not open display [%s]\n", getenv("DISPLAY"));
		return -1;
	}
	XSynchronize(x11_dpy, False);

#ifdef _GAL_SUPPORT_OPENGL
	x11_vinfo = glXChooseVisual(x11_dpy, XDefaultScreen(x11_dpy), att);
	x11_glc = e_malloc(sizeof(GalGLXContext));
	x11_glc->context = glXCreateContext(x11_dpy, x11_vinfo, NULL, GL_TRUE);
#else
	vinfo.visualid = XVisualIDFromVisual(XDefaultVisual(x11_dpy, XDefaultScreen(x11_dpy)));
	x11_vinfo = XGetVisualInfo(x11_dpy, VisualIDMask, &vinfo, &vnum);
	if (vnum == 0) {
		printf("Bad visual id %d\n", (int)vinfo.visualid);
		return -1;
	}
#endif

	attributes.type = GalWindowRoot;
	root_window     = x11_window_new(&attributes);
	x11_root        = X11_WINDOW_DATA(root_window);
	x11_root->xid   = XRootWindow(x11_dpy, x11_vinfo->screen);
	xdraw = X11_DRAWABLE_DATA(root_window);
	xdraw->xid      = x11_root->xid;
	x11_cmap.xmap   = XCreateColormap(x11_dpy, x11_root->xid, x11_vinfo->visual, AllocNone);
	x11_visual.xvid = x11_vinfo->visual;
	//XCompositeRedirectSubwindows(x11_dpy, x11_root->xid, CompositeRedirectAutomatic);

	x11_window_get_attr(root_window, &attributes);
	x11_root->w = attributes.width;
	x11_root->h = attributes.height;
	xdraw->w    = attributes.width;
	xdraw->h    = attributes.height;

	if (x11_vinfo->class != TrueColor) {
		XFreeColormap(x11_dpy, x11_cmap.xmap);
		return -1;
	}

	wm->wait_event   = x11_wait_event;
	wm->window_new   = x11_window_new;
	wm->image_new    = x11_image_new;
	wm->image_free   = x11_image_free;
	wm->drawable_new = x11_drawable_new;
	wm->pb_new       = x11_pb_new;

	wm->cursor_new   = x11_cursor_new;
	wm->cursor_new_name   = x11_cursor_new_name;
	wm->cursor_new_pixbuf = x11_cursor_new_pixbuf;

	wm->root_window  = root_window;

	return 0;
}

static GalWindow x11_find_gwin(Window xwin)
{
	GalWindow window;

	e_thread_mutex_lock(&tree_lock);
	window = (GalWindow)e_tree_lookup(x11_tree, (eConstPointer)xwin);
	e_thread_mutex_unlock(&tree_lock);

	return window;
}

static int translate_key(XKeyEvent *event)
{
	KeySym keysym;
	char buf[100];
	int i;

	XLookupString(event, buf, 100, &keysym, 0);

	if (keysym > 0 && keysym <= GAL_KC_asciitilde)
		return keysym;

	for (i = 0; i < sizeof(x11_keymap); i++) {
		if (x11_keymap[i].xkey == keysym)
			return x11_keymap[i].gkey;
	}

	return -1;
}


static bool x11_predicate(Display *display, XEvent *event, GalEvent *ent)
{
	switch (event->type) {
		//case GraphicsExpose:
		//	return false;
		case Expose:
		{
			XExposeEvent *e = &event->xexpose;
			ent->type = GAL_ET_EXPOSE;
			ent->e.expose.rect.x = e->x;
			ent->e.expose.rect.y = e->y;
			ent->e.expose.rect.w = e->width;
			ent->e.expose.rect.h = e->height;
			return true;
		}
		case ConfigureNotify:
		{
			XConfigureEvent *e = &event->xconfigure;
			static eint old_w = -1;
			static eint old_h = -1;
			if (old_w == e->width && old_h == e->height)
				return false;
			old_w = e->width;
			old_h = e->height;
			ent->type = GAL_ET_CONFIGURE;
			ent->e.configure.rect.x = e->x;
			ent->e.configure.rect.y = e->y;
			ent->e.configure.rect.w = e->width;
			ent->e.configure.rect.h = e->height;
			return true;
		}
		case KeyPress:
		{
			XKeyEvent *e = &event->xkey;
			ent->type = GAL_ET_KEYDOWN;
			ent->e.key.code  = translate_key(e);
			ent->e.key.state = e->state;
			ent->e.key.time  = e->time;
			return true;
		}
		case KeyRelease:
		{
			XKeyEvent *e = &event->xkey;
			ent->type = GAL_ET_KEYUP;
			ent->e.key.code  = translate_key(e);
			ent->e.key.state = e->state;
			ent->e.key.time  = e->time;
			return true;
		}
		case ButtonPress:
		{
			XButtonEvent *e = &event->xbutton;
			if (e->button == Button1)
				ent->type = GAL_ET_LBUTTONDOWN;
			else if (e->button == Button2)
				ent->type = GAL_ET_MBUTTONDOWN;
			else if (e->button == Button3)
				ent->type = GAL_ET_RBUTTONDOWN;
			else if (e->button == Button4)
				ent->type = GAL_ET_WHEELFORWARD;
			else if (e->button == Button5)
				ent->type = GAL_ET_WHEELBACKWARD;
			ent->e.mouse.point.x = e->x;
			ent->e.mouse.point.y = e->y;
			ent->e.mouse.root_x  = e->x_root;
			ent->e.mouse.root_y  = e->y_root;
			ent->e.mouse.state   = e->state;
			ent->e.mouse.time    = e->time;
			return true;
		}
		case ButtonRelease:
		{
			XButtonEvent *e = &event->xbutton;
			if (e->button == Button1)
				ent->type = GAL_ET_LBUTTONUP;
			else if (e->button == Button2)
				ent->type = GAL_ET_MBUTTONUP;
			else if (e->button == Button3)
				ent->type = GAL_ET_RBUTTONUP;
			else if (e->button == Button4)
				return false;
			else if (e->button == Button5)
				return false;
				
			ent->e.mouse.point.x = e->x;
			ent->e.mouse.point.y = e->y;
			ent->e.mouse.root_x  = e->x_root;
			ent->e.mouse.root_y  = e->y_root;
			ent->e.mouse.state   = e->state;
			ent->e.mouse.time    = e->time;
			return true;
		}
		case MotionNotify:
		{
			XMotionEvent *e = &event->xmotion;
			ent->type = GAL_ET_MOUSEMOVE;
			ent->e.mouse.point.x = e->x;
			ent->e.mouse.point.y = e->y;
			ent->e.mouse.root_x  = e->x_root;
			ent->e.mouse.root_y  = e->y_root;
			ent->e.mouse.state   = e->state;
			return true;
		}
		case FocusIn:
		{
			ent->type = GAL_ET_FOCUS_IN;
			return true;
		}
		case FocusOut:
		{
			ent->type = GAL_ET_FOCUS_OUT;
			return true;
		}
		case EnterNotify:
			if (event->xcrossing.mode == NotifyNormal) {
				ent->type = GAL_ET_ENTER;
				return true;
			}
			return false;
		case LeaveNotify:
			if (event->xcrossing.mode == NotifyNormal) {
				ent->type = GAL_ET_LEAVE;
				return true;
			}
			return false;
		case ClientMessage:
			//XClientMessageEvent *e = &event->xclient;
			ent->type = GAL_ET_DESTROY;
			return true;
		case VisibilityNotify:
		case MapNotify:
		case UnmapNotify:
		case ReparentNotify:
		default:
			return false;
	}

	return false;
}

static GalWindow x11_wait_event(GalEvent *gent)
{
	XEvent xent;

#if 0
	XIfEvent(x11_dpy, &xent, x11_predicate, gent);
#else
	while (XPending(x11_dpy)) {
		XNextEvent(x11_dpy, &xent);
		if (x11_predicate(x11_dpy, &xent, gent)) {
			gent->window = x11_find_gwin(xent.xany.window);
			if (gent->window) {
#ifdef _GAL_SUPPORT_CAIRO
				if (gent->type == GAL_ET_CONFIGURE) {
					GalSurfaceX11 *xface = X11_SURFACE_DATA(X11_DRAWABLE_DATA(gent->window)->surface);
					cairo_xlib_surface_set_size(xface->surface, gent->e.configure.rect.w, gent->e.configure.rect.h);
				}
#endif
				return gent->window;
			}
		}
	}

	usleep(10000);
	gent->type   = GAL_ET_TIMEOUT;
	gent->window = root_window;
#endif
	return gent->window;
}

static bool x11_create_child_window(GalWindowX11 *);
static void x11_list_add(GalWindowX11 *parent, GalWindowX11 *child)
{
	e_thread_mutex_lock(&parent->lock);
	list_add_tail(&child->list, &parent->child_head);
	e_thread_mutex_unlock(&parent->lock);
}

static void x11_list_del(GalWindowX11 *xwin)
{
	GalWindowX11 *parent = xwin->parent;
	if (parent) {
		e_thread_mutex_lock(&parent->lock);
		list_del(&xwin->list);
		e_thread_mutex_unlock(&parent->lock);
	}
}

#define INPUT_MASK \
	(ButtonPressMask \
	 | ButtonReleaseMask | PointerMotionMask \
	 | ButtonMotionMask | EnterWindowMask  \
	 | LeaveWindowMask | KeyPressMask | KeyReleaseMask | FocusChangeMask)

#define EXPOSE_MASK (ExposureMask | VisibilityChangeMask)

static bool x11_create_window(GalWindowX11 *parent, GalWindowX11 *child)
{
	XSetWindowAttributes xattribs;
	GalWindow  window  = OBJECT_OFFSET(child);
	eulong xevent_mask = 0;
	euint  wclass = 0;;
	eint   depth  = 0;;

	e_memset(&xattribs, 0, sizeof(xattribs));

	xattribs.event_mask = StructureNotifyMask | PropertyChangeMask;

	if (child->attr.wa_mask & GAL_WA_NOREDIR)
		xevent_mask |= CWOverrideRedirect;

	if (child->attr.input_event) {
		xattribs.event_mask |= INPUT_MASK;
		xevent_mask         |= CWEventMask;
	}

	if (child->attr.event_mask) {
		xattribs.event_mask |= child->attr.event_mask;
		xevent_mask         |= CWEventMask;
	}

	if (child->attr.wclass == GAL_INPUT_OUTPUT) {
		wclass = InputOutput;
		depth  = x11_vinfo->depth;

		if (child->attr.output_event) {
			xattribs.event_mask |= EXPOSE_MASK;
			xevent_mask         |= CWEventMask;
		}

		if (child->attr.type == GalWindowTop) {
			//xattribs.border_pixel = XWhitePixel(x11_dpy, x11_vinfo->screen);
			//xevent_mask          |= CWBorderPixel;
		}
		else if (child->attr.type == GalWindowDialog) {
		}
		else if (child->attr.type == GalWindowTemp) {
			xattribs.save_under   = true;
			xattribs.cursor       = None;
			xevent_mask          |= CWSaveUnder | CWOverrideRedirect;
			xattribs.override_redirect = true;
		}
		else if (child->attr.type == GalWindowChild) {
		}

		//xattribs.bit_gravity = StaticGravity;
		xattribs.colormap    = x11_cmap.xmap;

		//xevent_mask |= CWBackPixel;
		//xattribs.background_pixel = child->attr.bg_color;
		//xattribs.background_pixel = XBlackPixel(x11_dpy, x11_vinfo->screen);
#ifdef _GAL_SUPPORT_OPENGL
		xevent_mask |= CWColormap;
#endif
	}
	else if (child->attr.wclass == GAL_INPUT_ONLY) {
		wclass       = InputOnly;
		depth        = 0;
		xevent_mask |= CWOverrideRedirect;
		xattribs.event_mask &= ~EXPOSE_MASK;
		xattribs.override_redirect = true;
	}

	child->xid = XCreateWindow(x11_dpy,
			parent->xid,
			child->x, child->y,
			child->w, child->h,
			0,
			depth,
			wclass,
			x11_vinfo->visual,
			xevent_mask,
			&xattribs);

#ifdef _GAL_SUPPORT_OPENGL
	if (x11_glc->current == child) {
		glXMakeCurrent(x11_dpy, child->xid, x11_glc->context);
		x11_glc->xid = child->xid;
	}
#endif

	if (child->attr.type == GalWindowTop || child->attr.type == GalWindowDialog) {
		Atom protocols = XInternAtom(x11_dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(x11_dpy, child->xid, &protocols, 1);
	}

	//XStoreName(x11_dpy, child->xid, "egui");

	e_thread_mutex_lock(&tree_lock);
	e_tree_insert(x11_tree, (ePointer)child->xid, (ePointer)window);
	e_thread_mutex_unlock(&tree_lock);

	x11_create_child_window(child);

	if (child->attr.visible)
		XMapWindow(x11_dpy, child->xid);

	if (wclass == InputOutput)
		x11_drawable_init(window, child->xid, child->w, child->h, x11_vinfo->depth);

	if (child->attr.type == GalWindowDialog)
		XSetTransientForHint(x11_dpy, child->xid, parent->xid);

	return true;
}

static bool x11_create_child_window(GalWindowX11 *parent)
{
	list_t *pos;

	list_for_each(pos, &parent->child_head) {
		GalWindowX11 *child = list_entry(pos, GalWindowX11, list);
		x11_create_window(parent, child);
	}
	return true;
}

static void x11_destory_window(GalWindowX11 *xwin)
{
	//list_t *pos;

	x11_list_del(xwin);

	//list_for_each(pos, &xwin->child_head) {
	//	GalWindowX11 *child = list_entry(pos, GalWindowX11, list);
	//	x11_destory_window(child);
	//}

	e_thread_mutex_lock(&tree_lock);
	e_tree_remove(x11_tree, (ePointer)xwin->xid);
	e_thread_mutex_unlock(&tree_lock);

	if (xwin->xid)
		XDestroyWindow(x11_dpy, xwin->xid);
}

static eint x11_window_put(GalWindow win1, GalWindow win2, eint x, eint y)
{
	GalWindowX11 *parent = X11_WINDOW_DATA(win1);
	GalWindowX11 *child  = X11_WINDOW_DATA(win2);

	if (child->parent)
		return 0;

	child->x = x;
	child->y = y;
	child->parent = parent;
	x11_list_add(parent, child);
	if (parent->xid != 0)
		x11_create_window(parent, child);
	return 0;
}

static eint x11_window_show(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XMapWindow(x11_dpy, xwin->xid);
	return 0;
}

static eint x11_window_hide(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XUnmapWindow(x11_dpy, xwin->xid);
	return 0;
}

static eint x11_window_move(GalWindow window, eint x, eint y)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	xwin->x = x;
	xwin->y = y;
	if (xwin->xid) {
		return XMoveWindow(x11_dpy, xwin->xid, x, y);
	}
	return -1;
}

static eint x11_window_rasie(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XRaiseWindow(x11_dpy, xwin->xid);
	return 0;
}

static eint x11_window_lower(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XLowerWindow(x11_dpy, xwin->xid);
	return 0;
}

static eint x11_window_remove(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XUnmapWindow(x11_dpy, xwin->xid);
	return 0;
}

static eint x11_window_configure(GalWindow window, GalRect *rect)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	XWindowChanges ch;
	eint retval = 0;

	if (rect->w == 0 || rect->h == 0)
		return -1;

	if (xwin->xid) {
		ch.x      = rect->x;
		ch.y      = rect->y;
		ch.width  = rect->w;
		ch.height = rect->h;
		retval = XConfigureWindow(x11_dpy, xwin->xid, 0, &ch);
	}
	xwin->x   = rect->x;
	xwin->y   = rect->y;
	xwin->w   = rect->w;
	xwin->h   = rect->h;
	return retval;
}

static eint x11_window_resize(GalWindow window, eint w, eint h)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	eint retval = 0;

	if (w == 0 || h == 0)
		return -1;

	if (xwin->xid) {
#ifdef _GAL_SUPPORT_CAIRO
		GalSurfaceX11 *xface =
			X11_SURFACE_DATA(X11_DRAWABLE_DATA(window)->surface);
		cairo_xlib_surface_set_size(xface->surface, w, h);
#endif
		retval = XResizeWindow(x11_dpy, xwin->xid, w, h);
	}
	xwin->w = w;
	xwin->h = h;
	return retval;
}

static eint x11_window_move_resize(GalWindow window, eint x, eint y, eint w, eint h)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	eint retval = 0;
	if (xwin->xid) {
#ifdef _GAL_SUPPORT_CAIRO
		GalSurfaceX11 *xface =
			X11_SURFACE_DATA(X11_DRAWABLE_DATA(window)->surface);
		cairo_xlib_surface_set_size(xface->surface, w, h);
#endif
		retval = XMoveResizeWindow(x11_dpy, xwin->xid, x, y, w, h);
	}
	xwin->x = x;
	xwin->y = y;
	xwin->w = w;
	xwin->h = h;
	return retval;
}

static void x11_window_get_geometry_hints(GalWindow window, GalGeometry *geometry, GalWindowHints *geom_mask)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	XSizeHints   *size_hints;  
	elong junk_supplied_mask = 0;

	*geom_mask = 0;

	if (!(size_hints = XAllocSizeHints()))
		return;

	if (!XGetWMNormalHints(x11_dpy, xwin->xid, size_hints, &junk_supplied_mask))
		size_hints->flags = 0;

	if (size_hints->flags & PMinSize) {
		*geom_mask |= GAL_HINT_MIN_SIZE;

		geometry->min_width  = size_hints->min_width;
		geometry->min_height = size_hints->min_height;
	}

	if (size_hints->flags & PMaxSize) {
		*geom_mask |= GAL_HINT_MAX_SIZE;

		geometry->max_width  = MAX(size_hints->max_width, 1);
		geometry->max_height = MAX(size_hints->max_height, 1);
	}

	if (size_hints->flags & PResizeInc) {
		*geom_mask |= GAL_HINT_RESIZE_INC;

		geometry->width_inc  = size_hints->width_inc;
		geometry->height_inc = size_hints->height_inc;
	}

	if (size_hints->flags & PAspect) {
		*geom_mask |= GAL_HINT_ASPECT;

		geometry->min_aspect = (edouble)size_hints->min_aspect.x / (edouble)size_hints->min_aspect.y;
		geometry->max_aspect = (edouble)size_hints->max_aspect.x / (edouble)size_hints->max_aspect.y;
	}

	if (size_hints->flags & PWinGravity) {
		*geom_mask |= GAL_HINT_WIN_GRAVITY;

		geometry->win_gravity = size_hints->win_gravity;
	}

	XFree(size_hints);
}

static eint x11_get_origin(GalWindow window, eint *x, eint *y)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);

	eint retval;
	Window child;
	eint tx = 0;
	eint ty = 0;

	retval = XTranslateCoordinates(x11_dpy,
                xwin->xid, x11_root->xid,
                0, 0, &tx, &ty,
                &child);

	if (x) *x = tx;
	if (y) *y = ty;

	return retval;
}

static void x11_window_set_geometry_hints(GalWindow window, GalGeometry *geometry, GalWindowHints geom_mask)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	XSizeHints size_hints;

	//if (GAL_WINDOW_DESTROYED(window))
	//	return;

	if (xwin->xid == 0)
		return;

	size_hints.flags = 0;

	if (geom_mask & GAL_HINT_POS) {
		size_hints.flags |= PPosition;
		size_hints.x = 0;
		size_hints.y = 0;
	}

	if (geom_mask & GAL_HINT_USER_POS)
		size_hints.flags |= USPosition;

	if (geom_mask & GAL_HINT_USER_SIZE)
		size_hints.flags |= USSize;

	if (geom_mask & GAL_HINT_MIN_SIZE) {
		size_hints.flags     |= PMinSize;
		size_hints.min_width  = geometry->min_width;
		size_hints.min_height = geometry->min_height;
	}

	if (geom_mask & GAL_HINT_MAX_SIZE) {
		size_hints.flags     |= PMaxSize;
		size_hints.max_width  = MAX(geometry->max_width, 1);
		size_hints.max_height = MAX(geometry->max_height, 1);
	}

	if (geom_mask & GAL_HINT_BASE_SIZE) {
		size_hints.flags      |= PBaseSize;
		size_hints.base_width  = geometry->base_width;
		size_hints.base_height = geometry->base_height;
	}

	if (geom_mask & GAL_HINT_RESIZE_INC) {
		size_hints.flags     |= PResizeInc;
		size_hints.width_inc  = geometry->width_inc;
		size_hints.height_inc = geometry->height_inc;
	}

	if (geom_mask & GAL_HINT_ASPECT) {
		size_hints.flags |= PAspect;
		if (geometry->min_aspect <= 1) {
			size_hints.min_aspect.x = 65536 * geometry->min_aspect;
			size_hints.min_aspect.y = 65536;
		}
		else {
			size_hints.min_aspect.x = 65536;
			size_hints.min_aspect.y = 65536 / geometry->min_aspect;;
		}
		if (geometry->max_aspect <= 1) {
			size_hints.max_aspect.x = 65536 * geometry->max_aspect;
			size_hints.max_aspect.y = 65536;
		}
		else {
			size_hints.max_aspect.x = 65536;
			size_hints.max_aspect.y = 65536 / geometry->max_aspect;;
		}
	}

	if (geom_mask & GAL_HINT_WIN_GRAVITY) {
		size_hints.flags      |= PWinGravity;
		size_hints.win_gravity = geometry->win_gravity;
	}

	XSetWMNormalHints(x11_dpy, xwin->xid, &size_hints);
}

static void x11_warp_pointer(GalWindow window, int sx, int sy, int sw, int sh, int dx, int dy)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	if (xwin->xid)
		XWarpPointer(x11_dpy, None, xwin->xid, sx, sy, sw, sh, dx, dy);
}

#define MOUSE_MASK  (ButtonPressMask \
	 | ButtonReleaseMask | PointerMotionMask \
	 | ButtonMotionMask | EnterWindowMask  \
	 | LeaveWindowMask) 

static void x11_get_pointer(GalWindow window,
		eint *root_x, eint *root_y,
		eint *win_x, eint *win_y, GalModifierType *mask)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	Window root = None;
	Window child;
	int rootx, rooty;
	int winx;
	int winy;
	unsigned int xmask;

	 XQueryPointer(x11_dpy, xwin->xid,
			 &root, &child, &rootx, &rooty, &winx, &winy, &xmask);
	 if (win_x)  *win_x  = winx;
	 if (win_y)  *win_y  = winy;
	 if (root_x) *root_x = rootx;
	 if (root_y) *root_y = rooty;
	 if (mask)   *mask = xmask;
}

static GalGrabStatus x11_grab_pointer(GalWindow window, bool owner_events, GalCursor cursor)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	Cursor xcursor = None;
	if (cursor)
		xcursor = X11_CURSOR_DATA(cursor)->xid;

	return XGrabPointer(x11_dpy,
			xwin->xid,
			owner_events, MOUSE_MASK,
			GrabModeAsync, GrabModeAsync,
			None, xcursor, 0);
}

static GalGrabStatus x11_ungrab_pointer(GalWindow window)
{
	return XUngrabPointer(x11_dpy, 0);
}

static GalGrabStatus x11_grab_keyboard(GalWindow window, bool owner_events)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);

	return XGrabKeyboard(x11_dpy, xwin->xid,
			owner_events, GrabModeAsync, GrabModeAsync, 0);
}

static GalGrabStatus x11_ungrab_keyboard(GalWindow window)
{
	return XUngrabKeyboard(x11_dpy, 0);
}

static INLINE bool xpb_init_gc(GalPBX11 *xpb)
{
	if (xpb->xwin->xid == 0)
		return false;
	if (xpb->gc == 0)
		xpb->gc = XCreateGC(x11_dpy, xpb->xwin->xid, GCGraphicsExposures | GCFunction, &xpb->values);
	return true;
}

static void x11_draw_drawable(GalDrawable dst,
		GalPB pb, int x, int y,
		GalDrawable src, int sx, int sy, int w, int h)
{
	GalDrawableX11 *xsrc = X11_DRAWABLE_DATA(src);
	GalDrawableX11 *xdst = X11_DRAWABLE_DATA(dst);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	eint  src_depth = egal_get_depth(src);
	eint dest_depth = egal_get_depth(dst);

	if (xsrc->xid && xdst->xid && src_depth == dest_depth && xpb_init_gc(xpb)) {
		XCopyArea(x11_dpy, xsrc->xid, xdst->xid, xpb->gc, sx, sy, w, h, x, y);
		XSync(x11_dpy, False);
	}
}

static void x11_draw_image(GalDrawable drawable,
		GalPB pb, int x, int y,
		GalImage *gimg, int bx, int by, int w, int h)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	GalImageX11    *ximg = (GalImageX11 *)gimg;

	if (draw->xid && xpb_init_gc(xpb)) {
		XPutImage(x11_dpy, draw->xid, xpb->gc, ximg->ximg, bx, by, x, y, w, h);
		XSync(x11_dpy, False);
	}
}

static void x11_draw_point(GalDrawable drawable, GalPB pb, int x, int y)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);

	if (draw->xid && xpb_init_gc(xpb)) {
//#ifdef _GAL_SUPPORT_CAIRO
//		GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
//		efloat b = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
//		efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
//		efloat r = ((efloat)((xpb->attr.foreground & 0xff0000) >> 16)) / 255;
//		cairo_set_source_rgb(surface->cr, r, g, b);
//#else
		XDrawPoint(x11_dpy, draw->xid, xpb->gc, x, y);
		XSync(x11_dpy, False);
//#endif
	}
}

static void x11_draw_line(GalDrawable drawable, GalPB pb, int x1, int y1, int x2, int y2)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);

	if (draw->xid && xpb_init_gc(xpb)) {
#ifdef _GAL_SUPPORT_CAIRO
		if (xpb->attr.func == GalPBcopy) {
			GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
			efloat b = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
			efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
			efloat r = ((efloat)((xpb->attr.foreground & 0xff0000) >> 16)) / 255;
			cairo_set_source_rgb(surface->cr, r, g, b);
			cairo_move_to(surface->cr, x1, y1);
			cairo_line_to(surface->cr, x2, y2);
			cairo_stroke(surface->cr);
		}
		else {
			XDrawLine(x11_dpy, draw->xid, xpb->gc, x1, y1, x2, y2);
			XSync(x11_dpy, False);
		}
#else
		XDrawLine(x11_dpy, draw->xid, xpb->gc, x1, y1, x2, y2);
		XSync(x11_dpy, False);
#endif
	}
}

static void x11_draw_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	if (draw->xid && xpb_init_gc(xpb)) {
#ifdef _GAL_SUPPORT_CAIRO
		if (xpb->attr.func == GalPBcopy) {
			GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
			efloat b = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
			efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
			efloat r = ((efloat)((xpb->attr.foreground & 0xff0000) >> 16)) / 255;
			cairo_set_source_rgb(surface->cr, r, g, b);
			cairo_rectangle(surface->cr, x, y, w, h);
			cairo_stroke(surface->cr);
		}
		else {
			XDrawRectangle(x11_dpy, draw->xid, xpb->gc, x, y, w, h);
			XSync(x11_dpy, False);
		}
#else
		XDrawRectangle(x11_dpy, draw->xid, xpb->gc, x, y, w, h);
		XSync(x11_dpy, False);
#endif
	}
}

static void x11_draw_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	if (draw->xid && xpb_init_gc(xpb)) {
#ifdef _GAL_SUPPORT_CAIRO
  	if (xpb->attr.func == GalPBcopy) {
		GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
		efloat b = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
		efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
		efloat r = ((efloat)((xpb->attr.foreground & 0xff000) >> 16)) / 255;
		if (draw->depth == 32) {
			efloat a = ((efloat)((xpb->attr.foreground & 0xff000000) >> 24)) / 255;
			cairo_set_source_rgba(surface->cr, r, g, b, a);
		}
		else
			cairo_set_source_rgb(surface->cr, r, g, b);
		cairo_arc(surface->cr, x, y, (efloat)w / (efloat)h, angle1, angle2);
		cairo_stroke(surface->cr);
	}
	else {
		XDrawArc(x11_dpy, draw->xid, xpb->gc, x, y, w, h, angle1, angle2);
		XSync(x11_dpy, False);
	}
#else
  	XDrawArc(x11_dpy, draw->xid, xpb->gc, x, y, w, h, angle1, angle2);
  	XSync(x11_dpy, False);
#endif
	}
}

static void x11_fill_rect(GalDrawable drawable, GalPB pb, int x, int y, int w, int h)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	if (draw->xid && xpb_init_gc(xpb)) {
#ifdef _GAL_SUPPORT_CAIRO
		if (xpb->attr.use_cairo) {
			GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
			efloat b = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
			efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
			efloat r = ((efloat)((xpb->attr.foreground & 0xff0000) >> 16)) / 255;
			if (draw->depth == 32) {
				efloat a = ((efloat)((xpb->attr.foreground & 0xff000000) >> 24)) / 255;
				cairo_set_source_rgba(surface->cr, r, g, b, a);
			}
			else
				cairo_set_source_rgb(surface->cr, r, g, b);
			cairo_rectangle(surface->cr, x, y, w, h);
			cairo_fill(surface->cr);
		}
		else {
			XFillRectangle(x11_dpy, draw->xid, xpb->gc, x, y, w, h);
			XSync(x11_dpy, False);
		}
#else
		XFillRectangle(x11_dpy, draw->xid, xpb->gc, x, y, w, h);
		XSync(x11_dpy, False);
#endif
	}
}

static void x11_fill_arc(GalDrawable drawable, GalPB pb, int x, int y, euint w, euint h, int angle1, int angle2)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	if (draw->xid && xpb_init_gc(xpb)) {
#ifdef _GAL_SUPPORT_CAIRO
		if (xpb->attr.use_cairo) {
			GalSurfaceX11 *surface = X11_SURFACE_DATA(draw->surface);
			efloat r = ((efloat)(xpb->attr.foreground & 0xff)) / 255;
			efloat g = ((efloat)((xpb->attr.foreground & 0xff00) >> 8)) / 255;
			efloat b = ((efloat)((xpb->attr.foreground & 0xff0000) >> 16)) / 255;
			if (draw->depth == 32) {
				efloat a = ((efloat)((xpb->attr.foreground & 0xff000000) >> 24)) / 255;
				cairo_set_source_rgba(surface->cr, r, g, b, a);
			}
			else
				cairo_set_source_rgb(surface->cr, r, g, b);
			cairo_arc(surface->cr, x, y, (efloat)w / (efloat)h, angle1, angle2);
			cairo_fill(surface->cr);
		}
		else {
			XFillArc(x11_dpy, draw->xid, xpb->gc, x, y, w, h, angle1, angle2);
			XSync(x11_dpy, False);
		}
#else
		XFillArc(x11_dpy, draw->xid, xpb->gc, x, y, w, h, angle1, angle2);
		XSync(x11_dpy, False);
#endif
	}
}

/*
static int x11_fill_polygon(GalDrawable drawable, GapPB pb, GalPoint *points, int npoints, int shape, int mode)
{
	GalDrawableX11 *draw = X11_DRAWABLE_DATA(drawable);
	GalPBX11       *xpb  = X11_PB_DATA(pb);
	XFillPolygon(x11_dpy, draw->xid, xpb->gc, points, npoints, shape, mode);
}
*/

static void x11_set_attachmen(GalWindow window, eHandle obj)
{
	X11_WINDOW_DATA(window)->attachmen = obj;
}

static eHandle x11_get_attachmen(GalWindow window)
{
	return X11_WINDOW_DATA(window)->attachmen;
}

static eint x11_window_set_cursor(GalWindow window, GalCursor cursor)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	xwin->cursor = cursor;

	if (cursor)
		return XDefineCursor(x11_dpy, xwin->xid, X11_CURSOR_DATA(cursor)->xid);
	
	return XUndefineCursor(x11_dpy, xwin->xid);
}

static GalCursor x11_window_get_cursor(GalWindow window)
{
	return X11_WINDOW_DATA(window)->cursor;
}

static eint x11_window_set_attr(GalWindow window, GalWindowAttr *attr)
{
	//GalWindowX11 *xwin = X11_WINDOW_DATA(window);

	//XSetWindowAttributes
	//XWindowAttributes xattr;
	//XChangeWindowAttributes(x11_dpy, xwin->xid, &xattr);
	return 0;
}

static eint x11_window_get_attr(GalWindow window, GalWindowAttr *attr)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);

	XWindowAttributes xattr;
	XGetWindowAttributes(x11_dpy, xwin->xid, &xattr);
	*attr = xwin->attr;
	attr->x = xattr.x;
	attr->y = xattr.y;
	attr->width  = xattr.width;
	attr->height = xattr.height;
	attr->depth  = xattr.depth;
	return 0;
}

static int x11_set_foreground(GalPB pb, eulong color)
{
	GalPBX11 *xpb = X11_PB_DATA(pb);
	xpb->attr.foreground = color;
	if (xpb_init_gc(xpb))
		return XSetForeground(x11_dpy, xpb->gc, color);
	return -1;
}

static int x11_set_background(GalPB pb, eulong color)
{
	GalPBX11 *xpb = X11_PB_DATA(pb);
	xpb->attr.background = color;
	if (xpb_init_gc(xpb))
		return XSetBackground(x11_dpy, xpb->gc, color);
	return -1;
}

static eulong x11_get_foreground(GalPB pb)
{
	GalPBX11 *xpb = X11_PB_DATA(pb);
#if 0
	XGCValues values;
	XGetGCValues(x11_dpy, xpb->gc, GCForeground, &values);
	return values.foreground;
#else
	return xpb->attr.foreground;
#endif
}

static eulong x11_get_background(GalPB pb)
{
	GalPBX11 *xpb = X11_PB_DATA(pb);
#if 0
	XGCValues values;
	XGetGCValues(x11_dpy, xpb->gc, GCBackground, &values);
	return values.background;
#else
	return xpb->attr.background;
#endif
}

/*
static int x11_set_font(GalPB *pb, GalFont *font)
{
	GalPBX11 *xpb = (GalPBX11 *)X11_PB_DATA(pb);
	return XSetFont(x11_dpy, xpb->gc, font);
}
*/

#ifdef _GAL_SUPPORT_OPENGL
static void x11_window_make_GL(GalWindow window)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);
	x11_glc->current   = xwin;
	if (xwin->xid) {
		glXMakeCurrent(x11_dpy, xwin->xid, x11_glc->context);
		x11_glc->xid = xwin->xid;
	}
}
#endif

static eint x11_window_set_name(GalWindow window, const echar *name)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(window);

	if (xwin->xid)
		return XStoreName(x11_dpy, xwin->xid, (char *)name);

	return -1;
}

static void x11_window_init_orders(eGeneType new, ePointer this)
{
	GalWindowOrders *win = e_genetype_orders(new, GTYPE_GAL_WINDOW);

	win->window_put    = x11_window_put;
	win->window_show   = x11_window_show;
	win->window_hide   = x11_window_hide;
	win->window_move   = x11_window_move;
	win->window_raise  = x11_window_rasie;
	win->window_lower  = x11_window_lower;
	win->window_remove = x11_window_remove;
	win->window_resize = x11_window_resize;
	win->window_configure   = x11_window_configure;
	win->window_move_resize = x11_window_move_resize;
	win->set_geometry_hints = x11_window_set_geometry_hints;
	win->get_geometry_hints = x11_window_get_geometry_hints;

	win->set_attachment = x11_set_attachmen;
	win->get_attachment = x11_get_attachmen;

	win->set_cursor = x11_window_set_cursor;
	win->get_cursor = x11_window_get_cursor;

	win->set_attr = x11_window_set_attr;
	win->get_attr = x11_window_get_attr;

	win->grab_keyboard   = x11_grab_keyboard;
	win->ungrab_keyboard = x11_ungrab_keyboard;

	win->get_origin      = x11_get_origin;
	win->get_pointer     = x11_get_pointer;
	win->grab_pointer    = x11_grab_pointer;
	win->ungrab_pointer  = x11_ungrab_pointer;
	win->warp_pointer    = x11_warp_pointer;
	//win->set_font        = x11_set_font;

	win->set_name        = x11_window_set_name;
#ifdef _GAL_SUPPORT_OPENGL
	win->make_GL         = x11_window_make_GL;
#endif
}

static void x11_window_free(eHandle hobj, ePointer data)
{
	x11_destory_window(X11_WINDOW_DATA(hobj));
}

static eGeneType x11_genetype_window(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			x11_window_init_orders,
			sizeof(GalWindowX11),
			NULL,
			x11_window_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_WINDOW, x11_genetype_drawable(), NULL);
	}
	return gtype;
}

static eint x11_window_init(eHandle hobj, GalWindowAttr *attr)
{
	GalWindowX11 *xwin = X11_WINDOW_DATA(hobj);

	xwin->attr   = *attr;
	xwin->xid    = 0;
	xwin->parent = NULL;
	if (xwin->attr.type != GalWindowRoot) {
		xwin->w  = xwin->attr.width;
		xwin->h  = xwin->attr.height;
	}
	INIT_LIST_HEAD(&xwin->child_head);
	e_thread_mutex_init(&xwin->lock, NULL);

	if (xwin->attr.type == GalWindowTop
			|| xwin->attr.type == GalWindowDialog
			|| xwin->attr.type == GalWindowTemp) {
		if (!x11_create_window(x11_root, xwin))
			return -1;
		x11_list_add(x11_root, xwin);
	}

	return 0;
}

static GalWindow x11_window_new(GalWindowAttr *attr)
{
	GalWindow window = e_object_new(x11_genetype_window());
	x11_window_init(window, attr);
	return window;
}

static GalImage *x11_image_new(eint w, eint h, bool alpha)
{
	GalImageX11 *ximg = e_malloc(sizeof(GalImageX11));
	GalImage    *gimg = (GalImage *)ximg;
	eint  depth;

	gimg->w = w;
	gimg->h = h;

	if (alpha)
		depth = 32;
	else
		depth = x11_vinfo->depth;

	if ((gimg->pixelbytes = depth / 8) == 3)
		gimg->pixelbytes = 4;

	ximg->ximg = XCreateImage(x11_dpy, x11_vinfo->visual, depth,
			ZPixmap, 0, NULL, gimg->w, gimg->h, gimg->pixelbytes * 8, 0);
	if (!ximg->ximg)
		goto err;

	ximg->ximg->data = e_malloc((((gimg->w * gimg->pixelbytes + 7) & ~7) * gimg->h));
	ximg->pb         = 0;
	ximg->pixmap     = 0;

	gimg->alpha      = alpha;
	gimg->pixels     = (euint8 *)ximg->ximg->data;
	gimg->depth      = gimg->pixelbytes * 8;
	gimg->rowbytes   = ximg->ximg->bytes_per_line;

	return gimg;
err:
	e_free(gimg);
	return NULL;
}

static void x11_image_free(GalImage *image)
{
	GalImageX11 *ximg = (GalImageX11 *)image;
	e_free(ximg->ximg->data);
	ximg->ximg->data = NULL;
	XDestroyImage(ximg->ximg);
	e_free(image);
}

static void x11_pixmap_init(GalDrawable draw, eint w, eint h, bool alpha)
{
	Window xid;
	eint depth;

	if (alpha)
		depth = 32;
	else
		depth = 24;

	xid = XCreatePixmap(x11_dpy, x11_root->xid, w, h, depth);

	x11_drawable_init(draw, xid, w, h, depth);
}

static void x11_pixmap_free(eHandle pixmap, ePointer data)
{
	GalDrawableX11 *xdraw = X11_DRAWABLE_DATA(pixmap);
	//e_object_unref(xdraw->surface);
	XFreePixmap(x11_dpy, xdraw->xid);
}

static eGeneType x11_genetype_pixmap(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			NULL,
			0,
			NULL,
			x11_pixmap_free,
			NULL,
		};

		gtype = e_register_genetype(&info, x11_genetype_drawable(), NULL);
	}
	return gtype;
}

static GalDrawable x11_drawable_new(eint w, eint h, bool alpha)
{
	GalDrawable pixmap = e_object_new(x11_genetype_pixmap());
	x11_pixmap_init(pixmap, w, h, alpha);
	return pixmap;
}

static int x11_func_convert(GalPBFunc func)
{
	switch (func) {
		case GalPBclear:		return GXclear;
		case GalPBand:			return GXand;
		case GalPBandReverse:	return GXandReverse;
		case GalPBcopy:			return GXcopy;
		case GalPBandInverted:	return GXandInverted;
		case GalPBnoop:			return GXnoop;
		case GalPBxor:			return GXxor;
		case GalPBor:			return GXor;
		case GalPBnor:			return GXnor;
		case GalPBequiv:		return GXequiv;
		case GalPBinvert:		return GXinvert;
		case GalPBorReverse:	return GXorReverse;
		case GalPBcopyInverted:	return GXcopyInverted;
		case GalPBorInverted:	return GXorInverted;
		case GalPBnand:			return GXnand;
		case GalPBset:			return GXset;
	}
	return -1;
}

static eint x11_pb_init(eHandle hobj, GalDrawable drawable, GalPBAttr *attr)
{
	GalPBX11 *xpb = X11_PB_DATA(hobj);

	if (!drawable)
		xpb->xwin = X11_DRAWABLE_DATA(root_window);
	else
		xpb->xwin = X11_DRAWABLE_DATA(drawable);

	if (attr) {
		xpb->values.function   = x11_func_convert(attr->func);
		xpb->values.foreground = attr->foreground;
		xpb->values.background = attr->background;
#ifdef _GAL_SUPPORT_CAIRO
		xpb->attr.use_cairo    = attr->use_cairo;
#endif
		xpb->attr.func         = attr->func;
	}
	else {
		xpb->attr.func = GalPBcopy;
		xpb->values.function = GXcopy;
		xpb->values.foreground = 0;
		xpb->values.background = 0;
	}
	xpb->values.graphics_exposures = False;
	xpb->attr.foreground = xpb->values.foreground;
	xpb->attr.background = xpb->values.background;

	if (xpb->xwin->xid != 0)
		xpb->gc = XCreateGC(x11_dpy, xpb->xwin->xid, GCGraphicsExposures | GCFunction, &xpb->values);

	return 0;
}

static void x11_pb_free(eHandle hobj, ePointer data)
{
	GalPBX11 *pb = data;
	if (pb->gc)
		XFreeGC(x11_dpy, pb->gc);
}

static void x11_pb_init_orders(eGeneType new, ePointer this)
{
	GalPBOrders *pb = e_genetype_orders(new, GTYPE_GAL_PB);
	pb->set_foreground = x11_set_foreground;
	pb->set_background = x11_set_background;
	pb->get_foreground = x11_get_foreground;
	pb->get_background = x11_get_background;
}

static eGeneType x11_genetype_pb(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			x11_pb_init_orders,
			sizeof(GalPBX11),
			NULL,
			x11_pb_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_PB, NULL);
	}
	return gtype;
}

static GalPB x11_pb_new(GalDrawable drawable, GalPBAttr *attr)
{
	GalPB pb = e_object_new(x11_genetype_pb()); 
	x11_pb_init(pb, drawable, attr);
	return pb;
}

#include "pixbuf.h"

static XRenderPictFormat *GetRenderRGB24Format(Display *dpy)
{
	static XRenderPictFormat *pictformat = NULL;

	if (pictformat)
		return pictformat;

	XRenderPictFormat templ;
	templ.depth = 24;
	templ.type = PictTypeDirect;
	templ.direct.red   = 16;
	templ.direct.green = 8;
	templ.direct.blue  = 0;
	templ.direct.redMask   = 0xff;
	templ.direct.greenMask = 0xff;
	templ.direct.blueMask  = 0xff;

	static const unsigned long kMask =
		PictFormatType | PictFormatDepth |
		PictFormatRed | PictFormatRedMask |
		PictFormatGreen | PictFormatGreenMask |
		PictFormatBlue | PictFormatBlueMask;

	pictformat = XRenderFindFormat(dpy, kMask, &templ, 0);

	if (!pictformat)
		pictformat = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	return pictformat;
}

static XRenderPictFormat *GetRenderARGB32Format(Display *dpy)
{
	static XRenderPictFormat *pictformat = NULL;

	if (pictformat)
		return pictformat;

	XRenderPictFormat templ;
	templ.depth = 32;
	templ.type = PictTypeDirect;
	templ.direct.red   = 16;
	templ.direct.green = 8;
	templ.direct.blue  = 0;
	templ.direct.alpha = 24;
	templ.direct.redMask   = 0xff;
	templ.direct.greenMask = 0xff;
	templ.direct.blueMask  = 0xff;
	templ.direct.alphaMask = 0xff;

	static const unsigned long kMask =
		PictFormatType | PictFormatDepth |
		PictFormatRed | PictFormatRedMask |
		PictFormatGreen | PictFormatGreenMask |
		PictFormatBlue | PictFormatBlueMask |
		PictFormatAlphaMask;

	pictformat = XRenderFindFormat(dpy, kMask, &templ, 0);

	if (!pictformat)
		pictformat = XRenderFindStandardFormat(dpy, PictStandardARGB32);

	return pictformat;
}

static void x11_composite(GalDrawable dst,
		int dx, int dy,
		GalDrawable src, int sx, int sy, int w, int h)
{
	GalDrawableX11 *xdst = X11_DRAWABLE_DATA(dst);
	GalDrawableX11 *xsrc = X11_DRAWABLE_DATA(src);
	XRenderPictFormat *format;
	XRenderPictureAttributes attributes;

	if (xsrc->picture == 0) {
		if (e_object_type_check(src, x11_genetype_pixmap())) {
			format = GetRenderARGB32Format(x11_dpy);
			attributes.graphics_exposures = False;
		}
		else {
			format = XRenderFindVisualFormat(x11_dpy, x11_vinfo->visual);
			attributes.graphics_exposures = True;
		}
		xsrc->picture = XRenderCreatePicture(x11_dpy, xsrc->xid,
				format, CPGraphicsExposure, &attributes);
	}

	if (xdst->picture == 0) {
		if (e_object_type_check(dst, x11_genetype_pixmap())) {
			if (xdst->depth == 32)
				format = GetRenderARGB32Format(x11_dpy);
			else
				format = GetRenderRGB24Format(x11_dpy);
			attributes.graphics_exposures = False;
			xdst->picture = XRenderCreatePicture(x11_dpy, xdst->xid,
					format, CPGraphicsExposure, &attributes);
		}
		else {
			format = XRenderFindVisualFormat(x11_dpy, x11_vinfo->visual);
			attributes.graphics_exposures = True;
			attributes.subwindow_mode     = IncludeInferiors | ClipByChildren;
			xdst->picture = XRenderCreatePicture(x11_dpy, xdst->xid,
					format, CPSubwindowMode | CPGraphicsExposure, &attributes);
		}
	}

	XRenderComposite(x11_dpy, PictOpOver,
			xsrc->picture, 0,
			xdst->picture,
			sx, sy,
			0, 0,
			dx, dy,
			w, h);
}

static void x11_composite_image(GalDrawable dst,
		int dx, int dy,
		GalImage *img, int sx, int sy, int w, int h)
{
	GalImageX11 *ximg = (GalImageX11 *)img;

	if (ximg->pixmap == 0) {
		ximg->pixmap = x11_drawable_new(img->w, img->h, img->depth);
		ximg->pb     = x11_pb_new(ximg->pixmap, NULL);
	}
	x11_draw_image(ximg->pixmap, ximg->pb, sx, sy, img, sx, sy, w, h);

	x11_composite(dst, dx, dy, ximg->pixmap, sx, sy, w, h);
}

static void x11_composite_subwindow(GalWindow window, int x, int y, int w, int h)
{
	GalWindowX11 *child = X11_WINDOW_DATA(window);

	if (!child->parent)
		return;
	x11_composite(OBJECT_OFFSET(child->parent), x, y, window, x, y, w, h);
}

static eint x11_get_mark(GalDrawable drawable)
{
	return X11_DRAWABLE_DATA(drawable)->xid;
}

static eint x11_get_depth(GalDrawable drawable)
{
	return X11_DRAWABLE_DATA(drawable)->depth;
}

static GalVisual *x11_get_visual(GalDrawable drawable)
{
	return X11_DRAWABLE_DATA(drawable)->visual;
}

static GalColormap *x11_get_colormap(GalDrawable drawable)
{
	return X11_DRAWABLE_DATA(drawable)->colormap;
}

static int x11_get_visual_info(GalDrawable drawable, GalVisualInfo *vinfo)
{
	GalDrawableX11 *xdraw = X11_DRAWABLE_DATA(drawable);

	vinfo->w     = xdraw->w;
	vinfo->h     = xdraw->h;
	vinfo->depth = xdraw->depth;
	return 0;
}

static ePointer x11_refer_surface_private(GalDrawable drawable)
{
#ifdef _GAL_SUPPORT_CAIRO
	if (X11_DRAWABLE_DATA(drawable)->surface)
		return X11_SURFACE_DATA(X11_DRAWABLE_DATA(drawable)->surface)->cr;
	else
		return ENULL;
#else
	return ENULL;
#endif
}

static void x11_refer_surface(eHandle hobj)
{
#ifdef _GAL_SUPPORT_CAIRO
	cairo_surface_reference(X11_SURFACE_DATA(hobj)->surface);
#endif
}

static void x11_unref_surface(eHandle hobj)
{
#ifdef _GAL_SUPPORT_CAIRO
	cairo_surface_destroy(X11_SURFACE_DATA(hobj)->surface);
#endif
}

static void x11_surface_init_orders(eGeneType new, ePointer this)
{
	eCellOrders *cell = e_genetype_orders(new, GTYPE_CELL);
	cell->refer = x11_refer_surface;
	cell->unref = x11_unref_surface;
}

static void x11_surface_free_data(eHandle hobj, ePointer this)
{
#ifdef _GAL_SUPPORT_CAIRO
	GalSurfaceX11 *xface = this;
	cairo_destroy(xface->cr);
	//cairo_surface_destroy(xface->surface);
#endif
}

static eGeneType x11_genetype_surface(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			x11_surface_init_orders, 
			sizeof(GalSurfaceX11),
			NULL,
			x11_surface_free_data, NULL
		};

		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

static GalSurface __x11_refer_surface(GalDrawable drawable, GalDrawableX11 *draw)
{
	if (!draw->surface) {
		GalSurfaceX11 *xface;
		GalVisualX11  *visual = (GalVisualX11 *)egal_get_visual(drawable);

		draw->surface = e_object_new(x11_genetype_surface());
		xface = X11_SURFACE_DATA(draw->surface);
#ifdef _GAL_SUPPORT_CAIRO
		if (draw->depth == 32) {
			XRenderPictFormat *format;
			if (e_object_type_check(drawable, x11_genetype_pixmap()))
				format = GetRenderARGB32Format(x11_dpy);
			else
				format = XRenderFindVisualFormat(x11_dpy, x11_vinfo->visual);
			xface->surface =
				cairo_xlib_surface_create_with_xrender_format(x11_dpy,
					draw->xid, visual->xvid, format, draw->w, draw->h);
		}
		else
			xface->surface = cairo_xlib_surface_create(x11_dpy, draw->xid, visual->xvid, draw->w, draw->h);

		xface->cr = cairo_create(xface->surface);
#endif
	}
	else
		e_object_refer(draw->surface);

	return draw->surface;
}

#ifdef _GAL_SUPPORT_CAIRO
static GalSurface x11_drawable_refer_surface(GalDrawable drawable)
{
	return __x11_refer_surface(drawable, X11_DRAWABLE_DATA(drawable));
}
#endif

static void x11_drawable_init(eHandle drawable, Window xid, eint w, eint h, eint depth)
{
	GalDrawableX11 *xdraw = X11_DRAWABLE_DATA(drawable);
	xdraw->w        = w;
	xdraw->h        = h;
	xdraw->xid      = xid;
	xdraw->depth    = depth;
	xdraw->visual   = (GalVisual *)&x11_visual;
	xdraw->colormap = (GalColormap *)&x11_cmap;

	xdraw->surface = __x11_refer_surface(drawable, xdraw);
}

static void x11_drawable_init_orders(eGeneType new, ePointer this)
{
	GalDrawableOrders *draw = e_genetype_orders(new, GTYPE_GAL_DRAWABLE);

	draw->get_mark        = x11_get_mark;
	draw->get_depth       = x11_get_depth;
	draw->draw_image      = x11_draw_image;
	draw->draw_point      = x11_draw_point;
	draw->draw_line       = x11_draw_line;
	draw->draw_rect       = x11_draw_rect;
	draw->draw_arc        = x11_draw_arc;
	draw->fill_rect       = x11_fill_rect;
	draw->fill_arc        = x11_fill_arc;
	draw->get_visual      = x11_get_visual;
	draw->get_colormap    = x11_get_colormap;
	draw->get_visual_info = x11_get_visual_info;

	draw->composite           = x11_composite;
	draw->composite_image     = x11_composite_image;
	draw->composite_subwindow = x11_composite_subwindow;
	draw->draw_drawable       = x11_draw_drawable;

#ifdef _GAL_SUPPORT_CAIRO
	draw->refer_surface         = x11_drawable_refer_surface;
	draw->refer_surface_private = x11_refer_surface_private;
#endif
}

static void x11_drawable_free(eHandle hobj, ePointer data)
{
	GalDrawableX11 *xdraw = data;
	if (xdraw->surface)
		e_object_unref(xdraw->surface);
}

static eGeneType x11_genetype_drawable(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			x11_drawable_init_orders,
			sizeof(GalDrawableX11),
			NULL, x11_drawable_free, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_DRAWABLE, NULL);
	}
	return gtype;
}

static void x11_cursor_free(eHandle hobj, ePointer data)
{
	GalCursorX11 *xcursor = data;
	XFreeCursor(x11_dpy, xcursor->xid);
	if (xcursor->name)
		e_free(xcursor->name);
}

static eGeneType x11_genetype_cursor(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0,
			NULL,
			sizeof(GalCursorX11),
			NULL,
			x11_cursor_free,
			NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_GAL_CURSOR, NULL);
	}
	return gtype;
}

static void x11_convert_format(euchar *src_buf,
                            eint              src_row,
                            euchar           *dst_buf,
                            eint              dst_row,
                            eint              width,
                            eint              height)
{
	eint i;

	for (i = 0; i < height; i++) {
		euchar *p = (src_buf + i * src_row);
		euchar *q = (dst_buf + i * dst_row);
		euchar *end = p + 4 * width;
		euint t1,t2,t3;

#define MULT(d,c,a,t) { t = c * a; d = ((t >> 8) + t) >> 8; }

#if 1
		while (p < end)
		{
			MULT(q[0], p[2], p[3], t1);
			MULT(q[1], p[1], p[3], t2);
			MULT(q[2], p[0], p[3], t3);
			q[3] = p[3];
			p += 4;
			q += 4;
		}
#else
		while (p < end) {
			q[0] = p[3];
			MULT(q[1], p[0], p[3], t1);
			MULT(q[2], p[1], p[3], t2);
			MULT(q[3], p[2], p[3], t3);
			p += 4;
			q += 4;
		}
#endif
#undef MULT
	}
}

static XcursorImage* create_cursor_image(GalPixbuf *pixbuf, eint x, eint y)
{
	euint width, height, rowstride, n_channels;
	euchar *pixels, *src;
	XcursorImage *xcimage;
	XcursorPixel *dest;

	width  = pixbuf->w;
	height = pixbuf->h;

	n_channels = pixbuf->pixelbytes;
	rowstride  = pixbuf->rowbytes;
	pixels     = pixbuf->pixels;

	xcimage = XcursorImageCreate(width, height);

	xcimage->xhot = x;
	xcimage->yhot = y;

	dest = xcimage->pixels;

	if (n_channels == 3) {
		eint i, j;

		for (j = 0; j < height; j++) {
			src = pixels + j * rowstride;
			for (i = 0; i < width; i++) {
				*dest = (0xff << 24) | (src[0] << 16) | (src[1] << 8) | src[2];
			}
			src += n_channels;
			dest++;
		}
	}
	else
		x11_convert_format(pixels, rowstride, (euchar *)dest, 4 * width, width, height);

	return xcimage;
}

static GalCursor x11_cursor_new_pixbuf(GalPixbuf *pixbuf, eint x, eint y)
{
	XcursorImage *xcimage;
	GalCursor cursor;
	Cursor xid;

	if (x < 0 || x >= pixbuf->w)
		return 0;
	if (y < y || y >= pixbuf->h)
		return 0;

	xcimage = create_cursor_image(pixbuf, x, y);
	xid     = XcursorImageLoadCursor(x11_dpy, xcimage);
	XcursorImageDestroy(xcimage);

	cursor  = e_object_new(x11_genetype_cursor());
	X11_CURSOR_DATA(cursor)->xid = xid;

	return cursor;
}

static GalCursor x11_cursor_new_name(const echar *name)
{
	GalCursorX11 *xcursor;
	GalCursor cursor;
	Cursor xid;

	xid = XcursorLibraryLoadCursor(x11_dpy, (const char *)name);
	if (xid == None)
		return 0;

	cursor  = e_object_new(x11_genetype_cursor());
	xcursor = X11_CURSOR_DATA(cursor);
	xcursor->xid  = xid;
	xcursor->name = e_strdup(name);

	return cursor;
}

static GalCursor x11_cursor_new(GalCursorType type)
{
	GalCursorX11 *xcursor;
	GalCursor cursor;
	Cursor xid;

	xid = XCreateFontCursor(x11_dpy, type);
	if (xid == None)
		return 0;

	cursor  = e_object_new(x11_genetype_cursor());
	xcursor = X11_CURSOR_DATA(cursor);
	xcursor->xid  = xid;
	xcursor->type = type;

	return cursor;
}

void GalSwapBuffers(void)
{
#ifdef _GAL_SUPPORT_OPENGL
	if (x11_glc->xid)
		glXSwapBuffers(x11_dpy, x11_glc->xid);
#endif
}
