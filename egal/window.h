#ifndef __GAL_WINDOW_H
#define __GAL_WINDOW_H

#include <elib/elib.h>
#include <egal/types.h>
#include <egal/rect.h>
#include <egal/pixbuf.h>
#include <egal/cursor.h>

#define GTYPE_GAL						(egal_genetype())
#define GTYPE_GAL_PB					(egal_genetype_pb())
#define GTYPE_GAL_WINDOW				(egal_genetype_window())
#define GTYPE_GAL_CURSOR				(egal_genetype_cursor())
#define GTYPE_GAL_DRAWABLE				(egal_genetype_drawable())

#define GAL_PB_ORDERS(hobj)				((GalPBOrders *)e_object_type_orders(hobj, GTYPE_GAL_PB))
#define GAL_WINDOW_ORDERS(hobj)			((GalWindowOrders *)e_object_type_orders(hobj, GTYPE_GAL_WINDOW))
#define GAL_CURSOR_ORDERS(hobj)			((GalCursorOrders *)e_object_type_orders(hobj, GTYPE_GAL_CURSOR))
#define GAL_DRAWABLE_ORDERS(hobj)		((GalDrawableOrders *)e_object_type_orders(hobj, GTYPE_GAL_DRAWABLE))
#define GAL_SURFACE_DATE(hobj)			((GalDrawableOrders *)e_object_type_orders(hobj, GTYPE_GAL_DRAWABLE))

typedef eHandle						GalDrawable;
typedef eHandle						GalWindow;
typedef eHandle						GalPB;
typedef eHandle						GalFont;
typedef eHandle						GalCursor;
typedef eHandle						GalSurface;

typedef struct _GalImage			GalImage;
typedef struct _GalScreen			GalScreen;
typedef struct _GalVisual			GalVisual;
typedef struct _GalColor			GalColor;
typedef struct _GalColormap			GalColormap;
typedef struct _GalDrawableOrders	GalDrawableOrders;
typedef struct _GalWindowOrders		GalWindowOrders;
typedef struct _GalCursorOrders		GalCursorOrders;
typedef struct _GalPBOrders			GalPBOrders;
typedef struct _GalWindowManager	GalWindowManager;
typedef struct _GalWindowAttr		GalWindowAttr;
typedef struct _GalGeometry			GalGeometry;
typedef struct _GalVisualInfo		GalVisualInfo;
typedef struct _GalPBAttr			GalPBAttr;
typedef struct _GalEvent			GalEvent;
typedef struct _GalPattern			GalPattern;

typedef enum {
	GalGrabSuccess         = 0, 
	GalGrabAlready_grabbed = 1, 
	GalGrabInvalid_time    = 2, 
	GalGrabNot_viewable    = 3, 
	GalGrabFrozen          = 4 
} GalGrabStatus;

typedef enum {
	GalPBclear			= 0,
	GalPBand			= 1,
	GalPBandReverse		= 2,
	GalPBcopy			= 3,
	GalPBandInverted	= 4,
	GalPBnoop			= 5,
	GalPBxor			= 6,
	GalPBor				= 7,
	GalPBnor			= 8,
	GalPBequiv			= 9,
	GalPBinvert			= 10,
	GalPBorReverse		= 11,
	GalPBcopyInverted	= 12,
	GalPBorInverted		= 13,
	GalPBnand			= 14,
	GalPBset			= 15,
} GalPBFunc;

struct _GalWindowManager {
	GalWindow   (*wait_event)(GalEvent *, ebool);
	void        (*get_event)(GalEvent *);
	GalWindow   (*window_new)(GalWindowAttr *);
	GalDrawable (*drawable_new)(eint, eint, ebool);
	GalImage*   (*image_new)(eint, eint, ebool);
	void        (*image_free)(GalImage *);
	GalPB       (*pb_new)(GalDrawable, GalPBAttr *);
	GalCursor   (*cursor_new)(GalCursorType);
	GalCursor   (*cursor_new_name)(const echar *);
	GalCursor   (*cursor_new_pixbuf)(GalPixbuf *, eint, eint);
	GalFont		(*create_font)(GalPattern *);
	GalWindow   root_window;
};

