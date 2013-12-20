#include <stdio.h>

#include <egui/egui.h>

#include "image.h"
#define _WIDTH 10
#define _HIGH 20
#define B_SIZE 20
void clear_block(void);
static GalImage *img;
eHandle image;
static int x = 0;
static int y = 0;
static int change = 0;
bool key_down = false;

struct block_struct {
	int w, h;
	char block[4][4];
};

struct change_block_s {
	int n;
	struct block_struct *blocks[4];
};

char block_size[_HIGH][_WIDTH] = {{0}};
struct change_block_s *cur_block;

struct block_struct block1 = 
{2, 2,
	{{1,1,0,0},
	 {1,1,0,0},
	 {0,0,0,0},
	 {0,0,0,0}}
};

struct change_block_s t_block1 = {1, {&block1}};

struct block_struct block2_1 = 
{2, 3,
	{{1,0,0,0},
	 {1,1,0,0},
	 {0,1,0,0},
	 {0,0,0,0}}
};

struct block_struct block2_2 = 
{3, 2,
	{{0,1,1,0},
	 {1,1,0,0},
	 {0,0,0,0},
	 {0,0,0,0}}
};

struct change_block_s t_block2 = {2, {&block2_1, &block2_2}};

struct block_struct block3_1 = 
{2, 3,
	{{0,1,0,0},
	 {0,1,0,0},
	 {1,1,0,0},
	 {0,0,0,0}},
};

struct block_struct block3_2 = 
{3, 2,
	{{1,0,0,0},
	 {1,1,1,0},
	 {0,0,0,0},
	 {0,0,0,0}},
};

struct block_struct block3_3 = 
{2, 3,
	{{1,1,0,0},
	 {1,0,0,0},
	 {1,0,0,0},
	 {0,0,0,0}},
};

struct block_struct block3_4 = 
{3, 2,
	{{1,1,1,0},
	 {0,0,1,0},
	 {0,0,0,0},
	 {0,0,0,0}},
};

struct change_block_s t_block3 = {4, {&block3_1, &block3_2, &block3_3, &block3_4}};

struct block_struct block4_1 =
{1, 4,
	{{1,0,0,0},
	 {1,0,0,0},
	 {1,0,0,0},
	 {1,0,0,0}}
};

struct block_struct block4_2 =
{4, 1,
	{{1,1,1,1},
	 {0,0,0,0},
	 {0,0,0,0},
	 {0,0,0,0}}
};


struct change_block_s t_block4 = {2, {&block4_1, &block4_2}};

struct block_struct block5_1 = 
{3, 2,
	{{0,1,0,0},
	 {1,1,1,0},
	 {0,0,0,0},
	 {0,0,0,0}}
};

struct block_struct block5_2 = 
{2, 3,
	{{1,0,0,0},
	 {1,1,0,0},
	 {1,0,0,0},
	 {0,0,0,0}}
};

struct block_struct block5_3 = 
{2, 3,
	{{1,1,1,0},
	 {0,1,0,0},
	 {0,0,0,0},
	 {0,0,0,0}}
};

struct block_struct block5_4 = 
{2, 3,
	{{0,1,0,0},
	 {1,1,0,0},
	 {0,1,0,0},
	 {0,0,0,0}}
};

struct change_block_s t_block5 = {5, {&block5_1, &block5_2, &block5_3, &block5_4}};

void draw_block(int line, int row, bool T)
{
	GuiWidget *wid = GUI_WIDGET_DATA(image);

	if (T)
		egal_set_foreground(wid->pb, 0x00ffdd);
	else
		egal_set_foreground(wid->pb, 0x0);

	egal_fill_rect(wid->drawable, wid->pb, line * B_SIZE, row * B_SIZE, B_SIZE, B_SIZE);

}

void draw_test(struct block_struct *bs, int x, int y, bool T)
{
	int i, j;
	for (j = 0; j < 4; j++)
	{
		for (i = 0; i < 4; i++)
		{   
			if (bs->block[j][i])
				draw_block(i+x, j+y, T);
		}
	}
}

bool check_crack_x(int lr)
{

	int i, j;
	for (i = 0; i < cur_block->blocks[change]->h; i++) {
		for (j = 0; j < cur_block->blocks[change]->w; j++)
		 	if (block_size[i+y][j+x+lr] && cur_block->blocks[change]->block[i][j])
				return true;
	}
	return false;

}

bool check_crack_y(void)
{
	int i, j;
	for (i = 0; i < cur_block->blocks[change]->h; i++) {
		for (j = 0; j < cur_block->blocks[change]->w; j++)
		 	if (block_size[i+y+1][j+x] && cur_block->blocks[change]->block[i][j])
				return true;
	}
	return false;
}

