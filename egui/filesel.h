#ifndef __GUI_FILESEL_H__
#define __GUI_FILESEL_H__

#include <elib/elib.h>
#include <egui/gui.h>

#define GTYPE_FILESEL			(egui_genetype_filesel())

#define GUI_FILESEL_DATA(hobj)	((GuiFilesel *)e_object_type_data((eHandle)(hobj), GTYPE_FILESEL))

#define MAX_PATH				256

typedef struct _GuiFilesel GuiFilesel;

struct _GuiFilesel {
	eHandle vbox;
	eHandle clist_hbox;
	eHandle sepbar;
	eHandle addr_hbox;
	eHandle bn_hbox;
	eHandle ddlist;
	eHandle hide_bn;
	eHandle dl_hbox;
	eHandle location;
	eHandle up_bn;
	eHandle addr;
	eHandle locat_list;
	eHandle clist;
	eHandle ok_bn, cancel_bn;

	eint old_x;
	bool grab;
	bool is_hide;
	echar path[MAX_PATH];
	eint len;
	GalCursor cursor;
};

eGeneType egui_genetype_filesel(void);
eHandle   egui_filesel_new(void);
const echar *filesel_get_hlight_filename(eHandle);

#endif
