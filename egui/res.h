#ifndef __GUI_RES_H
#define __GUI_RES_H

#define GUI_RES_HANDLE			egui_res_handle

typedef struct _GuiRes			GuiRes;
typedef struct _GuiResItem		GuiResItem;

struct _GuiResItem {
	const echar *name;
	ePointer data;
	struct _GuiResItem *next;
};

struct _GuiRes {
	const echar *name;
	eint n;
	GuiResItem items[0];
};

extern eHandle egui_res_handle;
elong egui_init_res(void);

GuiResItem *egui_res_find(eHandle, const echar *);
ePointer egui_res_find_item(GuiResItem *, const echar *);

#endif
