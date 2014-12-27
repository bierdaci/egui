#include <math.h>
#include "win32.h"

typedef struct {
	GalPattern *pattern;
	const char *fname;
	GalMetrics metrics;
	HFONT hfont;
} W32Font;


HDC w32_create_compatible_DC(void);

static eGeneType egal_genetype_wfont(void);
#define GTYPE_FONT_W32				(egal_genetype_wfont())
#define W32_FONT_DATA(obj)			((W32Font *)(e_object_type_data(obj, GTYPE_FONT_W32)))

static HFONT oldfont = 0;
static HDC   tmpdc;
static MAT2 m = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};

static void wfont_get_glyph(GalFont font, eunichar ichar, GalGlyph *gg)
{
	W32Font *wfont = W32_FONT_DATA(font);
	GLYPHMETRICS gm;
	WORD glyph;

	if (ichar == '\t')
		ichar = ' ';

	oldfont = SelectObject(tmpdc, wfont->hfont);
	GetGlyphIndicesW(tmpdc, &ichar, 1, &glyph, 0);
	GetGlyphOutlineW(tmpdc, glyph, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &m);
	SelectObject(tmpdc, oldfont);

	gg->glyph = (eGlyph)glyph;
	gg->x     = (eushort)gm.gmptGlyphOrigin.x;
	gg->w     = gm.gmCellIncX;;
}

static int wfont_load_glyphs(GalFont font, GalGlyph *gs, eunichar *ichs, eint len)
{
	W32Font *wfont = W32_FONT_DATA(font);
	GLYPHMETRICS gm;
	WORD glyph;
	int i;

	oldfont = SelectObject(tmpdc, wfont->hfont);
	for (i = 0; i < len; i++) {
		GetGlyphIndicesW(tmpdc, ichs+i, 1, &glyph, 0);
		GetGlyphOutlineW(tmpdc, glyph, GGO_METRICS | GGO_GLYPH_INDEX, &gm, 0, NULL, &m);
		gs[i].glyph = (eGlyph)glyph;
		gs[i].x     = (eushort)gm.gmptGlyphOrigin.x;
		gs[i].w     = gm.gmCellIncX;
	}
	SelectObject(tmpdc, oldfont);
	return len;
}

static void wfont_get_extents(GalFont font, eunichar ichar, GalRect *rect)
{
	W32Font  *wfont = W32_FONT_DATA(font);
	GLYPHMETRICS gm;

	oldfont = SelectObject(tmpdc, wfont->hfont);
	GetGlyphOutlineW(tmpdc,
			      ichar,
			      GGO_METRICS | GGO_GLYPH_INDEX,
			      &gm,
			      0, NULL,
			      &m);
	SelectObject(tmpdc, oldfont);
	rect->w = gm.gmCellIncX;
	rect->h = gm.gmCellIncY;
}

static GalMetrics *wfont_get_metrics(GalFont font)
{
	return &W32_FONT_DATA(font)->metrics;
}

static void wfont_draw_glyphs(GalDrawable drawable, GalPB pb, GalFont obj,
		GalRect *area, eint x, eint y, GalGlyph *glyphs, eint len)
{
	W32Font *wfont = W32_FONT_DATA(obj);
	GalPB32 *wpb   = W32_PB_DATA(pb);

	static WORD glyph_indexes[1000];
	static int dx[1000];
	int i;
	int oldmode;

	oldfont = SelectObject(wpb->hdc, wfont->hfont);
	SetTextColor(wpb->hdc, wpb->font_color);

	for (i = 0; i < len; i++) {
		glyph_indexes[i] = glyphs[i].glyph;
		dx[i] = glyphs[i].w;
	}

	oldmode = SetBkMode(wpb->hdc, TRANSPARENT);
	ExtTextOutW(wpb->hdc,
			x, y,
			ETO_GLYPH_INDEX,
			NULL,
			glyph_indexes, len,
			dx);
	SetBkMode(wpb->hdc, oldmode);

	SelectObject(wpb->hdc, oldfont);
}

static void wfont_free_data(eHandle obj, ePointer data)
{
	W32Font *wfont = data;
	DeleteObject(wfont->hfont);
}

static eint wfont_init(GalFont obj, eValist vp)
{
	W32Font    *wfont = W32_FONT_DATA(obj);
	GalPattern *pattern = e_va_arg(vp, GalPattern *);

	TEXTMETRIC tm;
	LOGFONT lf = {0};
	
	if (tmpdc == 0)
		tmpdc = w32_create_compatible_DC();

	wfont->fname   = egal_pattern_get_file(pattern);
	wfont->pattern = pattern;
	wfont->metrics.size = pattern->size / 10;
	lf.lfHeight = wfont->metrics.size;
	wfont->hfont = CreateFontIndirect(&lf);
	oldfont = SelectObject(tmpdc, wfont->hfont);
	GetTextMetrics(tmpdc, &tm);
	SelectObject(tmpdc, oldfont);

	wfont->metrics.height  = tm.tmHeight;
	wfont->metrics.ascent  = tm.tmAscent;
	wfont->metrics.descent = tm.tmDescent;

	return 0;
}

static void wfont_init_orders(eGeneType new, ePointer this)
{
	GalFontOrders *f = e_genetype_orders(new, GTYPE_GAL_FONT);
	eCellOrders   *c = e_genetype_orders(new, GTYPE_CELL);

	c->init        = wfont_init;
	f->get_metrics = wfont_get_metrics;
	f->get_glyph   = wfont_get_glyph;
	f->get_extents = wfont_get_extents;
	f->draw_glyphs = wfont_draw_glyphs;
}

static eGeneType egal_genetype_wfont(void)
{
	static eGeneType gtype = 0;
	if (!gtype) {
		eGeneInfo info = {
			0,
			wfont_init_orders,
			sizeof(W32Font),
			NULL,
			wfont_free_data,
			NULL,
		};
		gtype = e_register_genetype(&info, GTYPE_GAL_FONT, NULL);
	}
	return gtype;
}

GalFont w32_create_font(GalPattern *pattern)
{
	return e_object_new(GTYPE_FONT_W32, pattern);
}
