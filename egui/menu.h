#ifndef __GUI_MENU_H
#define __GUI_MENU_H

#define GTYPE_MENU					(egui_genetype_menu())
#define GTYPE_MENU_BAR      		(egui_genetype_menu_bar())
#define GTYPE_MENU_ITEM				(egui_genetype_menu_item())

#define GUI_MENU_DATA(hobj)			((GuiMenu      *)e_object_type_data((hobj), GTYPE_MENU))
#define GUI_MENU_ITEM_DATA(hobj)	((GuiMenuItem  *)e_object_type_data((hobj), GTYPE_MENU_ITEM))

typedef struct _GuiMenu GuiMenu;
typedef struct _GuiMenuItem  GuiMenuItem;
typedef struct _GuiMenuShell GuiMenuShell;
typedef struct _GuiMenuRadio GuiMenuRadio;
typedef struct _GuiMenuEntry GuiMenuEntry;

typedef eint (*MenuPositionCB)(eHandle, eint *x, eint *y, eint *h);

struct _GuiMenu {
	eHandle scrwin;
	eHandle bn1;
	eHandle bn2;
	eHandle box;
	eHandle popup;
	eHandle temp;
	eHandle shell;
	eHandle super;
	eHandle super_item;
	eTimer  popup_timer;
	eTimer  close_timer;
	eint    old_offset_x;
	eint    old_offset_y;
	ebool    is_bar;
	eHandle owner;
	MenuPositionCB cb;	
};

struct _GuiMenuShell {
	eHandle root_menu;
	eHandle active;
	eHandle enter;
	eHandle source;
};

struct _GuiMenuRadio {
	eHandle selected;
};

typedef enum {
	MenuItemDefault   = 0,
	MenuItemCheck     = 1,
	MenuItemRadio     = 2,
	MenuItemBranch    = 3,
	MenuItemSeparator = 4,
	MenuItemTearoff   = 5,
} MenuItemType;

struct _GuiMenuItem {
	MenuItemType type;
	const echar *label;
	const echar *accelkey;
	eint  l_size;
	union {
		GuiMenuRadio *radio;
		eHandle submenu;
		ebool check;
	} p;
};

struct _GuiMenuEntry {
	const echar *path;
	const echar *accelkey;
	const echar *type;

	AccelKeyCB callback;
	ePointer   data;
};

eGeneType egui_genetype_menu(void);
eGeneType egui_genetype_menu_bar(void);
eGeneType egui_genetype_menu_item(void);

eHandle egui_menu_new(eHandle, MenuPositionCB);
void egui_menu_append(eHandle menu, eHandle item);
void egui_menu_popup(eHandle hobj);
eHandle egui_menu_item_new_with_label(const echar *);
void egui_menu_item_set_radio_group(eHandle, eHandle);
void egui_menu_item_set_check(eHandle, ebool);
void egui_menu_item_set_submenu(eHandle, eHandle);
void egui_menu_item_radio_select(eHandle);
void egui_menu_add_separator(eHandle hobj);
eHandle egui_menu_bar_new(void);
eHandle menu_bar_new_with_entrys(eHandle, GuiMenuEntry *, eint);
void egui_menu_item_set_accelkey(eHandle, eHandle, const echar *, AccelKeyCB, ePointer);

#endif