static eint timer_cb(eTimer timer, euint num, ePointer args)
{
    static int  count = 0;	
	bool b = false;
	int i, j;


	count += num;
	draw_test(cur_block->blocks[change], x, y, false);

	
	if (y + cur_block->blocks[change]->h == _HIGH || check_crack_y())
		b = true;

	if (!b)
	{
		if (key_down)
			y += num;
	    else if (count > 10)
		{
			y++;
			count = 0;
		}
	}
	draw_test(cur_block->blocks[change], x, y, true);
	if (b) {
		int _w = cur_block->blocks[change]->w;
		int _h = cur_block->blocks[change]->h;
		for (i = 0; i < _h; i++) 
		{
			for (j = 0; j < _w; j++) {
				if (cur_block->blocks[change]->block[i][j])
					block_size[y+i][x+j] = cur_block->blocks[change]->block[i][j];
			}
		}

        for (i = _HIGH-1; i > 0; i--) {
			for (j = 0; j < _WIDTH; j++ ) {
				if (!block_size[i][j])
					break;
			}

			if (j == _WIDTH) {
				for (; i > 0; i--)
					for (j = 0; j < _WIDTH; j++) {
						block_size[i][j] = block_size[i - 1][j];
					}
				egui_update(image);
			}
		}


		x = 0;
		y = 0;
		change = 0;
	}
	return 0;
}

static eint image_keydown(eHandle hobj, GalEventKey *ent)
{
	int t;
    draw_test(cur_block->blocks[change], x, y, false);
	if (ent->code == GAL_KC_Down) {
		key_down = true;
	}
	else if (ent->code == GAL_KC_Right) {
		if (x + cur_block->blocks[change]->w < _WIDTH && !check_crack_x(1))
			x++;
	}
	else if (ent->code == GAL_KC_Up) {
		t = change + 1;
		t %= cur_block->n;
	   if (x + cur_block->blocks[t]->w <= _WIDTH )
		  change = t;
	}
	else if (ent->code == GAL_KC_Left) {
	   if (x > 0 && !check_crack_x(-1))
		   x--;
	}

	draw_test(cur_block->blocks[change], x, y, true);

	return -1;
}

static eint image_keyup(eHandle hobj, GalEventKey *ent)
{

	key_down = false;
	return -1;
}


static eint bn_clicked(eHandle hobj, ePointer data)
{  
	static int ox = 0;
	static int oy = 0;

	GuiWidget *wid = GUI_WIDGET_DATA((eHandle)data);
	
	draw_block(x, y, true);
	egal_set_foreground(wid->pb, 0x00ffdd);
	egal_fill_rect(wid->drawable, wid->pb, x, y, 20 ,20);
	if (ox != x || oy != y)
	{
		egal_set_foreground(wid->pb, 0x0);
		egal_fill_rect(wid->drawable, wid->pb, ox, oy, 20, 20);
		draw_block(ox, oy, false);

		ox = x;
		oy = y;
	}

	if (y  < _HIGH)
		y += 1;
	else {
	    x +=  1;
		y = 0;
	}

	egui_set_focus(image);
	return 0;
}

void  clear_block(void)
{
	
	GuiWidget *wid = GUI_WIDGET_DATA(image);
	egal_set_foreground(wid->pb, 0x0);
	egal_fill_rect(wid->drawable, wid->pb, wid->rect.x, wid->rect.y, wid->rect.w, wid->rect.h);
}

static eint image_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *ent)
{
	int i, j;
	egal_set_foreground(ent->pb, 0x0);
	egal_fill_rect(wid->drawable, wid->pb, ent->rect.x, ent->rect.y, ent->rect.w, ent->rect.h);
	
 	for (i = 0; i < _HIGH; i++){
		for (j = 0; j < _WIDTH; j++)
		{
			if (block_size[i][j])
			{
				draw_block(j, i, true);
			}
		}
	}

	return 0;
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, label, bn, hbox;

	egui_init(argc, argv);

	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_START);
	egui_set_expand(vbox, true);
	egui_box_set_spacing(vbox, 20);
	egui_add(win, vbox);

	label = egui_simple_label_new(_("Russian Block"));
	egui_add(vbox, label);

    image  = egui_image_new(_WIDTH * B_SIZE, _HIGH * B_SIZE);
	egui_add(vbox, image);
	egui_set_focus(image);
    e_signal_connect(image, SIG_EXPOSE, image_expose);
    e_signal_connect(image, SIG_KEYDOWN, image_keydown);
   
    e_signal_connect(image, SIG_KEYUP, image_keyup);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_SPREAD);
	egui_set_expand_h(hbox, true);
	egui_box_set_spacing(hbox, 20);
	egui_add(vbox,  hbox);
	
  	bn = egui_button_new(80, 35);
	e_signal_connect2(bn, SIG_CLICKED, bn_clicked, (ePointer)image,(ePointer)1);
	egui_add(hbox, bn);

  	bn = egui_button_new(80, 35);
	e_signal_connect2(bn, SIG_CLICKED, bn_clicked, (ePointer)image, (ePointer)2);
	egui_add(hbox, bn);
   
	img = egal_image_new_from_file("3.gif");
	if(!img){
		printf("...............load \"3.gif\" failed\n");
		return 1;
	}


    e_timer_add(100, timer_cb, 0);
	cur_block = &t_block3;
	memset(block_size, 0, sizeof(block_size));
	
	egui_main();

	return 0;
}
