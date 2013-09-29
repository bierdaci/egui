#include <stdio.h>

#include <egui/egui.h>
#include <egal/pixbuf.h>


static void bn_insert_text(eHandle hobj, ePointer data)
{
	const echar *str = _("\n.....你好啊，我们 好\n哈保哈哈........\n");
	egui_insert_text((eHandle)data, str, e_strlen(str));
}

static void bn_clear_text(eHandle hobj, ePointer data)
{
	//static bool only = false;
	//only = !only;
	//egui_text_set_only_read((eHandle)data, only);
	egui_text_clear((eHandle)data);
}

static eint bn_underlines(eHandle hobj, ePointer data)
{
	egui_text_set_underline((eHandle)data);
	return 0;
}

static eint bn_hscrollbar(eHandle hobj, ePointer data)
{
	if (GUI_STATUS_VISIBLE(data))
		egui_hide((eHandle)data, true);
	else
		egui_show((eHandle)data, true);
	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, hbox, vbox1, text;
	eHandle vscrollbar, hscrollbar;
	eHandle hbox1, bn1, bn2, bn3, bn4;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	egui_request_resize(win, 500, 500);
	e_signal_connect(win, SIG_DESTROY, egui_quit);
	vbox = egui_vbox_new();
	egui_box_set_align(vbox, BoxAlignEnd);
	egui_box_set_spacing(vbox, 10);
	egui_box_set_layout(vbox, BoxLayout_SPREAD);
	egui_set_expand(vbox, true);

	vbox1 = egui_vbox_new();
	egui_set_expand(vbox1, true);
	text = egui_text_new(400, 400);
	vscrollbar = egui_vscrollbar_new(true);
	hscrollbar = egui_hscrollbar_new(true);
	egui_hook_v(text, vscrollbar);

	egui_add(vbox1, text);
	egui_add(vbox1, hscrollbar);

	hbox = egui_hbox_new();
	egui_set_expand(hbox, true);

	egui_add(hbox, vbox1);
	egui_add(hbox, vscrollbar);

	hbox1 = egui_hbox_new();
	egui_box_set_spacing(hbox1, 10);
	bn1 = egui_label_button_new(_("insert"));
	bn2 = egui_label_button_new(_("clear"));
	bn3 = egui_label_button_new(_("unline"));
	bn4 = egui_label_button_new(_("hide_h"));
	egui_add(hbox1, bn1);
	egui_add(hbox1, bn2);
	egui_add(hbox1, bn3);
	egui_add(hbox1, bn4);

	egui_add(vbox, hbox);
	egui_add(vbox, hbox1);
	egui_add(win, vbox);

	e_signal_connect1(bn1, SIG_CLICKED, bn_insert_text, (ePointer)text);
	e_signal_connect1(bn2, SIG_CLICKED, bn_clear_text, (ePointer)text);
	e_signal_connect1(bn3, SIG_CLICKED, bn_underlines, (ePointer)text);
	e_signal_connect1(bn4, SIG_CLICKED, bn_hscrollbar, (ePointer)hscrollbar);

	FILE *infile = fopen("a.c", "r");
	if (infile) {
		echar buffer[1024];
		eint nchars;

		while (1) {
			nchars = fread(buffer, 1, 1024, infile);
			egui_text_append(text, buffer, nchars);

			if (nchars < 1024)
				break;
		}

		fclose(infile);
	}

	egui_main();

	return 0;
}
