#include <math.h>
#include <ftglyph.h>
#include <ft2build.h>

#include FT_FREETYPE_H 

#include <cairo/cairo.h>
#include <cairo/cairo-ft.h>

#include <egal/font.h>

#define GTYPE_FONT_CAIRO				(egal_genetype_cairo())
#define CAIRO_FONT_DATA(font)			((CairoFont *)(e_object_type_data(font, GTYPE_FONT_CAIRO)))

static eGeneType egal_genetype_cairo(void);
FcPattern* egal_fc_make_pattern(const GalPattern *);

typedef struct _CairoFont {
	const echar         *fname;
	GalMetrics           metrics;
	cairo_t             *cr;
	eint                 mark;
	GalDrawable          drawable;
	cairo_scaled_font_t *scale_font;
	cairo_font_face_t   *cairo_face;
} CairoFont;

static GalFace cairo_lock_face(GalFont font)
{
	return (GalFace)cairo_ft_scaled_font_lock_face(CAIRO_FONT_DATA(font)->scale_font);
}

static void cairo_unlock_face(GalFont font)
{
	cairo_ft_scaled_font_unlock_face(CAIRO_FONT_DATA(font)->scale_font);
}

static void cairo_get_glyph(GalFont font, eunichar ichar, GalGlyph *fg)
{
	CairoFont *crfont = CAIRO_FONT_DATA(font);
	FT_Face    face   = cairo_ft_scaled_font_lock_face(crfont->scale_font);
	FT_UInt    index  = FT_Get_Char_Index(face, ichar);

	cairo_glyph_t glyph;
	glyph.index = index;
	cairo_text_extents_t extents;

	cairo_scaled_font_glyph_extents(crfont->scale_font, &glyph, 1, &extents);

	fg->glyph = (eGlyph)index;
	fg->x     = extents.x_bearing;
	fg->w     = extents.x_advance;

	cairo_ft_scaled_font_unlock_face(crfont->scale_font);
}

static void cairo_get_extents(GalFont font, eunichar ichar, GalRect *rect)
{
	CairoFont *crfont = CAIRO_FONT_DATA(font);
	FT_Face    face   = cairo_ft_scaled_font_lock_face(crfont->scale_font);
	FT_UInt index     = FT_Get_Char_Index(face, ichar);
	FT_Glyph_Metrics *gm;

	FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);

	gm = &face->glyph->metrics;

	rect->w = gm->horiAdvance / 64;
	rect->h = crfont->metrics.height;

	cairo_ft_scaled_font_unlock_face(crfont->scale_font);
}

static void cairo_draw_glyphs(GalDrawable drawable, GalPB pb, GalFont font, GalRect *area, eint x, eint y, GalGlyph *glyphs, eint len)
{
#define MAX_STACK 80
	CairoFont *crfont = CAIRO_FONT_DATA(font);
	cairo_glyph_t stack_glyphs[MAX_STACK];
	cairo_glyph_t *cairo_glyphs;
	eint i;

	if (!crfont->cr
			|| crfont->drawable != drawable
			|| crfont->mark != egal_drawable_get_mark(drawable)) {
		crfont->drawable  = drawable;
		crfont->mark      = egal_drawable_get_mark(drawable);

		if (!(crfont->cr = egal_surface_private(drawable)))
			return;

		cairo_set_scaled_font(crfont->cr, crfont->scale_font);
	}

	if (len > MAX_STACK)
		cairo_glyphs = e_malloc(sizeof(cairo_glyph_t) * len);
	else
		cairo_glyphs = stack_glyphs;

	for (i = 0; i < len; i++) {
		cairo_glyphs[i].index = glyphs[i].glyph;
		cairo_glyphs[i].x = x;
		cairo_glyphs[i].y = y + crfont->metrics.ascent;
		x += glyphs[i].w;
	}

	eulong color = egal_get_foreground(pb);
	double b = color & 0xff;
	double g = (color & 0xff00) >> 8;
	double r = (color & 0xff0000) >> 16;
	cairo_set_source_rgb(crfont->cr, r, g, b);
	cairo_show_glyphs(crfont->cr, cairo_glyphs, len);

	if (len > MAX_STACK)
		e_free(cairo_glyphs);
}

static GalMetrics *cairo_get_metrics(GalFont font)
{
	return &CAIRO_FONT_DATA(font)->metrics;
}

static eint cairo_init(GalFont font, eValist vp)
{
	CairoFont  *crfont = CAIRO_FONT_DATA(font);
	GalPattern *gpn    = e_va_arg(vp, GalPattern *);

	crfont->cairo_face   = cairo_ft_font_face_create_for_pattern((FcPattern *)gpn->private);
	crfont->metrics.size = gpn->size / 10;

	{
		FT_Face face;
		cairo_matrix_t matrix, ctm;
		cairo_font_options_t *options = cairo_font_options_create();

		cairo_matrix_init_identity(&matrix);
		cairo_matrix_init_identity(&ctm);
		cairo_matrix_scale(&matrix, crfont->metrics.size, crfont->metrics.size);

		crfont->scale_font = cairo_scaled_font_create(crfont->cairo_face, &matrix, &ctm, options);

		face = cairo_ft_scaled_font_lock_face(crfont->scale_font);
		crfont->metrics.height  = face->size->metrics.height / 64;
		crfont->metrics.ascent  = face->size->metrics.ascender / 64;
		crfont->metrics.descent = face->size->metrics.descender / 64;
		cairo_ft_scaled_font_unlock_face(crfont->scale_font);
	}

	return 0;
}

static void cairo_init_orders(eGeneType new, ePointer this)
{
	GalFontOrders *f = e_genetype_orders(new, GTYPE_GAL_FONT);
	eCellOrders   *c = e_genetype_orders(new, GTYPE_CELL);

	c->init        = cairo_init;
	f->get_metrics = cairo_get_metrics;
	f->get_glyph   = cairo_get_glyph;
	f->get_extents = cairo_get_extents;
	f->draw_glyphs = cairo_draw_glyphs;
	f->lock_face   = cairo_lock_face;
	f->unlock_face = cairo_unlock_face;
}

static eGeneType egal_genetype_cairo(void)
{
	static eGeneType etype = 0;
	if (!etype) {
		eGeneInfo info = {
			0,
			cairo_init_orders,
			sizeof(CairoFont),
			NULL, NULL, NULL,
		};
		etype = e_register_genetype(&info, GTYPE_GAL_FONT, NULL);
	}
	return etype;
}

GalFont cairo_create_font(GalPattern *pattern)
{
	return e_object_new(GTYPE_FONT_CAIRO, pattern);
}

/*
cairo_font_face_destroy(font_face);

cairo_matrix_init_rotate(&gravity_matrix, pango_gravity_to_rotation(cf_priv->gravity));
cairo_matrix_multiply(&cf_priv->data->font_matrix, font_matrix, &gravity_matrix);

pango_ctm = pango_context_get_matrix(context);
cairo_matrix_init(&cf_priv->data->ctm,
              pango_ctm->xx,
              pango_ctm->yx,
              pango_ctm->xy,
              pango_ctm->yy,
              0., 0.);

if (crenderer->do_path)
	cairo_glyph_path(crenderer->cr, cairo_glyphs, count);
else
	cairo_show_glyphs(crenderer->cr, cairo_glyphs, count);
*/