typedef enum {
	GalNoEventMask				= 0L,
	GalKeyPressMask				= 1L<<0,
	GalKeyReleaseMask			= 1L<<1,
	GalButtonPressMask			= 1L<<2,
	GalButtonReleaseMask		= 1L<<3,
	GalEnterWindowMask			= 1L<<4,
	GalLeaveWindowMask			= 1L<<5,
	GalPointerMotionMask		= 1L<<6,
	GalPointerMotionHintMask	= 1L<<7,
	GalButton1MotionMask		= 1L<<8,
	GalButton2MotionMask		= 1L<<9,
	GalButton3MotionMask		= 1L<<10,
	GalButton4MotionMask		= 1L<<11,
	GalButton5MotionMask		= 1L<<12,
	GalButtonMotionMask			= 1L<<13,
	GalKeymapStateMask			= 1L<<14,
	GalExposureMask				= 1L<<15,
	GalVisibilityChangeMask		= 1L<<16,
	GalStructureNotifyMask		= 1L<<17,
	GalResizeRedirectMask		= 1L<<18,
	GalSubstructureNotifyMask	= 1L<<19,
	GalSubstructureRedirectMask	= 1L<<20,
	GalFocusChangeMask			= 1L<<21,
	GalPropertyChangeMask		= 1L<<22,
	GalColormapChangeMask		= 1L<<23,
	GalOwnerGrabButtonMask		= 1L<<24,
} GalEventMask;

typedef enum {
	GalWindowChild	= 0,
	GalWindowTop	= 1,
	GalWindowTemp   = 2,
	GalWindowRoot	= 3,
	GalWindowDialog = 4,
} GalWindowType;

typedef enum {        
	GAL_INPUT_OUTPUT,
	GAL_INPUT_ONLY,
} GalWindowClass;

typedef enum {
	GAL_WA_TITLE     = 1 << 1,
	GAL_WA_X         = 1 << 2,
	GAL_WA_Y         = 1 << 3,
	GAL_WA_CURSOR    = 1 << 4,
	GAL_WA_COLORMAP  = 1 << 5,
	GAL_WA_VISUAL    = 1 << 6,
	GAL_WA_WMCLASS   = 1 << 7,
	GAL_WA_NOREDIR   = 1 << 8,
	GAL_WA_TYPE_HINT = 1 << 9,
	GAL_WA_TOPMOST   = 1 << 10,
} GalWindowAttributesMask;

typedef enum {
	GAL_SHIFT_MASK    = 1 << 0,
	GAL_LOCK_MASK     = 1 << 1,
	GAL_CONTROL_MASK  = 1 << 2,
	GAL_MOD1_MASK     = 1 << 3,
	GAL_MOD2_MASK     = 1 << 4,
	GAL_MOD3_MASK     = 1 << 5,
	GAL_MOD4_MASK     = 1 << 6,
	GAL_MOD5_MASK     = 1 << 7,
	GAL_BUTTON1_MASK  = 1 << 8,
	GAL_BUTTON2_MASK  = 1 << 9,
	GAL_BUTTON3_MASK  = 1 << 10,
	GAL_BUTTON4_MASK  = 1 << 11,
	GAL_BUTTON5_MASK  = 1 << 12,

	GAL_SUPER_MASK    = 1 << 26,
	GAL_HYPER_MASK    = 1 << 27,
	GAL_META_MASK     = 1 << 28,

	GAL_RELEASE_MASK  = 1 << 30,

	GAL_MODIFIER_MASK = 0x5c001fff
} GalModifierType;

typedef enum {
	GAL_HINT_POS			= 1L << 0,
	GAL_HINT_USER_POS		= 1L << 1,
	GAL_HINT_USER_SIZE		= 1L << 2,
	GAL_HINT_MIN_SIZE		= 1L << 3,
	GAL_HINT_MAX_SIZE		= 1L << 4,
	GAL_HINT_BASE_SIZE		= 1L << 5,
	GAL_HINT_RESIZE_INC		= 1L << 6,
	GAL_HINT_ASPECT			= 1L << 7,
	GAL_HINT_WIN_GRAVITY	= 1L << 8,
} GalWindowHints;

struct _GalWindowAttr {
	GalWindowType type;
	eint x, y;
	eint width;
	eint height;
	eint depth;
	ebool visible;
	ebool input_event;
	ebool output_event;
	GalEventMask event_mask;
	GalWindowClass wclass;
	GalWindowAttributesMask wa_mask;
};

