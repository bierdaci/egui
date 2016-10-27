#include "widget.h"
#include "bin.h"
#include "fixed.h"
#include "gui.h"

static void fixed_init_orders(eGeneType, ePointer);

eGeneType egui_genetype_fixed(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		eGeneInfo info = {
			0, fixed_init_orders,
			0, NULL,
			NULL, NULL,
		};

		gtype = e_register_genetype(&info, GTYPE_BIN, NULL);
	}
	return gtype;
}

static eint fixed_init(eHandle hobj, eValist vp)
{
	GuiWidget *wid = GUI_WIDGET_DATA(hobj);
	eint w = e_va_arg(vp, eint);
	eint h = e_va_arg(vp, eint);

	wid->rect.w = w;
	wid->rect.h = h;
	wid->max_w  = 0;
	wid->max_h  = 0;

	return 0;
}

static void fixed_init_orders(eGeneType new, ePointer data)
{
	eCellOrders *c = e_genetype_orders(new, GTYPE_CELL);
	c->init  = fixed_init;
}

eHandle egui_fixed_new(int w, int h)
{
	return e_object_new(GTYPE_FIXED, w, h);
}

/*
static eHandle fixed_child_switch(GuiBin *bin, eHandle child, eint dir, eint);

static eHandle child_next(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->next;
	else
		cw = bin->head;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eHandle child_prev(GuiBin *bin, eHandle cobj)
{
	GuiWidget *cw;

	if (cobj)
		cw = GUI_WIDGET_DATA(cobj)->prev;
	else
		cw = bin->tail;

	if (cw) return OBJECT_OFFSET(cw);

	return 0;
}

static eHandle fixed_child_switch(GuiBin *bin, eHandle child, eint dir, eint axis)
{
	ebool loop;
	eHandle near = 0;
	eint diff = 0xffff;
	eint min  = 0xffff;
	eint init_x, init_y;

	GuiWidget *cw;
	eHandle (*cb)(GuiBin *, eHandle);

	if (dir == BIN_DIR_NEXT)
		cb = child_next;
	else
		cb = child_prev;

	if (child) {
		cw = GUI_WIDGET_DATA(child);
		if (dir == BIN_DIR_NEXT) {
			init_x = cw->rect.x + cw->rect.w;
			init_y = cw->rect.y + cw->rect.h;
		}
		else {
			init_x = cw->rect.x;
			init_y = cw->rect.y;
		}
	}
	else {
		cw = GUI_WIDGET_DATA(OBJECT_OFFSET(bin));
		if (dir == BIN_DIR_NEXT) {
			init_x = cw->rect.x;
			init_y = cw->rect.y;
		}
		else {
			init_x = cw->rect.x + cw->rect.w;
			init_y = cw->rect.y + cw->rect.h;
		}
	}

	do {
		child = cb(bin, child);
		if (!child)
			break;

		cw = GUI_WIDGET_DATA(child);
		loop = !bin_can_focus(cw);
		if (!loop && axis > 0) {
			eint d = 0;
			if (dir == BIN_DIR_NEXT) {
				if (axis & BIN_X_AXIS)
					d = (axis & 0xffff) - (cw->rect.x + cw->rect.w);
				else if (axis & BIN_Y_AXIS)
					d = (axis & 0xffff) - (cw->rect.y + cw->rect.h);

				if (d < 0) {
					if (axis & BIN_X_AXIS)
						d += cw->rect.w;
					else if (axis & BIN_Y_AXIS)
						d += cw->rect.h;
					if (d < 0) d = -d;
				}

				if (axis & BIN_X_AXIS) {
					if (cw->rect.y > init_y && cw->rect.y < min) {
						diff = d;
						near = child;
						min  = cw->rect.y;
					}
					else if (cw->rect.y == min && d < diff) {
						diff = d;
						near = child;
					}
				}
				else if (axis & BIN_Y_AXIS) {
					if (cw->rect.x > init_x && cw->rect.x < min) {
						diff = d;
						near = child;
						min = cw->rect.x;
					}
					else if (cw->rect.x == min && d < diff) {
						diff = d;
						near = child;
					}
				}

				loop = cw->next ? etrue : efalse;
			}
			else {
				if (axis & BIN_X_AXIS)
					d = cw->rect.x - (axis & 0xffff);
				else if (axis & BIN_Y_AXIS)
					d = cw->rect.y - (axis & 0xffff);

				if (d < 0) {
					if (axis & BIN_X_AXIS)
						d += cw->rect.w;
					else if (axis & BIN_Y_AXIS)
						d += cw->rect.h;
					if (d < 0) d = -d;
				}

				if (axis & BIN_X_AXIS) {
					if (cw->rect.y > init_y && cw->rect.y < min) {
						diff = d;
						near = child;
						min  = cw->rect.y;
					}
					else if (cw->rect.y == min && d < diff) {
						diff = d;
						near = child;
					}
				}
				else if (axis & BIN_Y_AXIS) {
					if (cw->rect.x > init_x && cw->rect.x < min) {
						diff = d;
						near = child;
						min = cw->rect.x;
					}
					else if (cw->rect.x == min && d < diff) {
						diff = d;
						near = child;
					}
				}

				loop = cw->prev ? etrue : efalse;
			}
		}

	} while (loop);

	return near ? near : child;
}
*/

