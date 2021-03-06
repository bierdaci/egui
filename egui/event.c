#include "egui.h"
#include "event.h"

esig_t event_signal_keydown       = 0;
esig_t event_signal_keyup         = 0;
esig_t event_signal_mousemove     = 0;
esig_t event_signal_enter         = 0;
esig_t event_signal_leave         = 0;
esig_t event_signal_lbuttondown   = 0;
esig_t event_signal_lbuttonup     = 0;
esig_t event_signal_rbuttondown   = 0;
esig_t event_signal_rbuttonup     = 0;
esig_t event_signal_mbuttondown   = 0;
esig_t event_signal_mbuttonup     = 0;
esig_t event_signal_wheelforward  = 0;
esig_t event_signal_wheelbackward = 0;
esig_t event_signal_ime_input     = 0;
esig_t event_signal_focus_in      = 0;
esig_t event_signal_focus_out     = 0;
esig_t event_signal_clicked       = 0;
esig_t event_signal_2clicked      = 0;

static void event_init_gene(eGeneType new);

eGeneType egui_genetype_event(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			sizeof(GuiEventOrders),
			NULL, 0, NULL, NULL,
			event_init_gene,
		};

		gtype = e_register_genetype(&info, GTYPE_GUI, NULL);
	}
	return gtype;
}

static void event_init_gene(eGeneType new)
{
	event_signal_keydown = e_signal_new("keydown",
			new,
			STRUCT_OFFSET(GuiEventOrders, keydown),
			efalse, 0, "%p");
	event_signal_keyup = e_signal_new("keyup",
			new,
			STRUCT_OFFSET(GuiEventOrders, keyup),
			efalse, 0, "%p");
	event_signal_mousemove = e_signal_new("mousemove",
			new,
			STRUCT_OFFSET(GuiEventOrders, mousemove),
			efalse, 0, "%p");
	event_signal_lbuttondown = e_signal_new("lbuttondown",
			new,
			STRUCT_OFFSET(GuiEventOrders, lbuttondown),
			efalse, 0, "%p");
	event_signal_lbuttonup = e_signal_new("lbuttonup",
			new,
			STRUCT_OFFSET(GuiEventOrders, lbuttonup),
			efalse, 0, "%p");
	event_signal_rbuttondown = e_signal_new("rbuttondown",
			new,
			STRUCT_OFFSET(GuiEventOrders, rbuttondown),
			efalse, 0, "%p");
	event_signal_rbuttonup = e_signal_new("rbuttonup",
			new,
			STRUCT_OFFSET(GuiEventOrders, rbuttonup),
			efalse, 0, "%p");
	event_signal_mbuttondown = e_signal_new("mbuttondown",
			new,
			STRUCT_OFFSET(GuiEventOrders, mbuttondown),
			efalse, 0, "%p");
	event_signal_mbuttonup = e_signal_new("mbuttonup",
			new,
			STRUCT_OFFSET(GuiEventOrders, mbuttonup),
			efalse, 0, "%p");
	event_signal_wheelforward = e_signal_new("wheelforward",
			new,
			STRUCT_OFFSET(GuiEventOrders, wheelforward),
			efalse, 0, "%p");
	event_signal_wheelbackward = e_signal_new("wheelbackward",
			new,
			STRUCT_OFFSET(GuiEventOrders, wheelbackward),
			efalse, 0, "%p");
	event_signal_ime_input = e_signal_new("imeinput",
			new,
			STRUCT_OFFSET(GuiEventOrders, imeinput),
			efalse, 0, "%p");
	event_signal_focus_in = e_signal_new("focus_in",
			new,
			STRUCT_OFFSET(GuiEventOrders, focus_in),
			efalse, 0, NULL);
	event_signal_focus_out = e_signal_new("focus_out",
			new,
			STRUCT_OFFSET(GuiEventOrders, focus_out),
			efalse, 0, NULL);
	event_signal_enter = e_signal_new("enter",
			new,
			STRUCT_OFFSET(GuiEventOrders, enter),
			efalse, 0, "%d%d");
	event_signal_leave = e_signal_new("leave",
			new,
			STRUCT_OFFSET(GuiEventOrders, leave),
			efalse, 0, NULL);
	event_signal_clicked = e_signal_new("clicked",
			new,
			STRUCT_OFFSET(GuiEventOrders, clicked),
			efalse, 0, "%p");
	event_signal_2clicked = e_signal_new("2clicked",
			new,
			STRUCT_OFFSET(GuiEventOrders, dbclicked),
			efalse, 0, "%p");
}

