#ifndef __GUI_WIDGET_H__
#define __GUI_WIDGET_H__

#include <egal/window.h>

#define GTYPE_WIDGET					(egui_genetype_widget())
#define GTYPE_FONT						(egui_genetype_font())

#define GUI_WIDGET_DATA(wid)			((GuiWidget       *)e_object_type_data((eHandle)(wid), GTYPE_WIDGET))
#define GUI_WIDGET_ORDERS(wid)			((GuiWidgetOrders *)e_object_type_orders((eHandle)(wid), GTYPE_WIDGET))
#define GUI_FONT_ORDERS(hobj)			((GuiFontOrders   *)e_object_type_orders((hobj), GTYPE_FONT))

#define SIG_HIDE						widget_signal_hide
#define SIG_SHOW						widget_signal_show
#define SIG_REALIZE						widget_signal_realize
#define SIG_CONFIGURE					widget_signal_configure
#define SIG_RESIZE						widget_signal_resize
#define SIG_EXPOSE						widget_signal_expose
#define SIG_EXPOSE_BG				    widget_signal_expose_bg
#define SIG_DESTROY						widget_signal_destroy

typedef struct _GuiWidget		GuiWidget;
typedef struct _GuiWidgetOrders	GuiWidgetOrders;
typedef struct _GuiFontOrders	GuiFontOrders;

typedef enum {
	GuiStatusFocus			= 1 << 0,
	GuiStatusActive 		= 1 << 1,
	GuiStatusVisible		= 1 << 2,
	GuiStatusMouse			= 1 << 3,
	GuiStatusRefer			= 1 << 4,
	GuiStatusImeOpen		= 1 << 5,
	GuiStatusTransparent	= 1 << 6,
} GuiStatus;

struct _GuiFontOrders {
	void (*set_font)(eHandle, GalFont);
	void (*set_color)(eHandle, euint32);
};

struct _GuiWidget {
	GuiStatus status;

	eHandle parent;
	eint min_w, min_h;
	eint max_w, max_h;
	eint offset_x;
	eint offset_y;
	GalRect rect;
	GuiWidget *prev;
	GuiWidget *next;

	GalPB       pb;
	GalDrawable window;
	GalDrawable drawable;
	eint        bg_color;
	eint        fg_color;
	ePointer    extra_data;
};

struct _GuiWidgetOrders {
	void (*show)(eHandle);
	void (*hide)(eHandle);
	eint (*resize)(eHandle, GuiWidget *, GalEventResize *);
	eint (*expose)(eHandle, GuiWidget *, GalEventExpose *);
	eint (*expose_bg)(eHandle, GuiWidget *, GalEventExpose *);

	eint (*configure)(eHandle, GuiWidget *, GalEventConfigure *);
	void (*add_umask)(eHandle, GalRect *);

	void (*add)(eHandle, eHandle);
	void (*put)(eHandle, eHandle);
	void (*move)(eHandle);
	void (*lower)(eHandle);
	void (*raise)(eHandle);
	void (*remove)(eHandle, eHandle);
	void (*move_resize)(eHandle, GalRect *, GalRect *);
	void (*request_resize)(eHandle, eint, eint);

	void (*set_focus)(eHandle);
	void (*unset_focus)(eHandle);

	void (*realize)(eHandle, GuiWidget *);

	void (*adjust_offset)(eHandle);

	void (*set_transparent)(eHandle);
	void (*unset_transparent)(eHandle);

	void (*set_min)(eHandle, eint, eint);
	void (*set_max)(eHandle, eint, eint);

	void (*set_font)(eHandle, GalFont);

	void (*set_fg_color)(eHandle, euint32);
	void (*set_bg_color)(eHandle, euint32);

	void (*destroy)(eHandle);
};

#define WIDGET_STATUS_VISIBLE(wid)				((wid)->status & GuiStatusVisible)
#define WIDGET_STATUS_FOCUS(wid)				((wid)->status & GuiStatusFocus)
#define WIDGET_STATUS_REFER(wid)				((wid)->status & GuiStatusRefer)
#define WIDGET_STATUS_IME_OPEN(wid)				((wid)->status & GuiStatusImeOpen)
#define WIDGET_STATUS_TRANSPARENT(wid)			((wid)->status & GuiStatusTransparent)
#define WIDGET_STATUS_ACTIVE(wid)				(((wid)->status & GuiStatusActive) && WIDGET_STATUS_VISIBLE(wid))
#define WIDGET_STATUS_MOUSE(wid)				(((wid)->status & GuiStatusMouse)  && WIDGET_STATUS_VISIBLE(wid))

extern esig_t widget_signal_configure;
extern esig_t widget_signal_expose;
extern esig_t widget_signal_expose_bg;
extern esig_t widget_signal_resize;
extern esig_t widget_signal_realize;
extern esig_t widget_signal_hide;
extern esig_t widget_signal_show;
extern esig_t widget_signal_destroy;

eGeneType egui_genetype_widget(void);
eGeneType egui_genetype_font(void);
void egui_create_window(eHandle, eint, eint);

static INLINE void widget_set_status(GuiWidget *data, GuiStatus flag)
{
	data->status |= flag;
}

static INLINE void widget_unset_status(GuiWidget *data, GuiStatus flag)
{
	data->status &= ~flag; 
}

static INLINE void widget_set_extra_data(GuiWidget *widget, ePointer data)
{
	widget->extra_data = data;
}

static INLINE ePointer widget_get_extra_data(GuiWidget *widget)
{
	return widget->extra_data;
}

#endif

