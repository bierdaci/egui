#include <stdio.h>
#include <string.h>
#include <math.h>

#include <egui/egui.h>

#define GLEW_STATIC

#include <GL/glew.h>
/*
#pragma comment(lib, "glew32s.lib")
#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "elib.lib")
#pragma comment(lib, "egal.lib")
#pragma comment(lib, "egui.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "jpeg.lib")
*/
static GalCursor cursor;
static void path_node_reset(void);

#define NODE_POINT_MAX 256
#define INDEXS_MAX		20

#define ELEMENT_TYPE_UNKNOWN	0
#define ELEMENT_TYPE_PLINE		1
#define ELEMENT_TYPE_POINT		2
#define ELEMENT_TYPE_REGION		3

#define DEG2RAD(a) (a * M_PI) / 180.0F

typedef struct _GisMap GisMap;
typedef struct _GisPoint {
	float x, y;
	struct _GisPoint *next;
} GisPoint;

typedef struct {
	int num;
	GisPoint *points;
} GisPline;

typedef struct {
	int num;
	GisPoint *points;
} GisRegion;

typedef struct _GisElement {
	int type;
	int width;
	int pattern;
	int color;
	struct _GisElement *next;
	union {
		GisPline  line;
		GisPoint  point;
		GisRegion region;
	} d;
	int id;
	GisMap *map;
	char *add;
} GisElement;

typedef struct _GisNode {
	int type;
	int width;
	int pattern;
	int color;
	ebool is_sel;
	GalRect rect;
	union {
		GisPline  line;
		GisPoint  point;
		GisRegion region;
	} d;
	GisMap *map;
	char *add;
} GisNode;

struct _GisMap {
	GisElement *elm_head;
	int node_num;
	int columns;
	float r, g, b;
	struct {
		int type;
		int size;
	} indexs[INDEXS_MAX];
	GisNode *nodes;
	struct _GisMap *next;
};

static GisMap *map_head = NULL;

static int cache_pos = 0;
static GisPoint points_cache[1024 * 1024];
static GisNode *cur_enter = NULL;
static GisNode *cur_sel   = NULL;
static float show_scale    = 10.;
static GalRect map_rect;
static int offset_x = 0;
static int offset_y = 0;
static int save_x, save_y;
static ebool is_drag = efalse;
static int win_w, win_h;

static void win_expose_bg(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
}

static void draw_region_node(GuiWidget *wid, GisNode *node)
{
	int i;
/*	int total = node->d.region.num;

	for (i = 0; i < node->d.region.num; i++) {
		if (i < node->d.region.num - 1)
			node->d.region.points[i].next = &node->d.region.points[i + 1];
		else
			node->d.region.points[i].next = NULL;
	}
*/
	glBegin(GL_POLYGON);
	for (i = 0; i < node->d.region.num; i++) {
		GalRect rc = {node->rect.x + offset_x, node->rect.y + offset_y, node->rect.w, node->rect.h};
		if (egal_rect_is_intersect(&wid->rect, &rc)) {
			GisPoint *p = node->d.region.points + i;
			glVertex2f(p->x + offset_x, p->y + offset_y);
		}
	}
	glEnd();
}

