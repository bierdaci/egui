#ifndef __GUI_H__
#define __GUI_H__

#include <elib/object.h>
#include <egal/egal.h>
#include <egui/layout.h>
#include <egui/widget.h>
#include <egui/bin.h>

#define GUI_TYPE_WINDOW(hobj)					(e_object_type_check(hobj, GTYPE_WINDOW))
static inline bool GUI_TYPE_BIN(hobj)
{
	GuiBin *b = GUI_BIN_DATA(hobj);
	if (b && b->is_bin)
		return true;
	return false;
}

#define GTYPE_GUI								(egui_genetype())
#define GTYPE_STRINGS						    (egui_genetype_strings())
#define GTYPE_INOUT_WIDGET 						(egui_genetype_inout())

#define GUI_STRINGS_ORDERS(hobj)				((GuiStringsOrders *)e_object_type_orders(hobj, GTYPE_STRINGS))

#define GUI_STATUS_VISIBLE(hobj)				(WIDGET_STATUS_VISIBLE(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_FOCUS(hobj)					(WIDGET_STATUS_FOCUS(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_REFER(hobj)					(WIDGET_STATUS_REFER(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_IME_OPEN(hobj)				(WIDGET_STATUS_IME_OPEN(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_TRANSPARENT(hobj)			(WIDGET_STATUS_TRANSPARENT(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_ACTIVE(hobj)					(WIDGET_STATUS_ACTIVE(GUI_WIDGET_DATA(hobj)))
#define GUI_STATUS_MOUSE(hobj)					(WIDGET_STATUS_MOUSE(GUI_WIDGET_DATA(hobj)))

#define GUI_STATUS_OFFSET_VISIBLE(hobj)			(GUI_STATUS_VISIBLE(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_FOCUS(hobj)			(GUI_STATUS_FOCUS(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_REFER(hobj)			(GUI_STATUS_REFER(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_IME_OPEN(hobj)		(GUI_STATUS_IME_OPEN(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_TRANSPARENT(hobj)	    (GUI_STATUS_TRANSPARENT(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_ACTIVE(hobj)			(GUI_STATUS_ACTIVE(OBJECT_OFFSET(hobj)))
#define GUI_STATUS_OFFSET_MOUSE(hobj)			(GUI_STATUS_MOUSE(OBJECT_OFFSET(hobj)))

typedef enum {
  GUI_POS_LEFT,
  GUI_POS_RIGHT,
  GUI_POS_TOP,
  GUI_POS_BOTTOM,
} GuiPositionType;

typedef eint (*AccelKeyCB)(eHandle, ePointer);

typedef struct _GuiStringsOrders                GuiStringsOrders;
typedef struct _GuiAccelKey                     GuiAccelKey;

struct _GuiAccelKey {
	GalKeyState   state;
	GalKeyCode    code;
	ePointer      data;
	AccelKeyCB    cb;
	GuiAccelKey *next;
};

struct _GuiStringsOrders {
	void (*set_max) (eHandle, eint);
	void (*set_text)(eHandle, const echar *, eint);
	eint (*get_text)(eHandle, echar *, eint);
	void (*insert_text)(eHandle, const echar *, eint);
	const echar *(*get_strings)(eHandle);
};

static inline void egui_set_status(eHandle hobj, GuiStatus flag)
{
	widget_set_status(GUI_WIDGET_DATA(hobj), flag);
}

static inline void egui_unset_status(eHandle hobj, GuiStatus flag)
{
	widget_unset_status(GUI_WIDGET_DATA(hobj), flag);
}

static inline void egui_set_extra_data(eHandle hobj, ePointer data)
{
	widget_set_extra_data(GUI_WIDGET_DATA(hobj), data);
}

static inline ePointer egui_get_extra_data(eHandle hobj)
{
	return widget_get_extra_data(GUI_WIDGET_DATA(hobj));
}

static inline eHandle egui_get_top(eHandle hobj)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	while (wid->parent)
		wid = GUI_WIDGET_DATA(wid->parent);
	if (wid->window) {
		GalWindowAttr attr;
		egal_window_get_attr(wid->window, &attr);
		if (attr.type == GalWindowTop
				|| attr.type == GalWindowTemp
				|| attr.type == GalWindowDialog)
			return OBJECT_OFFSET(wid);
	}
	return 0;
}

eGeneType egui_genetype(void);
eGeneType egui_genetype_strings(void);
eGeneType egui_genetype_inout(void);

int  egui_init(int, char* const[]);
int  egui_main(void);
void egui_quit(void);
void egui_show(eHandle, bool);
void egui_hide(eHandle, bool);
void egui_show_async(eHandle, bool);
void egui_hide_async(eHandle, bool);
void egui_update(eHandle);
void egui_update_rect(eHandle, GalRect *);
void egui_update_async(eHandle);
void egui_update_rect_async(eHandle, GalRect *);
void egui_put(eHandle, eHandle, eint, eint);
void egui_remove(eHandle); 
void egui_move(eHandle, eint, eint);
void egui_move_resize(eHandle, eint, eint, eint, eint);
void egui_raise(eHandle);
void egui_lower(eHandle);
void egui_set_focus(eHandle);
void egui_unset_focus(eHandle);
void egui_set_font(eHandle, GalFont);
void egui_insert_text(eHandle, const echar *, eint);
void egui_draw_strings(GalDrawable, GalPB, GalFont, const echar *, GalRect *, LayoutFlags);
void egui_draw_vbar(GalWindow, GalPB, GalImage *, GalImage *, GalImage *, int, int, int, int, int);
void egui_draw_hbar(GalWindow, GalPB, GalImage *, GalImage *, GalImage *, int, int, int, int, int);
void egui_add(eHandle, eHandle);
void egui_add_spacing(eHandle, eint);
void egui_request_resize(eHandle, eint, eint);
void egui_request_layout_async(eHandle, eHandle, eint, eint, bool, bool);
void egui_request_layout(eHandle, eHandle, eint, eint, bool, bool);
void egui_strings_extent(GalFont, const echar *, eint *, eint *);
void egui_set_strings(eHandle, const echar *);
const echar *egui_get_strings(eHandle);

eint egui_get_text(eHandle, echar *, eint);
void egui_set_max_text(eHandle, eint);

void egui_set_transparent(eHandle);
void egui_unset_transparent(eHandle);

void egui_hook_v(eHandle, eHandle);
void egui_hook_h(eHandle, eHandle);

void egui_set_min(eHandle hobj, eint w, eint h);
void egui_set_max(eHandle hobj, eint w, eint h);

void egui_set_fg_color(eHandle hobj, euint32 color);
void egui_set_bg_color(eHandle hobj, euint32 color);

void egui_set_expand  (eHandle, bool);
void egui_set_expand_v(eHandle, bool);
void egui_set_expand_h(eHandle, bool);

void egui_accelkey_connect(eHandle hobj, const echar *accelkey, AccelKeyCB cb, ePointer data);

void egui_signal_emit (eHandle, eint);
void egui_signal_emit1(eHandle, eint, ePointer);
void egui_signal_emit2(eHandle, eint, ePointer, ePointer);
void egui_signal_emit3(eHandle, eint, ePointer, ePointer, ePointer);
void egui_signal_emit4(eHandle, eint, ePointer, ePointer, ePointer, ePointer);
#endif