struct _GalGeometry {
	eint min_width;
	eint min_height;
	eint max_width;
	eint max_height;
	eint base_width;
	eint base_height;
	eint width_inc;
	eint height_inc;
	eint min_aspect;
	eint max_aspect;
	eint win_gravity;
};

struct _GalWindowOrders {
	eint (*window_put)(GalWindow, GalWindow, eint, eint);
	eint (*window_show)(GalWindow);
	eint (*window_hide)(GalWindow);
	eint (*window_move)(GalWindow, eint, eint);
	eint (*window_resize)(GalWindow, eint, eint);
	eint (*window_configure)(GalWindow, GalRect *);
	eint (*window_move_resize)(GalWindow, eint, eint, eint, eint);
	eint (*window_raise)(GalWindow);
	eint (*window_lower)(GalWindow);
	eint (*window_remove)(GalWindow);

	eint    (*set_name)(GalWindow, const echar *);

	void    (*set_attachment)(GalWindow, eHandle);
	eHandle (*get_attachment)(GalWindow);

	eint      (*set_cursor)(GalWindow, GalCursor);
	GalCursor (*get_cursor)(GalWindow);

	eint (*set_attr)(GalWindow, GalWindowAttr *);
	eint (*get_attr)(GalWindow, GalWindowAttr *);

	void (*warp_pointer)(GalWindow, int, int, int, int, int, int); 
	GalGrabStatus (*grab_pointer)(GalWindow, ebool, GalCursor);
	GalGrabStatus (*ungrab_pointer)(GalWindow);
	GalGrabStatus (*grab_keyboard)(GalWindow, ebool);
	GalGrabStatus (*ungrab_keyboard)(GalWindow);

	void (*get_pointer)(GalWindow, eint *, eint *, eint *, eint *, GalModifierType *);
	eint (*get_origin)(GalWindow, eint *, eint *);

	void (*set_geometry_hints)(GalWindow, GalGeometry *, GalWindowHints);
	void (*get_geometry_hints)(GalWindow, GalGeometry *, GalWindowHints *);

	void (*make_GL)(GalWindow);
};

struct _GalDrawableOrders {
	void (*draw_drawable)(GalDrawable, GalPB, eint, eint, GalDrawable, GalPB, eint, eint, eint, eint);
	void (*draw_image)(GalDrawable, GalPB, eint, eint, GalImage *, eint, eint, eint, eint);
	void (*draw_point)(GalDrawable, GalPB, eint, eint);
	void (*draw_line)(GalDrawable, GalPB, eint, eint, eint, eint);
	void (*draw_rect)(GalDrawable, GalPB, eint, eint, eint, eint);
	void (*draw_arc)(GalDrawable, GalPB, eint, eint, euint, euint, eint, eint);
	void (*fill_rect)(GalDrawable, GalPB, eint, eint, eint, eint);
	void (*fill_arc)(GalDrawable, GalPB, eint, eint, euint, euint, eint, eint);

	void (*composite)(GalDrawable, GalPB, eint, eint, GalDrawable, GalPB, eint, eint, eint, eint);
	void (*composite_image)(GalDrawable, GalPB, eint, eint, GalImage *, eint, eint, eint, eint);

	eint (*get_mark)       (GalDrawable);
	eint (*get_depth)      (GalDrawable);
	void (*get_size)       (GalDrawable drawable, eint *, eint *);
	void (*set_colormap)   (GalDrawable , GalColormap *);
	eint (*get_visual_info)(GalDrawable, GalVisualInfo *);


	GalColormap* (*get_colormap)(GalDrawable);
	GalVisual *  (*get_visual)(GalDrawable);
	GalScreen *  (*get_screen)(GalDrawable);
	GalImage  *  (*get_image)(GalDrawable , eint, eint, eint, eint);

	GalSurface (*refer_surface)(GalDrawable);
	ePointer   (*refer_surface_private)(GalDrawable);
};

struct _GalCursorOrders {
#ifdef WIN32
	int a;
#endif
};

struct _GalImage {
    eint w, h;
    eint depth;
    eint pixelbytes;
    eint rowbytes;
	eint negative;
	ebool alpha;
    euchar *pixels;
};

struct _GalVisual {
	//GalVisualType type;
	eint depth;
	//GalByteOrder byte_order;
	eint colormap_size;
	eint bits_per_rgb;
	
