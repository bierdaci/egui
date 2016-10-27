#include <math.h>
#include <ftglyph.h>
#include <ft2build.h>

#include FT_FREETYPE_H 

#include <egal/window.h>
#include <egal/image.h>
#include <egal/font.h>

#define GTYPE_FONT_FT2				(egal_genetype_ft2())
#define FT2_FONT_DATA(obj)			((Ft2Font *)(e_object_type_data(obj, GTYPE_FONT_FT2)))

static eGeneType egal_genetype_ft2(void);

typedef struct _Ft2Font {
	GalPattern *pattern;
	const char *fname;
	GalMetrics metrics;
	FT_Face face;
	GalImage *image;
} Ft2Font;

static FT_Library ft2_library = NULL;

static FT_Face ft2_get_face(Ft2Font *ft2font)
{
	if (!ft2_library)
		FT_Init_FreeType(&ft2_library);

	if (!ft2font->face) {
		FT_New_Face(ft2_library, ft2font->fname, 0, &ft2font->face);
		FT_Set_Char_Size(ft2font->face, ft2font->metrics.size * 64, ft2font->metrics.size * 64, 0, 0);
	}
	return ft2font->face;
}

static void ft2_get_glyph(GalFont font, eunichar ichar, GalGlyph *gg)
{
	Ft2Font *ft2font = FT2_FONT_DATA(font);
	FT_Face  face    = ft2_get_face(ft2font);
	FT_UInt  index   = FT_Get_Char_Index(face, ichar);
	FT_Glyph_Metrics *gm;

	FT_Load_Glyph(ft2font->face, index, FT_LOAD_DEFAULT);

	gm = &ft2font->face->glyph->metrics;
	gg->glyph = (eGlyph)index;
	gg->x     = gm->horiBearingX / 64;
	gg->w     = gm->horiAdvance  / 64;
}

static void ft2_get_extents(GalFont font, eunichar ichar, GalRect *rect)
{
	Ft2Font  *ft2font = FT2_FONT_DATA(font);
	FT_Face   face    = ft2_get_face(ft2font);
	FT_UInt   index   = FT_Get_Char_Index(face, ichar);
	FT_Glyph_Metrics *gm;

	FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
	gm = &face->glyph->metrics;
	
	rect->w = gm->horiAdvance / 64;
	rect->h = ft2font->metrics.size;
}

static GalMetrics *ft2_get_metrics(GalFont font)
{
	return &FT2_FONT_DATA(font)->metrics;
}

static void ft2_bitmap_to_image(FT_Bitmap *bitmap, GalImage *image, eint x, eint y)
{
	eint width  = bitmap->width;
	eint height = bitmap->rows;

	euchar *dst = image->pixels + image->rowbytes * y + x * 4;
	euchar *src = bitmap->buffer;

	eint i, j;
	for (j = 0; j < height; j++) {
		for (i = 0; i < width; i++)
			dst[i * 4 + 3] |= src[i];
		dst += image->rowbytes;
		src += bitmap->width;
	}
}

static void ft2_draw_glyphs(GalDrawable drawable, GalPB pb, GalFont obj,
		GalRect *area, eint x, eint y, GalGlyph *glyphs, eint len)
{
	Ft2Font *ft2font = FT2_FONT_DATA(obj);
	GalImage  *image = ft2font->image;
	FT_Face     face = ft2_get_face(ft2font);
	FT_BitmapGlyph bitmap_glyph;
	FT_Bitmap     *bitmap;
	eint i, w = 0;

	egal_fill_image(image, 0, area->x, 0, area->w, image->h);
	for (i = 0; i < len; i++) {
		FT_Glyph glyph;
		FT_Load_Glyph(face, glyphs[i].glyph, FT_LOAD_DEFAULT);
		FT_Get_Glyph(face->glyph, &glyph);
		if (face->glyph->format != ft_glyph_format_bitmap)
			FT_Render_Glyph(face->glyph, ft_render_mode_normal);

		FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, 0, 1);
		bitmap_glyph = (FT_BitmapGlyph)glyph;
		bitmap       = &bitmap_glyph->bitmap;

		ft2_bitmap_to_image(bitmap, image,
				x + w + face->glyph->bitmap_left,
				ft2font->metrics.height - face->glyph->bitmap_top + ft2font->metrics.descent);
		w += glyphs[i].w;
	}

	{
		GalRect rc = {x, y, w, ft2font->metrics.height};
		egal_rect_intersect(&rc, &rc, area);
		eint _y = rc.y - y;
		egal_composite_image(drawable, pb, rc.x, rc.y, image, rc.x, _y, rc.w, rc.h - _y);
		//egal_draw_image(drawable, pb, rc.x, rc.y, image, rc.x, _y, rc.w, rc.h - _y);
	}
}

static void ft2_free_data(eHandle obj, ePointer data)
{
	Ft2Font *ft2font = data;

	FT_Done_Face(ft2font->face);
	FT_Done_FreeType(ft2_library);
	ft2_library = NULL;
}

static eint ft2_init(GalFont obj, eValist vp)
{
	Ft2Font    *ft2font = FT2_FONT_DATA(obj);
	GalPattern *pattern = e_va_arg(vp, GalPattern *);
	FT_Face     face;

	ft2font->fname   = egal_pattern_get_file(pattern);
	ft2font->pattern = pattern;
	ft2font->metrics.size = pattern->size / 10;

	if (!(face = ft2_get_face(ft2font)))
		return -1;

	ft2font->metrics.height  = face->size->metrics.height / 64;
	ft2font->metrics.ascent  = face->size->metrics.ascender / 64;
	ft2font->metrics.descent = face->size->metrics.descender / 64;
	ft2font->image = egal_image_new(1000, ft2font->metrics.height, etrue);

	return 0;
}

static void ft2_init_orders(eGeneType new, ePointer this)
{
	GalFontOrders *f = e_genetype_orders(new, GTYPE_GAL_FONT);
	eCellOrders   *c = e_genetype_orders(new, GTYPE_CELL);

	c->init        = ft2_init;
	f->get_metrics = ft2_get_metrics;
	f->get_glyph   = ft2_get_glyph;
	f->get_extents = ft2_get_extents;
	f->draw_glyphs = ft2_draw_glyphs;
}

static eGeneType egal_genetype_ft2(void)
{
	static eGeneType gtype = 0;
	if (!gtype) {
		eGeneInfo info = {
			0,
			ft2_init_orders,
			sizeof(Ft2Font),
			NULL,
			ft2_free_data,
			NULL,
		};
		gtype = e_register_genetype(&info, GTYPE_GAL_FONT, NULL);
	}
	return gtype;
}

GalFont ft2_create_font(GalPattern *pattern)
{
	return e_object_new(GTYPE_FONT_FT2, pattern);
}
