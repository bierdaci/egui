#include "font.h"
#include <sys/types.h>
#include <sys/stat.h>
#ifdef linux
#include <unistd.h>
#endif

//#undef _GAL_SUPPORT_CAIRO

void egal_unichar_extents(GalFont font, eunichar ichar, GalRect *rect)
{
	GAL_FONT_ORDERS(font)->get_extents(font, ichar, rect);
}

void egal_get_glyph(GalFont font, eunichar ichar, GalGlyph *glyph)
{
	GAL_FONT_ORDERS(font)->get_glyph(font, ichar, glyph);
}

eint egal_load_glyphs(GalFont font, GalGlyph *glyphs, eunichar *ichar, eint len)
{
	return GAL_FONT_ORDERS(font)->load_glyphs(font, glyphs, ichar, len);
}

void egal_draw_glyphs(GalWindow window, GalPB pb,
		GalFont font, GalRect *area,
		eint x, eint y,
		GalGlyph *glyphs, eint len)
{
	GAL_FONT_ORDERS(font)->draw_glyphs(window, pb, font, area, x, y, glyphs, len);
}

GalMetrics *egal_font_metrics(GalFont font)
{
	return GAL_FONT_ORDERS(font)->get_metrics(font);
}

eint egal_font_height(GalFont font)
{
	return GAL_FONT_ORDERS(font)->get_metrics(font)->height;
}

eint egal_unichar_width(GalFont font, eunichar ichar)
{
	GalRect extent;
	egal_unichar_extents(font, ichar, &extent);
	return extent.w;
}

GalFont egal_default_font(void)
{
	static GalFont font = 0;
	struct stat st;

	if (!font) {
#ifdef WIN32
		if (stat("config", &st) == 0) {
#else
		if (stat("./.config", &st) == 0) {
#endif
			eConfig *conf = e_conf_open(_("./.config"));
			font = egal_font_load(e_conf_get_val(conf, _("font"), _("default")));
			e_conf_destroy(conf);
		}
		else {
			echar path[256];
			char *p = getenv("HOME");
			if (p)
				e_strcpy(path, (echar *)p);
			else
				path[0] = 0;
#ifdef WIN32
			e_strcat(path, _("\\egui\\config"));
#else
			e_strcat(path, _("/.egui/config"));
#endif
			if (stat((const char *)path, &st) == 0) {
				eConfig *conf = e_conf_open(path);
				font = egal_font_load(e_conf_get_val(conf, _("font"), _("default")));
				e_conf_destroy(conf);
			}
			else
				font = egal_font_load(_("-*-AR PL UMing CN-medium-r-*-*-*-190-*-*-*-*-*-*"));
		}
	}
	return font;
}

eint egal_text_width(GalFont font, const echar *text, eint len)
{
	eint width = 0;
	eint nchar = 0;
	eint i;
	for (i = 0; i < len; i++) {
		GalRect extent;
		eunichar ichar = e_uni_get_char((const euchar *)text + nchar);
		nchar += e_uni_char_len(text + nchar);
		egal_unichar_extents(font, ichar, &extent);
		width += extent.w;
	}

	return width;
}

eint egal_text_height(GalFont font, const echar *text, eint len)
{
	eint line  = 1;
	eint nchar = 0;
	eint i;
	for (i = 0; i < len; i++) {
		eint ichar = e_uni_get_char((const euchar *)text + nchar);
		nchar += e_uni_char_len(text + nchar);
		if (ichar == '\n')
			line++;
	}
	return line * egal_font_height(font);
}

eGeneType egal_genetype_font(void)
{
	static eGeneType gtype = 0;
	if (!gtype) {
		eGeneInfo info = {
			sizeof(GalFontOrders),
			NULL, 
			0,
			NULL, NULL, NULL,
		};
		gtype = e_register_genetype(&info, GTYPE_GAL, NULL);
	}
	return gtype;
}

GalFont egal_font_load(const echar *font_name)
{
	if (_gwm.create_font) {
		GalPattern *gpn = egal_pattern_new(font_name);
		return _gwm.create_font(gpn);
	}
	return 0;
}

