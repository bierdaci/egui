#ifndef __GAL_FONT_H__
#define __GAL_FONT_H__

#include <egal/types.h>
#include <egal/window.h>

#define GTYPE_GAL_FONT					(egal_genetype_font())

#define GAL_FONT_ORDERS(font)			((GalFontOrders *)e_object_type_orders(font, GTYPE_GAL_FONT))

#define FONT_SCALE      1024
#define FONT_SCALE_26_6 (FONT_SCALE / (1<<6))
#define FONT_PIXELS_26_6(d)								\
	(((d) >= 0) ?										\
	 ((d) + FONT_SCALE_26_6 / 2) / FONT_SCALE_26_6 :	\
	 ((d) - FONT_SCALE_26_6 / 2) / FONT_SCALE_26_6)

#define FONT_UNITS_26_6(d) (FONT_SCALE_26_6 * (d))

typedef eulong						GalFace;
typedef struct _GalFontOrders		GalFontOrders;
typedef struct _GalGlyph			GalGlyph;
typedef struct _GalMetrics			GalMetrics;

struct _GalGlyph {
	eGlyph glyph;
	eint16 x, w;
};

struct _GalMetrics {
	eint size;
	eint height;
	eint ascent;
	eint descent;
};

struct _GalFontOrders {
	void (*get_glyph  )(GalFont, eunichar, GalGlyph *);
	eint (*load_glyphs)(GalFont, GalGlyph *, eunichar *, eint);
	void (*get_extents)(GalFont, eunichar, GalRect  *);
	void (*draw_glyphs)(GalDrawable, GalPB, GalFont, GalRect *, eint, eint, GalGlyph *, eint);
	GalMetrics *(*get_metrics)(GalFont);

	GalFace (*lock_face  )(GalFont);
	void    (*unlock_face)(GalFont);
};

typedef enum {
	FONT_SLANT_NORMAL,
	FONT_SLANT_OBLIQUE,
	FONT_SLANT_ITALIC
} GalFontSlant;

typedef enum {
	FONT_VARIANT_NORMAL,
	FONT_VARIANT_SMALL_CAPS
} GalFontVariant;

typedef enum {
	FONT_WEIGHT_ULTRALIGHT = 200,
	FONT_WEIGHT_LIGHT      = 300,
	FONT_WEIGHT_NORMAL     = 400,
	FONT_WEIGHT_SEMIBOLD   = 600,
	FONT_WEIGHT_BOLD       = 700,
	FONT_WEIGHT_ULTRABOLD  = 800,
	FONT_WEIGHT_HEAVY      = 900,
} GalFontWeight;

typedef enum {
	FONT_STRETCH_ULTRA_CONDENSED,
	FONT_STRETCH_EXTRA_CONDENSED,
	FONT_STRETCH_CONDENSED,
	FONT_STRETCH_SEMI_CONDENSED,
	FONT_STRETCH_NORMAL,
	FONT_STRETCH_SEMI_EXPANDED,
	FONT_STRETCH_EXPANDED,
	FONT_STRETCH_EXTRA_EXPANDED,
	FONT_STRETCH_ULTRA_EXPANDED
} GalFontStretch;

typedef enum {
	FONT_MASK_FAMILY  = 1 << 0,
	FONT_MASK_STYLE   = 1 << 1,
	FONT_MASK_VARIANT = 1 << 2,
	FONT_MASK_WEIGHT  = 1 << 3,
	FONT_MASK_STRETCH = 1 << 4,
	FONT_MASK_SIZE    = 1 << 5,
	FONT_MASK_GRAVITY = 1 << 6
} GalFontMask;

typedef enum {
	FONT_GRAVITY_SOUTH,
	FONT_GRAVITY_EAST,
	FONT_GRAVITY_NORTH,
	FONT_GRAVITY_WEST,
	FONT_GRAVITY_AUTO
} GalFontGravity;

struct _GalPattern {
	const echar *foundry;
    const echar *family;
    const echar *style;

    GalFontSlant   slant;
    GalFontVariant variant;
    GalFontWeight  weight;
    GalFontStretch stretch;
    GalFontGravity gravity;

    euint16 mask;
    euint size_is_absolute : 1;
 
    euint size;
	ePointer private;
};

eGeneType egal_genetype_font  (void);
GalFont   egal_default_font   (void);
GalFont   egal_font_load      (const echar *font_name);
void      egal_get_glyph      (GalFont, eunichar, GalGlyph *);
eint      egal_text_width     (GalFont, const echar *, eint);
eint      egal_text_height    (GalFont, const echar *, eint);
eint      egal_font_height    (GalFont);
void      egal_draw_glyphs    (GalDrawable, GalPB, GalFont, GalRect *, eint, eint, GalGlyph *, eint);
eint      egal_unichar_width  (GalFont, eunichar);
void      egal_unichar_extents(GalFont, eunichar, GalRect *);

GalMetrics *egal_font_metrics(GalFont);

GalPattern *egal_pattern_new(const echar  *font_name);
const char *egal_pattern_get_file(const GalPattern *);


#endif