static void win_expose(eHandle hobj, GuiWidget *wid, GalEventExpose *exp)
{
	GisMap *map = map_head;
	int i, j;

	glClearColor(0.0,0.0,0.0,0.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glMatrixMode(GL_PROJECTION);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glOrtho(0, win_w, win_h, 0, 0, 1);

	while (map) {
		for (j = 0; j < map->node_num; j++) {
			GisNode *node = map->nodes + j;
			if (node->is_sel)
				glColor3f(0.0f, 0.0f, 1.0f);
			else
				glColor3f(map->r, map->g, map->b);
			if (node->type == ELEMENT_TYPE_PLINE) {
				glBegin(GL_LINE_STRIP);
				for (i = 0; i < node->d.line.num; i++) {
					GalRect rc = {node->rect.x + offset_x, node->rect.y + offset_y, node->rect.w, node->rect.h};
					if (egal_rect_is_intersect(&wid->rect, &rc)) {
						GisPoint *p = node->d.line.points + i;
						glVertex2f(p->x + offset_x, p->y + offset_y);
					}
				}
				glEnd();
			}
			else if (node->type == ELEMENT_TYPE_POINT) {
				GalRect rc = {node->rect.x + offset_x, node->rect.y + offset_y, node->rect.w, node->rect.h};
				if (egal_rect_is_intersect(&wid->rect, &rc)) {
					glPointSize(1);
					glBegin(GL_POINTS);
					glColor3f(map->r, map->g, map->b);
					glVertex2f(node->d.point.x + offset_x, node->d.point.y + offset_y);
					glEnd();
				}
			}
			else if (node->type == ELEMENT_TYPE_REGION) {
				draw_region_node(wid, node);
			}
		}
		map = map->next;
	}

	GalSwapBuffers();
}

static GisNode *node_in_point(int x, int y)
{
	GisMap *map = map_head;
	int i, j;
	GisPoint *p1, *p2;
	GalRect rc;

	x -= offset_x;
	y -= offset_y;

	while (map) {
		for (j = 0; j < map->node_num; j++) {
			GisNode *node = map->nodes + j;

			if (node->type == ELEMENT_TYPE_POINT)
				continue;

			if (!egal_rect_point_in(&node->rect, x, y))
				continue;

			for (i = 0; i < node->d.line.num - 1; i++) {
				p1 = node->d.line.points + i;
				p2 = node->d.line.points + i + 1;
				if (p1->x < p2->x) {
					rc.x = p1->x;
					rc.w = p2->x;
				}
				else {
					rc.x = p2->x;
					rc.w = p1->x;
				}
				if (p1->y < p2->y) {
					rc.y = p1->y;
					rc.h = p2->y;
				}
				else {
					rc.y = p2->y;
					rc.h = p1->y;
				}
				rc.x--;
				rc.y--;
				rc.w = rc.w - rc.x + 2;
				rc.h = rc.h - rc.y + 2;
				if (egal_rect_point_in(&rc, x, y)) {
					float f;
					if (p1->x == p2->x) {
						f = x - p1->x;
					}
					else if (p1->y == p2->y) {
						f = y - p1->y;
					}
					else {
						float a, b, c, length;
						c = p1->x * p1->x + p1->y * p1->y;
						c = sqrt(c);
						a = 1.;
						b = -(float)(p2->x - p1->x) / (p2->y - p1->y);
						length = 1. + b * b;
						length = sqrt(length);

						f = ((x - p1->x) * a + (y - p1->y) * b) / length;
					}

					if (f < 0) f = -f;
					if (f < 2) return node;
				}
			}
		}
		map = map->next;
	}

	return NULL;
}

static eint win_mousemove(eHandle hobj, GalEventMouse *mevent)
{
	if (is_drag) {
		int x = offset_x;
		int y = offset_y;
		if (save_x != mevent->point.x)
			x += mevent->point.x - save_x;
		if (save_y != mevent->point.y)
			y += mevent->point.y - save_y;
		if (x != offset_x || y != offset_y) {
			offset_x = x;
			offset_y = y;
			egui_update(hobj);
		}
		save_x = mevent->point.x;
		save_y = mevent->point.y;
	}
	else {
		cur_enter = node_in_point(mevent->point.x, mevent->point.y);

		if (cur_enter)
			egal_set_cursor(GUI_WIDGET_DATA(hobj)->drawable, cursor);
		else
			egal_set_cursor(GUI_WIDGET_DATA(hobj)->drawable, 0);
	}

	return 0;
}

static eint win_lbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	GisPoint *p1;
	int i;

	if (cur_sel && cur_enter && cur_sel != cur_enter) {
		cur_sel->is_sel = efalse;
		glBegin(GL_LINE_STRIP);
		glColor3f(cur_sel->map->r, cur_sel->map->g, cur_sel->map->b);
		for (i = 0; i < cur_sel->d.line.num; i++) {
			p1 = cur_sel->d.line.points + i;
			glVertex2f(p1->x + offset_x, p1->y + offset_y);
		}
		glEnd();
		GalSwapBuffers();
		cur_sel = NULL;
	}

	if (!cur_sel && cur_enter) {
		cur_enter->is_sel = etrue;
		cur_sel = cur_enter;
		glBegin(GL_LINE_STRIP);
		glColor3f(0.0f, 0.0f, 1.0f);
		for (i = 0; i < cur_sel->d.line.num; i++) {
			p1 = cur_sel->d.line.points + i;
			glVertex2f(p1->x + offset_x, p1->y + offset_y);
		}
		glEnd();
		GalSwapBuffers();
	}

	return 0;
}

