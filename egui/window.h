#ifndef __GUI_WINDOW_H__
#define __GUI_WINDOW_H__

#include <elib/elib.h>
#include <egui/gui.h>

#define GTYPE_WINDOW			(egui_genetype_window())
#define GTYPE_DIALOG			(egui_genetype_dialog())

#define GUI_WINDOW_DATA(hobj)	((GuiWindow *)e_object_type_data((eHandle)(hobj), GTYPE_WINDOW))

typedef enum {
  GUI_WINDOW_TOPLEVEL,
  GUI_WINDOW_POPUP,
} GuiWindowType;

typedef struct _GuiWindow GuiWindow;

struct _GuiWindow {
	GuiWindowType type;
	e_thread_mutex_t lock;
	GuiAccelKey *head;
};

eGeneType egui_genetype_window(void);
eGeneType egui_genetype_dialog(void);
eHandle   egui_window_new(GuiWindowType);
eHandle   egui_dialog_new(void);
eint egui_window_set_name(eHandle, const echar *);

#endif
