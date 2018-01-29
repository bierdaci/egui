#include <egal/pixbuf.h>
#include <egui/egui.h>

#include <math.h>


static eint bn1_clicked(eHandle hobj, ePointer data)
{
	egui_clist_delete_hlight((eHandle)data);
	//collist_empty(data);
	return 0;
}

static eint bn2_clicked(eHandle hobj, ePointer data)
{
	echar buf1[10];
	static eint i = 0;
	e_sprintf(buf1, _("sert%d"), i++);
	egui_clist_hlight_insert_valist((eHandle)data, buf1, "bbb");
	return 0;
}

static eint bn3_clicked(eHandle hobj, ePointer data)
{
	GuiClist *cl = GUI_CLIST_DATA((eHandle)data);
	if (GUI_STATUS_VISIBLE(cl->tbar)) {
		//egui_hide(cl->hsbar);
		egui_clist_title_hide((eHandle)data);
	}
	else {
		//egui_show(cl->hsbar);
		egui_clist_title_show((eHandle)data);
	}
	return 0;
}

static eint cmp_cb(eHandle hobj, ClsItemBar *ib1, ClsItemBar *ib2)
{
	printf("%s  %s\n", clist_get_grid_data(hobj, ib2, 0), clist_get_grid_data(hobj, ib2, 1));
	return 1;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, hbox, lv, bn1, bn2, bn3;

	const echar *titles[2] = {_("aaa"), _("bbb")};
	int i;
	char buf1[10], buf2[10];
	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	egui_request_resize(win, 400, 400);
	vbox = egui_vbox_new();
	egui_set_expand (vbox, etrue);
	egui_box_set_layout (vbox, BoxLayout_SPREAD);
	egui_box_set_spacing(vbox, 10);

	lv = egui_clist_new(titles, 2);
	egui_clist_set_column_width(lv, 1, 200);
	e_signal_connect(lv, SIG_CLIST_CMP,  cmp_cb);

	hbox  = egui_hbox_new();
	egui_box_set_spacing(hbox, 20);

	bn1   = egui_label_button_new(_("delete"));
	e_signal_connect1(bn1, SIG_CLICKED, bn1_clicked, (ePointer)lv);
	egui_add(hbox, bn1);

	bn2 = egui_label_button_new(_("insert"));
	e_signal_connect1(bn2, SIG_CLICKED, bn2_clicked, (ePointer)lv);
	egui_add(hbox, bn2);

	bn3 = egui_label_button_new(_("hidebar"));
	e_signal_connect1(bn3, SIG_CLICKED, bn3_clicked, (ePointer)lv);
	egui_add(hbox, bn3);

	egui_add(vbox, lv);
	egui_add(vbox, hbox);
	egui_add(win,  vbox);

	for (i = 0; i < 21; i++) {
		sprintf(buf1, "r%d", i);
		sprintf(buf2, "s%d", i + 7);
		egui_clist_append_valist(lv, buf1, buf2);
	}

	egui_main();

	return 0;
}