static eint win_lbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	if (is_drag)
		is_drag = efalse;
	return 0;
}

static eint win_mbuttondown(eHandle hobj, GalEventMouse *mevent)
{
	is_drag = etrue;
	save_x = mevent->point.x;
	save_y = mevent->point.y;
	return 0;
}

static eint win_mbuttonup(eHandle hobj, GalEventMouse *mevent)
{
	if (is_drag)
		is_drag = efalse;
	return 0;
}

static eint win_wheelforward(eHandle hobj, GalEventMouse *mevent)
{
	int x = (mevent->point.x - offset_x) * 1.3;
	int y = (mevent->point.y - offset_y) * 1.3;

	show_scale *= 1.3;
	path_node_reset();

	offset_x = -(x - mevent->point.x);
	offset_y = -(y - mevent->point.y);

	egui_update(hobj);
	return 0;
}

static eint win_wheelbackward(eHandle hobj, GalEventMouse *mevent)
{
	int x = (mevent->point.x - offset_x) / 1.3;
	int y = (mevent->point.y - offset_y) / 1.3;

	show_scale /= 1.3;
	path_node_reset();

	offset_x = -(x - mevent->point.x);
	offset_y = -(y - mevent->point.y);

	egui_update(hobj);
	return 0;
}

static eint win_resize(eHandle hobj, GuiWidget *wid, GalEventResize *resize)
{
	win_w = resize->w;
	win_h = resize->h;
	egui_make_GL(hobj);
	glViewport(0, 0, win_w, win_h);
	path_node_reset();
	return 0;
}

