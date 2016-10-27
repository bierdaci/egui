#include <egui/egui.h>

static eint print_hello(eHandle hobj, ePointer data)
{
	printf("%s\n", (char *)data);
	return 0;
}

static eint print_toggle(eHandle hobj, ePointer data)
{
	printf("toggle\n");
	return 0;
}

static eint print_selected(eHandle hobj, ePointer data)
{
	printf("selected %d\n", (eint)data);
	return 0;
}

static GuiMenuEntry menu_entrys[] = {
	{ _("/File"),          _("<ALT>F"),  _("<Branch>"),       NULL,           NULL},
	{ _("/File/New"),      _("<CTRL>N"), _("<StockItem>"),    print_hello,    "ctrl+n"},
	{ _("/File/Open"),     NULL,         _("<Branch>"),       NULL,           NULL},
	{ _("/File/Open/A"),   _("<CTRL>A"), _("<Item>"),         print_hello,    "ctrl+a"},
	{ _("/File/Open/B"),   _("<CTRL>B"), _("<Item>"),         print_hello,    "ctrl+b"},
	{ _("/File/Save"),     _("<CTRL>S"), _("<Item>"),         print_hello,    "ctrl+s"},
	{ _("/File/Save As"),  NULL,         _("<Item>"),         NULL,           NULL},
	{ _("/File/sep1"),     NULL,         _("<Separator>"),    NULL,           NULL},
	{ _("/File/Quit"),     _("<CTRL>Q"), _("<Item>"),         print_hello,    "ctrl+q"},
	{ _("/Options"),       _("<ALT>O"),  _("<Branch>"),       NULL,           NULL},
	{ _("/Options/tear"),  NULL,         _("<Tearoff>"),      NULL,           NULL},
	{ _("/Options/Check"), NULL,         _("<Check>"),        print_toggle,   NULL},
	{ _("/Options/sep"),   NULL,         _("<Separator>"),    NULL,           NULL},
	{ _("/Options/Rad1"),  NULL,         _("<RadioItem>"),    print_selected, (ePointer)1},
	{ _("/Options/Rad2"),  NULL,         _("/Options/Rad1"),  print_selected, (ePointer)2},
	{ _("/Options/Rad3"),  NULL,         _("/Options/Rad1"),  print_selected, (ePointer)3},
	{ _("/Help"),          _("<ALT>H"),  _("<Branch>"),       NULL,           NULL},
	{ _("/Help/About"),    NULL,         _("<Item>"),         NULL,           NULL},
};

int main(int argc, char *const argv[])
{
	eHandle win, vbox, bar, button;
	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	vbox = egui_vbox_new();
	egui_set_expand(vbox, etrue);
	egui_add(win, vbox);

	bar = menu_bar_new_with_entrys(win, menu_entrys, sizeof(menu_entrys) / sizeof(GuiMenuEntry));
	egui_add(vbox, bar);

	button = egui_button_new(200, 50);
	egui_add(vbox, button);

	egui_main();
	return 0;
}
