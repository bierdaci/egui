#include <stdio.h>

#include <elib/elib.h>

#define GTYPE_WIDGET	                (egui_genetype_widget())
#define GTYPE_EVENT	                    (egui_genetype_event())
#define GTYPE_HOOK 	                    (egui_genetype_hook())
#define GTYPE_BIN		                (egui_genetype_bin())
#define GTYPE_BOX		                (egui_genetype_box())
#define GTYPE_ADJUST		            (egui_genetype_adjust())
#define GTYPE_HOOK_BOX					(egui_genetype_hook_box())
#define GTYPE_SCROLLWIN                 (egui_genetype_scrollwin())
#define GTYPE_MENU_SCROLLWIN            (egui_genetype_menu_scrollwin())

eGeneType egui_genetype_widget(void)
{
    static eGeneType gtype = 0;

    if (!gtype) {
        eGeneInfo info = {
			//"widget",
            0, NULL, 0, NULL, NULL, NULL,
        };

        gtype = e_register_genetype(&info, GTYPE_CELL, NULL);
    }
    return gtype;
}


eGeneType egui_genetype_event(void)
{
    static eGeneType gtype = 0;

    if (!gtype) {
        eGeneInfo info = {
			//"event",
            0, NULL, 0, NULL, NULL, NULL,
        };

        gtype = e_register_genetype(&info, GTYPE_CELL, NULL);
    }
    return gtype;
}

eGeneType egui_genetype_hook(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			//"hook",
            0, NULL, 0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_CELL, NULL);
	}
	return gtype;
}


eGeneType egui_genetype_bin(void)
{
    static eGeneType gtype = 0;

    if (!gtype) {
        eGeneInfo info = {
			//"bin",
            0, NULL, 0, NULL, NULL, NULL,
        };

        gtype = e_register_genetype(&info, GTYPE_WIDGET, GTYPE_EVENT, NULL);
    }
    return gtype;
}

eGeneType egui_genetype_box(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			//"box",
            0, NULL, 0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

eGeneType egui_genetype_adjust(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			//"adjust",
            0, NULL, 0, NULL, NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

static eGeneType egui_genetype_hook_box(void)
{ 
    static eGeneType gtype = 0;    
  
    if (!gtype) {
        eGeneInfo info = {
			//"hook-box",
            0, NULL, 0, NULL, NULL, NULL,
        };
  
        gtype = e_register_genetype(&info, GTYPE_HOOK, GTYPE_ADJUST, GTYPE_BOX, NULL);
    }
    return gtype;
} 

eGeneType egui_genetype_scrollwin(void)
{
    static eGeneType gtype = 0;

    if (!gtype) {
        eGeneInfo info = {
			//"scrollwin",
            0, NULL, 0, NULL, NULL, NULL,
        };

        gtype = e_register_genetype(&info, GTYPE_BIN, GTYPE_HOOK, NULL);
    }
    return gtype;
}

typedef struct {
	eint i, j;
} MenuScrollData;

typedef struct {
	eint (*set_font)(eHandle, MenuScrollData *, eint, edouble, ellong, efloat, eullong, efloat,  efloat,  efloat,  efloat,  efloat,  efloat,  efloat, edouble);
	void (*set_color)(eHandle, eint);
} MenuScrollOrders;

static eint test_cb(eHandle hobj, eHandle  h1, eHandle h2, eHandle h3, eHandle h4, int i1, int i2, int i3, int i4)
{
	printf("%lx  %lx  %lx  %lx %d  %d  %d  %d\n", h1, h2, h3, h4, i1, i2, i3, i4);
	return 0;
}

static void widget_init_orders(eGeneType new, ePointer this)
{
	MenuScrollOrders *ms = this;
	ms->set_font = test_cb;
}

static eGeneType egui_genetype_menu_scrollwin(void)
{ 
    static eGeneType gtype = 0;    
  
    if (!gtype) {
        eGeneInfo info = {
            sizeof(MenuScrollOrders),
			widget_init_orders,
			sizeof(MenuScrollData),
			NULL, NULL, NULL,
        };
  
        gtype = e_register_genetype(&info, GTYPE_SCROLLWIN, GTYPE_BOX, NULL);
    }
    return gtype;
} 

int main(void)
{
	eHandle obj1 = e_object_new(GTYPE_MENU_SCROLLWIN);
	//eHandle obj2 = e_object_new(GTYPE_HOOK_BOX);
	//printf("check box  %d\n", e_object_type_check(hobj, GTYPE_BOX));

	//esig_t sig = e_signal_new("set_font",
	//		GTYPE_SCROLLWIN,
	//		STRUCT_OFFSET(MenuScrollOrders, set_font),
	//		true, 0, "%n %n %n %p %d %d %d %d");
	esig_t sig = e_signal_new_label("aaaa", GTYPE_SCROLLWIN, "%n %n %n %p %d %d %d %d");


	e_signal_connect(obj1, sig, test_cb);

	//printf("obj1 check box  %d\n", e_object_type_check(obj1, GTYPE_BOX));
	//printf("obj2 check box  %d\n", e_object_type_check(obj2, GTYPE_BOX));

	e_signal_emit(obj1, sig, 1, 2, 3, 4, 5, 6, 7, 8);

	return 0;
}