static void path_node_reset(void)
{
	GisMap *map = map_head;
	int i = 0;
	int j;

	cache_pos = 0;
	map_rect.x = 0x7fffffff;
	map_rect.y = 0x7fffffff;
	map_rect.w = 0;
	map_rect.h = 0;

	while (map) {
		GisElement *elm = map->elm_head;

		for (i = map->node_num - 1; i >= 0; i--, elm = elm->next) {
			GisNode *node = map->nodes + i;

			node->map     = map;
			node->type    = elm->type;
			node->width   = elm->width;
			node->color   = elm->color;
			node->pattern = elm->pattern;

			if (elm->type == ELEMENT_TYPE_PLINE) {
				GisPoint *gpoints = elm->d.line.points;
				int       n = elm->d.line.num;
				node->d.line.num = 0;
				node->d.line.points = &points_cache[cache_pos];
				node->d.line.points[0].x = gpoints[0].x * show_scale;
				node->d.line.points[0].y = gpoints[0].y * show_scale;
				node->rect.x = node->d.line.points[0].x;
				node->rect.y = node->d.line.points[0].y;
				node->rect.w = node->rect.x;
				node->rect.h = node->rect.y;
				for (j = 1; j < n; j++) {
					int _x = gpoints[j].x * show_scale;
					int _y = gpoints[j].y * show_scale;
					if (_x != node->d.line.points[node->d.line.num].x || _y != node->d.line.points[node->d.line.num].y) {
						node->d.line.num++;
						cache_pos++;
						node->d.line.points[node->d.line.num].x = _x;
						node->d.line.points[node->d.line.num].y = _y;
						if (_x < node->rect.x)
							node->rect.x = _x;
						else if (_x > node->rect.w)
							node->rect.w = _x;
						if (_y < node->rect.y)
							node->rect.y = _y;
						else if (_y > node->rect.h)
							node->rect.h = _y;
					}
				}
				if (node->d.line.num > 0) {
					node->rect.w = node->rect.w - node->rect.x + 1;
					node->rect.h = node->rect.h - node->rect.y + 1;
					node->d.line.num++;
					cache_pos++;
				}
			}
			else if (elm->type == ELEMENT_TYPE_REGION) {
				GisPoint *gpoints = elm->d.region.points;
				int       n = elm->d.region.num;
				node->d.region.num = 0;
				node->d.region.points = &points_cache[cache_pos];
				node->d.region.points[0].x = gpoints[0].x * show_scale;
				node->d.region.points[0].y = gpoints[0].y * show_scale;
				node->rect.x = node->d.region.points[0].x;
				node->rect.y = node->d.region.points[0].y;
				node->rect.w = node->rect.x;
				node->rect.h = node->rect.y;
				for (j = 1; j < n; j++) {
					int _x = gpoints[j].x * show_scale;
					int _y = gpoints[j].y * show_scale;
					if (_x != node->d.region.points[node->d.region.num].x || _y != node->d.region.points[node->d.region.num].y) {
						node->d.region.num++;
						cache_pos++;
						node->d.region.points[node->d.region.num].x = _x;
						node->d.region.points[node->d.region.num].y = _y;
						if (_x < node->rect.x)
							node->rect.x = _x;
						else if (_x > node->rect.w)
							node->rect.w = _x;
						if (_y < node->rect.y)
							node->rect.y = _y;
						else if (_y > node->rect.h)
							node->rect.h = _y;
					}
				}
				if (node->d.region.num > 0) {
					node->rect.w = node->rect.w - node->rect.x + 1;
					node->rect.h = node->rect.h - node->rect.y + 1;
					node->d.region.num++;
					cache_pos++;
				}
			}
			else if (elm->type == ELEMENT_TYPE_POINT) {
				node->d.point.x = elm->d.point.x * show_scale;
				node->d.point.y = elm->d.point.y * show_scale;
				node->width = elm->width;
				node->rect.x = node->d.point.x;
				node->rect.y = node->d.point.y;
				node->rect.w = 1;
				node->rect.h = 1;
			}

			if (node->rect.x < map_rect.x)
				map_rect.x = node->rect.x;
			else if (node->rect.x + node->rect.w > map_rect.w)
				map_rect.w = node->rect.x + node->rect.w;
			if (node->rect.y < map_rect.y)
				map_rect.y = node->rect.y;
			else if (node->rect.y + node->rect.h > map_rect.h)
				map_rect.h = node->rect.y + node->rect.h;
		}
		map = map->next;
	}

	map_rect.w = map_rect.w - map_rect.x + 1;
	map_rect.h = map_rect.h - map_rect.y + 1;
	if (map_rect.w < win_w)
		offset_x = (win_w - map_rect.w) / 2;
	if (map_rect.h < win_h)
		offset_y = (win_h - map_rect.h) / 2;

	map = map_head;
	while (map) {
		for (j = 0; j < map->node_num; j++) {
			GisNode *node = map->nodes + j;

			if (node->type == ELEMENT_TYPE_POINT) {
				node->d.point.x -= map_rect.x;
				node->d.point.y -= map_rect.y;
				node->d.point.y  = map_rect.h - node->d.point.y;
				node->rect.x = node->d.point.x;
				node->rect.y = node->d.point.y;
			}
			else if (node->type == ELEMENT_TYPE_PLINE) {
				int x, y;
				node->rect.x = 0x9fffffff;
				node->rect.y = 0x9fffffff;
				node->rect.w = 0;
				node->rect.h = 0;
				for (i = 0; i < node->d.line.num; i++) {
					if (node->type == ELEMENT_TYPE_PLINE) {
						x = node->d.line.points[i].x -= map_rect.x;
						y = node->d.line.points[i].y -= map_rect.y;
						y = node->d.line.points[i].y  = map_rect.h - y;
					}
					if (x < node->rect.x)
						node->rect.x = x;
					else if (x > node->rect.w)
						node->rect.w = x;
					if (y < node->rect.y)
						node->rect.y = y;
					else if (y > node->rect.h)
						node->rect.h = y;
				}
				node->rect.w = node->rect.w - node->rect.x + 1;
				node->rect.h = node->rect.h - node->rect.y + 1;
			}
			else if (node->type == ELEMENT_TYPE_REGION) {
				int x, y;
				node->rect.x = 0x9fffffff;
				node->rect.y = 0x9fffffff;
				node->rect.w = 0;
				node->rect.h = 0;
				for (i = 0; i < node->d.region.num; i++) {
					if (node->type == ELEMENT_TYPE_REGION) {
						x = node->d.region.points[i].x -= map_rect.x;
						y = node->d.region.points[i].y -= map_rect.y;
						y = node->d.region.points[i].y  = map_rect.h - y;
					}
					if (x < node->rect.x)
						node->rect.x = x;
					else if (x > node->rect.w)
						node->rect.w = x;
					if (y < node->rect.y)
						node->rect.y = y;
					else if (y > node->rect.h)
						node->rect.h = y;
				}
				node->rect.w = node->rect.w - node->rect.x + 1;
				node->rect.h = node->rect.h - node->rect.y + 1;
			}
		}
		map = map->next;
	}
}

