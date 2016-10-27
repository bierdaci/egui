#ifndef __GUI_COLLIST_H
#define __GUI_COLLIST_H

#include  <elib/elib.h>

#define GTYPE_CLIST  						(egui_genetype_clist())

#define GUI_CLIST_ORDERS(hobj)				((GuiClistOrders *)e_object_type_orders(hobj, GTYPE_CLIST))
#define GUI_CLIST_DATA(hobj)				((GuiClist       *)e_object_type_data(hobj, GTYPE_CLIST))

#define SIG_CLIST_CMP						signal_clist_cmp
#define SIG_CLIST_FIND						signal_clist_find
#define SIG_CLIST_UPDATE					signal_clist_update
#define SIG_CLIST_INSERT					signal_clist_insert
#define SIG_CLIST_DRAW_TITLE				signal_clist_draw_title
#define SIG_CLIST_DRAW_GRID					signal_clist_draw_grid
#define SIG_CLIST_DRAW_GRID_BK				signal_clist_draw_grid_bk

#define TITLE_MAX							50

typedef struct _GuiClist         GuiClist;
typedef struct _GuiClistOrders   GuiClistOrders;
typedef struct _ClsItemBar       ClsItemBar;
typedef struct _ClsTitle         ClsTitle;
typedef struct _ClsItemGrid      ClsItemGrid;

typedef enum {
	ClsGridText,
	ClsGridImage,
} ClsGridType;

typedef enum {
	ClsGridCenter = 0,
	ClsGridLeft   = 1,
	ClsGridRight  = 2,
} ClsGridDirect;

typedef struct {
	ClsGridType type;
	const echar *name;
} ClistTitle;

struct _ClsTitle {
	echar name[TITLE_MAX];
	eint  width;
	eint  fixed_w;
};

struct _ClsItemGrid {
	echar     *chars;
	GalImage  *icon;
};

struct _ClsItemBar {
	euint       status;
	eint        index;
	list_t      list;
	ePointer    add_data;
	ClsItemGrid grids[0];
};

struct _GuiClist {
	eHandle vbox;
	eHandle tbar;
	eHandle view;
	eHandle vsbar;
	eHandle hsbar;

	eint view_w, view_h;
	eint tbar_h, ibar_h;
	eint col_n;

	eint offset_x, offset_y;
	eint old_oy;
	eint item_n;

	GalDrawable drawable;
	GalPB       pb;
	ClsTitle    *titles;
	ClsItemBar  *top;
	ClsItemBar  *hlight;
#ifdef __TOUCHSCREEN
	ClsItemBar  *touch_sel;
#endif
	list_t item_head;

	eHandle     menu;
	GalFont     font;
	ePointer    clicked_args;
	ePointer    sel_args;
	eHandle     vadj, hadj;
	eint        ck_num;
	euint32     ck_time;
	ClsItemBar *ck_ibar;
	ebool vsbar_auto;
	ebool hsbar_auto;
	void (*clear)(ClsItemBar *);
};

struct _GuiClistOrders {
	eint (*cmp)(eHandle, ePointer, ClsItemBar *, ClsItemBar *);
	eint (*insert)(eHandle, GuiClist *, ClsItemBar *);
	eint (*update)(eHandle, GuiClist *, ClsItemBar *);
	void (*draw_title)(eHandle, GuiClist *, GalDrawable, GalPB);
	void (*draw_grid)(eHandle, GalDrawable, GalPB, GalFont, ClsItemBar *, int, int, int, int);
	void (*draw_grid_bk)(eHandle, GuiClist *, GalDrawable, GalPB, int, int, int, ebool, ebool, int);
	eint (*find)(eHandle, ClsItemBar *, ePointer);
};

eGeneType egui_genetype_clist(void);
eHandle egui_clist_new(const echar *[], eint);
ClsItemBar *egui_clist_insert(eHandle, ClsItemGrid *);
ClsItemBar *egui_clist_hlight_insert(eHandle, ClsItemGrid *);
ClsItemBar *egui_clist_insert_valist(eHandle, ...);
ClsItemBar *egui_clist_hlight_insert_valist(eHandle, ...);
ClsItemBar *egui_clist_append(eHandle, ClsItemGrid *);
ClsItemBar *egui_clist_append_valist(eHandle, ...);
ClsItemBar *egui_clist_find(eHandle, ePointer);
ClsItemBar *egui_clist_get_hlight(eHandle);
ClsItemBar *egui_clist_get_next(eHandle, ClsItemBar *);
ClsItemBar *egui_clist_get_first(eHandle);
ClsItemBar *egui_clist_get_last(eHandle);
ClsItemBar *egui_clist_ibar_new(eHandle, ClsItemGrid *);
ClsItemBar *egui_clist_ibar_new_valist(eHandle, ...);
void egui_clist_update_ibar(eHandle, ClsItemBar *);
void egui_clist_set_hlight(eHandle, ClsItemBar *);
void egui_clist_unset_hlight(eHandle);
void egui_clist_insert_ibar(eHandle, ClsItemBar *);
void egui_clist_delete_hlight(eHandle);
void egui_clist_delete(eHandle, ClsItemBar *);
void egui_clist_empty(eHandle);
void egui_clist_title_show(eHandle);
void egui_clist_title_hide(eHandle);
void egui_clist_set_column_width(eHandle, eint, eint);
void egui_clist_set_item_height(eHandle, eint);
void egui_clist_set_clear(eHandle, void (*)(ClsItemBar *));
ePointer clist_get_grid_data(eHandle, ClsItemBar *, eint);
void egui_clist_set_menu(eHandle hobj, eHandle menu);

extern esig_t signal_clist_cmp;
extern esig_t signal_clist_find;
extern esig_t signal_clist_insert;
extern esig_t signal_clist_update;
extern esig_t signal_clist_draw_title;
extern esig_t signal_clist_draw_grid;
extern esig_t signal_clist_draw_grid_bk;

#endif
