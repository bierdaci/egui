#ifndef __GUI_FRAME_H
#define __GUI_FRAME_H

#define GTYPE_FRAME				(egui_genetype_frame())

#define GUI_FRAME_DATA(hobj)	((GuiFrame *)e_object_type_data((hobj), GTYPE_FRAME))

typedef struct _GuiFrame 		GuiFrame;

struct _GuiFrame {
	const echar *title;
	eint title_width;
	GalFont font;
};

eGeneType egui_genetype_frame(void);
eHandle egui_frame_new(const echar *);

#endif