#define DEF_INT		0
#define DEF_FLOAT	1
#define DEF_CHAR	2
#define DEF_DECIMAL	3

static struct {
	const char *symbol;
	int type;
	int args;
} def_type[4] = {{"integer", DEF_INT, 0}, {"float", DEF_FLOAT, 0}, {"char", DEF_CHAR, 1}, {"decimal", DEF_DECIMAL, 2}};

static void def_int_fun(char *src, void *dst)
{
	*(int *)dst = atoi(src);
}

static void def_float_fun(char *src, void *dst)
{
	*(float *)dst = atof(src);
}

static void def_char_fun(char *src, void *dst)
{
	char *p = src + 1;
	char *c = dst;
	while (p[0] != '\"') {
		c[0] = p[0];
		c++;
		p++;
	}
	c[0] = 0;
}

static void def_decimal_fun(char *src, void *dst)
{
	*(int *)dst = atoi(src);
}

void (*def_funs[4])(char *src, void *dst) = {
	def_int_fun,
	def_float_fun,
	def_char_fun,
	def_decimal_fun,
};

static void load_columns(char *buf, GisMap *map, GisElement *elm)
{
	int  i;
	char *p  = buf;
	char *a1 = elm->add;
	char *a2 = elm->add + map->columns * sizeof(char *);
	for (i = 0; i < map->columns; i++) {
		if (map->indexs[i].type == DEF_CHAR) {
			*(char **)a1 = a2;
			def_funs[map->indexs[i].type](p, a1);
			a2 += map->indexs[i].size;
		}
		else
			def_funs[map->indexs[i].type](p, a1);
		a1 += sizeof(char *);
		p = strchr(p, ',') + 1;
	}
}