	euint32 red_mask;
	eint red_shift;
	eint red_prec;
	
	euint32 green_mask;
	eint green_shift;
	eint green_prec;
	
	euint32 blue_mask;
	eint blue_shift;
	eint blue_prec;
};

struct _GalScreen {
#ifdef WIN32
	int a;
#endif
};

struct _GalVisualInfo {
	eint  w, h;
	euint depth;
};

struct _GalColor {
	euint32 pixel;
	euint16 red; 
	euint16 green;
	euint16 blue;
};

struct _GalColormap {
	eint       size;
	GalColor  *colors;
	GalVisual *visual;
	ePointer   data;
};

struct _GalPBAttr {
	GalPBFunc func;
	euint foreground;
	euint background;
	eint line_width;
	eint line_style;
	eint fill_style;
	eint fill_rule;
	eint arc_mode;
#ifdef _GAL_SUPPORT_CAIRO
	ebool use_cairo;
#endif
};

struct _GalPBOrders {
	eint  (*set_attr)(GalPB, GalPBAttr *);
	eint  (*get_attr)(GalPB, GalPBAttr *);
	euint (*get_background)(GalPB);
	euint (*get_foreground)(GalPB);
	eint  (*set_foreground)(GalPB, euint);
	eint  (*set_background)(GalPB, euint);
	eint  (*set_font)(GalPB, GalFont);
	euint (*set_color)(GalPB, euint);
	euint (*get_color)(GalPB);
};

struct _GalSurface {
	ePointer priv;
};

typedef enum {
	GAL_ET_QUIT,
	GAL_ET_PRIVATE,
	GAL_ET_CONFIGURE,
	GAL_ET_RESIZE,
	GAL_ET_EXPOSE,
	GAL_ET_KEYDOWN,
	GAL_ET_KEYUP,
	GAL_ET_MOUSEMOVE,
	GAL_ET_LBUTTONDOWN,
	GAL_ET_LBUTTONUP,
	GAL_ET_RBUTTONDOWN,
	GAL_ET_RBUTTONUP,
	GAL_ET_MBUTTONDOWN,
	GAL_ET_MBUTTONUP,
	GAL_ET_WHEELFORWARD,
	GAL_ET_WHEELBACKWARD,
	GAL_ET_IME_INPUT,
	GAL_ET_ENTER,
	GAL_ET_LEAVE,
	GAL_ET_FOCUS_IN,
	GAL_ET_FOCUS_OUT,
	GAL_ET_TIMEOUT,
	GAL_ET_DESTROY,
} GalEventType;

typedef enum {
	GAL_KS_SHIFT     = 1 << 0,
	GAL_KS_CAPSLOCK  = 1 << 1,
	GAL_KS_CTRL      = 1 << 2,
	GAL_KS_ALT       = 1 << 3,
	GAL_KS_NUMLOCK   = 1 << 5,
	GAL_KS_WIN       = 1 << 6,
} GalKeyState;

typedef struct {
	GalPB pb;
	GalRect rect;
} GalEventExpose;

typedef struct {
	eint w, h;
} GalEventResize;

typedef struct {
	GalRect rect;
} GalEventConfigure;

#include <egal/keysym.h>
typedef struct {
	GalKeyCode  code;
	GalKeyState state;
	euint32     time;
} GalEventKey;

typedef struct {
	GalPoint point;
	eint root_x;
	eint root_y;
	euint32 time;
	GalKeyState state;
} GalEventMouse;

typedef struct {
	eint   len;
	euchar data[sizeof(eHandle) * 7];
} GalEventImeInput;

typedef struct {
	eHandle args[8];
} GalEventPrivate;

struct _GalEvent {
	GalEventType  type;
	eint private_type;
	GalWindow   window;
	union {
		GalEventPrivate   private;
		GalEventExpose    expose;
		GalEventResize    resize;
		GalEventConfigure configure;
		GalEventKey       key;
		GalEventMouse     mouse;
		GalEventImeInput  imeinput;
	} e;
};

extern GalWindowManager _gwm;

eGeneType egal_genetype(void);
eGeneType egal_genetype_pb(void);
eGeneType egal_genetype_drawable(void);
eGeneType egal_genetype_image(void);
eGeneType egal_genetype_window(void);
eGeneType egal_genetype_cursor(void);

