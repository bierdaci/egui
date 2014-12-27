#ifdef linux
#include <fontconfig/fontconfig.h>
#endif

#include <egal/font.h>

#define GTYPE_GAL_PATTERN				(egal_genetype_pattern())
#define GAL_PATTERN_DATA(hobj)			((GalPatternData *)e_object_type_orders(hobj, GTYPE_GAL_PATTERN))

#ifdef linux
static int fc_convert_slant(GalFontSlant slant)
{
	switch (slant) {
		case FONT_SLANT_NORMAL:
			return FC_SLANT_ROMAN;
		case FONT_SLANT_ITALIC:
			return FC_SLANT_ITALIC;
		case FONT_SLANT_OBLIQUE:
			return FC_SLANT_OBLIQUE;
		default:
			return FC_SLANT_ROMAN;
	}
}

static int fc_convert_weight(GalFontWeight weight)
{
	if (weight < (FONT_WEIGHT_ULTRALIGHT + FONT_WEIGHT_LIGHT) / 2)
		return FC_WEIGHT_ULTRALIGHT;
	else if (weight < (FONT_WEIGHT_LIGHT + FONT_WEIGHT_NORMAL) / 2)
		return FC_WEIGHT_LIGHT;
	else if (weight < (FONT_WEIGHT_NORMAL + 500) / 2)
		return FC_WEIGHT_NORMAL;
	else if (weight < (500 + FONT_WEIGHT_SEMIBOLD) / 2)
		return FC_WEIGHT_MEDIUM;
	else if (weight < (FONT_WEIGHT_SEMIBOLD + FONT_WEIGHT_BOLD) / 2)
		return FC_WEIGHT_DEMIBOLD;
	else if (weight < (FONT_WEIGHT_BOLD + FONT_WEIGHT_ULTRABOLD) / 2)
		return FC_WEIGHT_BOLD;
	else if (weight < (FONT_WEIGHT_ULTRABOLD + FONT_WEIGHT_HEAVY) / 2)
		return FC_WEIGHT_ULTRABOLD;
	else
		return FC_WEIGHT_BLACK;
}

#endif

static void fc_make_pattern(GalPattern *gpn)
{
#ifdef linux
	FcPattern *fcp;
	FcFontSet *font_patterns;
	echar    **families;
	FcResult   result;
	eint i;

	fcp = FcPatternBuild(NULL,
			FC_WEIGHT, FcTypeInteger, fc_convert_weight(gpn->weight),
			FC_SLANT,  FcTypeInteger, fc_convert_slant(gpn->slant),
			NULL);

	if (gpn->size > 0)
		FcPatternAddDouble(fcp, FC_PIXEL_SIZE, gpn->size / 10.);

	if (gpn->foundry)
		FcPatternAddString(fcp, FC_FOUNDRY, (euchar *)gpn->foundry);

	if (gpn->family) {
		families = e_strsplit(gpn->family, _(","), -1);
		for (i = 0; families[i]; i++)
			FcPatternAddString(fcp, FC_FAMILY, (euchar *)families[i]);
		e_strfreev(families);
	}

	/*
	if (language)
		FcPatternAddString(fcp, FC_LANG, (FcChar8 *)pango_language_to_string(language));

	if (gravity != PANGO_GRAVITY_SOUTH) {
		GEnumValue *value = g_enum_get_value(get_gravity_class(), gravity);
		FcPatternAddString(fcp, PANGO_FC_GRAVITY, value->value_nick);
	}
	*/

	FcConfigSubstitute(0, fcp, FcMatchPattern);
	FcDefaultSubstitute(fcp);

	//gpn->private = FcFontMatch(NULL, fcp, &result);

	font_patterns = FcFontSort(NULL, fcp, FcTrue, NULL, &result);
	FcFontRenderPrepare(NULL, fcp, font_patterns->fonts[0]);

#if 0
	for (i = 0; i < font_patterns->nfont; i++) {
		FcPattern *t = FcFontRenderPrepare(NULL, fcp, font_patterns->fonts[i]);
		FcChar8   *filename = NULL;
		double size;

		if (FcPatternGetString(t, FC_FOUNDRY, 0, &filename) == FcResultMatch)
			printf("foundry: %-10s ", filename);

		if (FcPatternGetString(t, FC_FAMILY, 0, &filename) == FcResultMatch)
			printf("family: %-30s ", filename);

		if (FcPatternGetString(t, FC_STYLELANG, 0, &filename) == FcResultMatch)
			printf("style: %-20s ", filename);

		if (FcPatternGetDouble(t, FC_SIZE, 0, &size) == FcResultMatch)
			printf("  %f ", size);

		if (FcPatternGetDouble(t, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
			printf("%f", size);

		printf("\n");
	}
#endif

	gpn->private = font_patterns->fonts[0];
	FcFontSetSortDestroy(font_patterns);
	FcPatternDestroy(fcp);
#endif
}

const char *egal_pattern_get_file(const GalPattern *gpn)
{
#ifdef linux
	FcPattern *fcp = (FcPattern *)gpn->private;
	FcChar8   *filename;
	if (FcPatternGetString(fcp, FC_FILE, 0, &filename) == FcResultMatch)
		return (const char *)filename;
#endif
	return NULL;
}

GalPattern *egal_pattern_new(const echar *font_name)
{
	GalPattern *gpn    = e_calloc(1, sizeof(GalPattern));
	echar     **pieces = e_strsplit(font_name, _("-"), 9);

	do {
		if (!pieces[0])
			break;

		if (!pieces[1])
			break;
		if (e_strcmp(pieces[1], _("*")) != 0)
			gpn->foundry = pieces[1];

		if (!pieces[2])
			break;
		if (e_strcmp(pieces[2], _("*")) != 0)
			gpn->family = pieces[2];

		if (!pieces[3])
			break;

		if (e_strcmp(pieces[3], _("light")) == 0)
			gpn->weight = FONT_WEIGHT_LIGHT;
		if (e_strcmp(pieces[3], _("medium")) == 0)
			gpn->weight = FONT_WEIGHT_NORMAL;
		if (e_strcmp(pieces[3], _("bold")) == 0)
			gpn->weight = FONT_WEIGHT_BOLD;

		if (!pieces[4])
			break;

		if (e_strcmp(pieces[4], _("r")) == 0)
			gpn->slant = FONT_SLANT_NORMAL;
		if (e_strcmp(pieces[4], _("i")) == 0)
			gpn->slant = FONT_SLANT_ITALIC;
		if (e_strcmp(pieces[4], _("o")) == 0)
			gpn->slant = FONT_SLANT_OBLIQUE;

		if (!pieces[5])
			break;

		if (e_strcmp(pieces[5], _("*")) != 0)
			gpn->style = pieces[5];

		if (!pieces[6])
			break;
		if (!pieces[7])
			break;

		if (e_strcmp(pieces[8], _("*")) != 0)
			gpn->size = e_atoi(pieces[8]);

		if (gpn->size == 0)
			gpn->size = 12;

	} while (0);

	e_strfreev(pieces);

	fc_make_pattern(gpn);

	return gpn;
}

//FcPatternGetMatrix(pattern, FC_MATRIX, 0, &fc_matrix) == FcResultMatch)