static void gis_load_mif(const char *name, float r, float g, float b)
{
	GisMap *map;
	GisElement *elm;
	char buf[256];
	char *p;
	int  i;
	int col_size = 0;
	FILE *mif_fp;
	FILE *mid_fp;
	GisPoint *point;

	strcpy(buf, name);
	strcat(buf, ".mif");
	mif_fp = fopen(buf, "r");
	strcpy(buf, name);
	strcat(buf, ".mid");
	mid_fp = fopen(buf, "r");

	map = e_calloc(sizeof(GisMap), 1);
	map->r = r;
	map->g = g;
	map->b = b;
	map->next = map_head;
	map_head  = map;

	while ((p = fgets(buf, sizeof(buf), mif_fp))) {
		if (e_strcasestr(p, "data"))
			break;
		else if (e_strcasestr(p, "columns")) {
			int j;
			char t1[20], t2[20];

			sscanf(buf + 8, "%d", &map->columns);
			for (i = 0; i < map->columns; i++) {
				p = fgets(buf, sizeof(buf), mif_fp);
				sscanf(buf, "%s %s", t1, t2);
				for (j = 0; j < sizeof(def_type) / sizeof(def_type[0]); j++)
					if (e_strcasestr(p, def_type[j].symbol)) {
						map->indexs[i].type = def_type[j].type;
						if (def_type[j].args > 0) {
							int a;
							p = strchr(buf, '(');
							if (def_type[j].args == 1) {
								sscanf(p, "(%d)", &map->indexs[i].size);
								map->indexs[i].size++;
								map->indexs[i].size = (map->indexs[i].size + 3) & ~3;
								col_size += map->indexs[i].size;
							}
							else if (def_type[j].args == 2)
								sscanf(p, "(%d,%d)", &map->indexs[i].size, &a);
						}
						else
							map->indexs[i].size = 4;
						break;
					}
			}
			col_size += map->columns * sizeof(char *);
		}
	}

	e_slice_set_level(col_size, 5);

	while ((p = fgets(buf, sizeof(buf), mif_fp))) {
		int n = 0;
		if (e_strcasestr(p, "pline")) {
			sscanf(buf + 6, "%d", &n);
			elm = e_slice_new(GisElement);
			elm->map  = map;
			elm->type = ELEMENT_TYPE_PLINE;
			elm->next = map->elm_head;
			map->elm_head  = elm;
			elm->d.line.num = n;
			elm->d.line.points = e_malloc(sizeof(GisPoint) * n);
			point = elm->d.line.points;
			for (i = 0; i < n; i++, point++) {
				fgets(buf, sizeof(buf), mif_fp);
				sscanf(buf, "%f %f", &point->x, &point->y);
			}
			fgets(buf, sizeof(buf), mif_fp);
			p = strchr(buf, '(');
			sscanf(p, "(%d,%d,%d)", &elm->width, &elm->pattern, &elm->color);
			map->node_num++;

			if (mid_fp) {
				elm->add = e_slice_alloc(col_size);
				fgets(buf, sizeof(buf), mid_fp);
				load_columns(buf, map, elm);
			}
		}
		else if (e_strcasestr(p, "line")) {
			elm = e_slice_new(GisElement);
			elm->map  = map;
			elm->type = ELEMENT_TYPE_PLINE;
			elm->next = map->elm_head;
			map->elm_head  = elm;
			elm->d.line.num = 2;
			elm->d.line.points = e_malloc(sizeof(GisPoint) * 2);
			point = elm->d.line.points;
			sscanf(buf + 5, "%f %f %f %f",
					&point[0].x, &point[0].y,
					&point[1].x, &point[1].y);
			fgets(buf, sizeof(buf), mif_fp);
			p = strchr(buf, '(');
			sscanf(p, "(%d,%d,%d)", &elm->width, &elm->pattern, &elm->color);
			map->node_num++;

			if (mid_fp) {
				elm->add = e_slice_alloc(col_size);
				fgets(buf, sizeof(buf), mid_fp);
				load_columns(buf, map, elm);
			}
		}
		else if (e_strcasestr(p, "point")) {
			elm = e_slice_new(GisElement);
			elm->map  = map;
			elm->type = ELEMENT_TYPE_POINT;
			elm->next = map->elm_head;
			map->elm_head  = elm;
			sscanf(buf + 6, "%f %f", &elm->d.point.x, &elm->d.point.y);
			fgets(buf, sizeof(buf), mif_fp);
			p = strchr(buf, '(');
			sscanf(p, "(%d,%d,%d)", &elm->pattern, &elm->color, &elm->width);
			map->node_num++;

			if (mid_fp) {
				elm->add = e_slice_alloc(col_size);
				fgets(buf, sizeof(buf), mid_fp);
				load_columns(buf, map, elm);
			}
		}
		else if (e_strcasestr(p, "region")) {
			int i;
			elm = e_slice_new(GisElement);
			elm->map  = map;
			elm->type = ELEMENT_TYPE_REGION;
			elm->next = map->elm_head;
			map->elm_head  = elm;
			p = fgets(buf, sizeof(buf), mif_fp);
			elm->d.region.num = atoi(p);
			elm->d.region.points = e_malloc(sizeof(GisPoint) * elm->d.region.num);
			for (i = 0; i < elm->d.region.num; i++) {
				fgets(buf, sizeof(buf), mif_fp);
				sscanf(buf, "%f %f", &elm->d.region.points[i].x, &elm->d.region.points[i].y);
			}
			fgets(buf, sizeof(buf), mif_fp);
			fgets(buf, sizeof(buf), mif_fp);
			map->node_num++;
		}
		else continue;
	}

	fclose(mif_fp);
	if (mid_fp)
		fclose(mid_fp);

	map->nodes = e_calloc(sizeof(GisNode) * map->node_num, 1);
}