GalPB egal_pb_new(GalDrawable, GalPBAttr *);
GalPB egal_type_pb(GalPBFunc);

GalImage *egal_image_new(eint, eint, ebool);
void egal_image_free(GalImage *);

GalWindow egal_window_new(GalWindowAttr *);
GalWindow egal_root_window(void);

GalDrawable egal_drawable_new(eint w, eint h, ebool alpha);

GalCursor egal_cursor_new(GalCursorType);
GalCursor egal_cursor_new_name(const echar *);
GalCursor egal_cursor_new_pixbuf(GalPixbuf *, eint, eint);

eint egal_window_put(GalWindow, GalWindow, eint, eint);
eint egal_window_show(GalWindow);
eint egal_window_hide(GalWindow);
eint egal_window_move(GalWindow, eint, eint);
eint egal_window_configure(GalWindow, GalRect *);
eint egal_window_raise(GalWindow);
eint egal_window_lower(GalWindow);
eint egal_window_remove(GalWindow);
eint egal_window_resize(GalWindow, eint, eint);
eint egal_window_move_resize(GalWindow, eint, eint, eint, eint);
eint egal_window_set_name(GalWindow, const echar *);

void egal_warp_pointer(GalWindow window, int sx, int sy, int sw, int sh, int dx, int dy);
GalGrabStatus egal_grab_pointer(GalWindow, ebool, GalCursor);
GalGrabStatus egal_ungrab_pointer(GalWindow);
GalGrabStatus egal_grab_keyboard(GalWindow, ebool);
GalGrabStatus egal_ungrab_keyboard(GalWindow);

void egal_get_pointer(GalWindow, eint *, eint *, eint *, eint *, GalModifierType *);

eHandle egal_window_get_attachment(GalWindow);
void egal_window_set_attachment(GalWindow, eHandle);
void egal_window_set_geometry_hints(GalWindow, GalGeometry *, GalWindowHints);
void egal_window_get_geometry_hints(GalWindow, GalGeometry *, GalWindowHints *);


GalPB egal_default_pb(void);
void egal_pb_set_attr(GalPB, GalPBAttr *);
euint egal_set_foreground(GalPB, euint);
euint egal_set_background(GalPB, euint);
euint egal_get_foreground(GalPB);
euint egal_get_background(GalPB);
euint egal_set_font_color(GalPB, euint);
euint egal_get_font_color(GalPB pb);

void egal_draw_drawable(GalDrawable, GalPB, eint, eint, GalDrawable, GalPB, eint, eint, eint, eint);
void egal_draw_image(GalDrawable, GalPB, eint, eint, GalImage *, eint, eint, eint, eint);
void egal_draw_point(GalDrawable, GalPB, eint, eint);
void egal_draw_line(GalDrawable, GalPB, eint, eint, eint, eint);
void egal_draw_rect(GalDrawable, GalPB, eint, eint, eint, eint);
void egal_draw_arc(GalDrawable, GalPB, eint, eint, euint, euint, eint, eint);
void egal_fill_rect(GalDrawable, GalPB, eint, eint, eint, eint);
void egal_fill_arc(GalDrawable, GalPB, eint, eint, euint, euint, eint, eint);

GalVisual *egal_get_visual(GalDrawable);
eint       egal_get_depth(GalDrawable);
eint       egal_get_visual_info(GalDrawable, GalVisualInfo *);
eint       egal_drawable_get_mark(GalDrawable);

GalSurface egal_refer_surface(GalDrawable);
ePointer egal_surface_private(GalDrawable);

void egal_composite(GalDrawable, GalPB, eint, eint, GalDrawable, GalPB, eint, eint, eint, eint);
void egal_composite_image(GalDrawable, GalPB, eint, eint, GalImage *, eint, eint, eint, eint);

eint egal_set_cursor(GalWindow, GalCursor);
GalCursor egal_get_cursor(GalWindow);

void egal_window_set_attr(GalWindow, GalWindowAttr *);
void egal_window_get_attr(GalWindow, GalWindowAttr *);
void egal_window_get_origin(GalWindow, eint *, eint *);

GalWindow egal_window_wait_event(GalEvent *, ebool);
void egal_window_get_event(GalEvent *event);
eint egal_window_init(void);

void egal_window_make_GL(GalWindow);
void GalSwapBuffers(void);

#endif
