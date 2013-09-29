#include <egui/egui.h>

static eHandle chat_win_new();

static GuiMenuEntry menu_entrys[] = {
	{ _("/Menu"),          _("<ALT>F"),  _("<Branch>"),       NULL,           NULL},
	{ _("/Menu/New"),      _("<CTRL>N"), _("<StockItem>"),    NULL,           "ctrl+n"},
	{ _("/Menu/Open"),     NULL,         _("<Branch>"),       NULL,           NULL},
};

static eint open_chat_win(eHandle hobj, ePointer data)
{
	eHandle chat = (eHandle)data;
	if (GUI_STATUS_VISIBLE(chat))
		egui_hide(chat, false);
	else
		egui_show(chat, false);
	return 0;
}

int main(int argc, const char *argv[])
{
	eHandle win, vbox, bar, lv, chat;
	const echar *titles[1] = {_("")};

	egui_init(argc, argv);

	win  = egui_window_new();
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	chat = chat_win_new();
	bar  = menu_bar_new_with_entrys(win, menu_entrys, sizeof(menu_entrys) / sizeof(GuiMenuEntry));

	egui_add(vbox, bar);

	lv = egui_clist_new(titles, 1);
	egui_set_min(lv, 150, 200);
	e_signal_connect(lv, SIG_2CLICKED, open_chat_win, (ePointer)chat);
	egui_clist_title_hide(lv);
	egui_request_resize(lv, 150, 400);
	clist_vsbar_set_auto(lv, true);
	egui_add(vbox, lv);

	egui_main();
	return 0;
}

struct _ChatWin {
	eHandle view_text;
	eHandle edit_text;
};

eGeneType egui_genetype_chat_win(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0, NULL,
			0, NULL,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_WINDOW, NULL);
	}
	return gtype;
}

static eHandle chat_win_new()
{
	eHandle win, vbox, hbox, view_text, edit_text, bn;
	
	win = egui_window_new();
	vbox = egui_vbox_new();
	egui_set_expand(vbox, true);
	egui_add(win, vbox);

	view_text = egui_text_new(450, 200);
	edit_text = egui_text_new(300, 200);
	egui_add(vbox, view_text);
	egui_add_spacing(vbox, 20);
	egui_add(vbox, edit_text);
	hbox = egui_hbox_new();
	bn  = egui_label_button_new(_("send"));
	egui_add(vbox, bn);
	egui_add(hbox, bn);
	egui_add(vbox, hbox);

	egui_text_set_only_read(view_text, true);
//	egui_hide(win, false);

	return win;
}