int main(int argc, char *const argv[])
{
	eHandle win, vbox, hbox, drawable, bn;

	egui_init(argc, argv);

	win_w = 600;
	win_h = 480;
	win  = egui_window_new(GUI_WINDOW_TOPLEVEL);
	e_signal_connect(win, SIG_DESTROY, egui_quit);

	vbox = egui_vbox_new();
	egui_box_set_layout(vbox, BoxLayout_CENTER);
	egui_set_expand(vbox, etrue);
	egui_add(win, vbox);

	drawable = egui_drawable_new(win_w, win_h);
	egui_add(vbox, drawable);

	hbox = egui_hbox_new();
	egui_box_set_layout(hbox, BoxLayout_CENTER);
	egui_set_expand(hbox, etrue);
	egui_add(vbox, hbox);

	bn = egui_label_button_new(_("hello"));
	egui_add(hbox, bn);

	bn = egui_label_button_new(_("hello1"));
	egui_add(hbox, bn);

	cursor = egal_cursor_new(GAL_SB_H_DOUBLE_ARROW);
	egui_make_GL(drawable);

	e_signal_connect(drawable, SIG_RESIZE,      win_resize);
	e_signal_connect(drawable, SIG_EXPOSE,      win_expose);
	e_signal_connect(drawable, SIG_EXPOSE_BG,   win_expose_bg);
	e_signal_connect(drawable, SIG_MOUSEMOVE,   win_mousemove);
	e_signal_connect(drawable, SIG_LBUTTONDOWN, win_lbuttondown);
	e_signal_connect(drawable, SIG_LBUTTONUP,   win_lbuttonup);
	e_signal_connect(drawable, SIG_MBUTTONDOWN, win_mbuttondown);
	e_signal_connect(drawable, SIG_MBUTTONUP,   win_mbuttonup);
	e_signal_connect(drawable, SIG_WHEELFORWARD, win_wheelforward);
	e_signal_connect(drawable, SIG_WHEELBACKWARD, win_wheelbackward);


	gis_load_mif("bou2_4l", 1, 1, 1);
	//gis_load_mif("mif\\bou1_4_line", 0, 1, 0);
	//gis_load_mif("mif\\diquJie",  0, 0, 1);
	//gis_load_mif("mif\\XianJie", 0, 0, 1);
	//gis_load_mif("mif\\river4", 1, 0, 1);
	//gis_load_mif("mif\\bou1_4_poly", 1, 0, 1);
	egui_main();
//	FT_Glyph_To_Bitmap(NULL, 0, NULL, 0);
	return 0;
}
