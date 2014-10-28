#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <pixbuf-io.h>

void _gal_pixbuf_bmp_fill_vtable(GalPixbufModule *module);
void _gal_pixbuf_gif_fill_vtable(GalPixbufModule *module);
#ifdef _GAL_SUPPORT_JPEG
void _gal_pixbuf_jpeg_fill_vtable(GalPixbufModule *module);
#endif

#ifdef _GAL_SUPPORT_PNG
void _gal_pixbuf_png_fill_vtable(GalPixbufModule *module);
#endif

static GalPixbufModule pixbuf_modules[] = {
	{"bmp", false, _gal_pixbuf_bmp_fill_vtable},
	{"gif", false, _gal_pixbuf_gif_fill_vtable},
#ifdef _GAL_SUPPORT_JPEG
	{"jpg", false, _gal_pixbuf_jpeg_fill_vtable},
#endif
#ifdef _GAL_SUPPORT_PNG
	{"png", false, _gal_pixbuf_png_fill_vtable},
#endif
#ifdef _GAL_SUPPORT_ICO
	{"ico", false, _gal_pixbuf_ico_fill_vtable},
#endif
	{NULL},
};

#define QUOTA_RGB(r, g, b, a, quota_x, quota_y) \
{ \
	_r = ( r * quota_x) >> SCALE_SHIFT; \
	_g = ( g * quota_x) >> SCALE_SHIFT; \
	_b = ( b * quota_x) >> SCALE_SHIFT; \
	_a = ( a * quota_x) >> SCALE_SHIFT; \
\
	_r = (_r * quota_y) >> SCALE_SHIFT; \
	_g = (_g * quota_y) >> SCALE_SHIFT; \
	_b = (_b * quota_y) >> SCALE_SHIFT; \
	_a = (_a * quota_y) >> SCALE_SHIFT; \
}

static INLINE void set_pixel32(euchar *pixels, euint8 r, euint8 g, euint8 b, euint8 a)
{
	pixels[0] += r;
	pixels[1] += g;
	pixels[2] += b;
	pixels[3] += a;
}

static INLINE void set_pixel24(euchar *pixels, euint8 r, euint8 g, euint8 b, euint8 a)
{
	pixels[0] += r;
	pixels[1] += g;
	pixels[2] += b;
}

static void _pixbuf_io_set_pixels(
		void (*set_pixel)(euchar *, euint8, euint8, euint8, euint8),
		euchar *pixels, eint bytes,
		euint8 r, euint8 g, euint8 b, euint8 a, eint w,
		euint32 dx, euint32 x_step, euint32 quota_y)
{
	euint32 _r, _g, _b, _a;
	euint32 quota_x, over_x, _dx;
	eint n;

	_dx  = dx;
	 dx += x_step;
	pixels += bytes * (_dx >> SCALE_SHIFT);

	if (dx >> SCALE_SHIFT > _dx >> SCALE_SHIFT) {
		quota_x = (1 << SCALE_SHIFT) - (_dx & SCALE_MASK);
		n = (dx >> SCALE_SHIFT) - (_dx >> SCALE_SHIFT) - 1;
		over_x  = dx & SCALE_MASK;
	}
	else {
		n = 0;
		over_x = 0;
		quota_x = x_step;
	}

	if (n + (_dx >> SCALE_SHIFT) >= w - 1) {
		n = w - 1 - (_dx >> SCALE_SHIFT);
		if (n < 0)
			n = 0;
		over_x = 0;
	}

	QUOTA_RGB(r, g, b, a, quota_x, quota_y);
	set_pixel(pixels, _r, _g, _b, _a);

	if (n > 0) {
		QUOTA_RGB(r, g, b, a, (1 << SCALE_SHIFT), quota_y);
		do {
			pixels += bytes;
			set_pixel(pixels, _r, _g, _b, _a);
		} while (--n > 0);
	}

	if (over_x) {
		pixels += bytes;
		QUOTA_RGB(r, g, b, a, over_x, quota_y);
		set_pixel(pixels, _r, _g, _b, _a);
	}
}

static void pixbuf_io_set_x(PixbufContext *context, eint x, euint8 r, euint8 g, euint8 b, euint8 a)
{
	if (x >= 0)
		context->dx = x * context->x_step;

	if (context->y >= 0 && context->y < context->h) {
		GalPixbuf *pixbuf = context->pixbuf;
		euchar    *pixels = context->pixels;
		eint ny = context->ny;

		_pixbuf_io_set_pixels(context->set_pixel,
				pixels, pixbuf->pixelbytes, r, g, b, a,
				pixbuf->w, context->dx, context->x_step, context->quota_y);

		if (ny > 0) {
			do {
				pixels += context->rowbytes;
				_pixbuf_io_set_pixels(context->set_pixel,
						pixels, pixbuf->pixelbytes, r, g, b, a,
						pixbuf->w, context->dx, context->x_step, 1 << SCALE_SHIFT);
			} while (--ny > 0);
		}

		if (context->over_y > 0) {
			pixels += context->rowbytes;
			_pixbuf_io_set_pixels(context->set_pixel, 
					pixels, pixbuf->pixelbytes, r, g, b, a, 
					pixbuf->w, context->dx, context->x_step, context->over_y);
		}

		context->dx += context->x_step;
	}
}

static bool pixbuf_io_set_y(PixbufContext *context, eint y)
{
	GalPixbuf *pixbuf = context->pixbuf;
	euint32  dy, _dy;

	if (y >= 0) context->dy = y * context->y_step;

	if (context->negative == 0) {
		_dy = context->shift_h -  context->dy;
		 dy = context->shift_h - (context->dy + context->y_step);
		if (dy >> SCALE_SHIFT < _dy >> SCALE_SHIFT) {
			context->quota_y =  _dy & SCALE_MASK;
			if (context->quota_y == 0)
				context->quota_y = 1 << SCALE_SHIFT;
			context->ny      = (_dy >> SCALE_SHIFT) - (dy >> SCALE_SHIFT) - 1;
			context->over_y  = (1 << SCALE_SHIFT) - (dy & SCALE_MASK);
			context->over_y &= SCALE_MASK;
		}
		else {
			context->ny      = 0;
			context->over_y  = 0;
			context->quota_y =  _dy - dy;
		}
		context->y = (_dy >> SCALE_SHIFT) - 1;
		if (context->y - context->ny <= 0) {
			context->ny = context->y;
			context->over_y = 0;
		}
	}
	else {
		_dy = context->dy;
		 dy = context->dy + context->y_step;
		if (dy >> SCALE_SHIFT > _dy >> SCALE_SHIFT) {
			context->quota_y = (1 << SCALE_SHIFT) - (_dy & SCALE_MASK);
			context->ny      = (dy >> SCALE_SHIFT) - (_dy >> SCALE_SHIFT) - 1;
			context->over_y  =  dy & SCALE_MASK;
		}
		else {
			context->ny      = 0;
			context->over_y  = 0;
			context->quota_y = context->y_step;
		}
		context->y = _dy >> SCALE_SHIFT;
		if (context->y + context->ny >= pixbuf->h - 1) {
			context->ny = pixbuf->h - 1 - context->y;
			context->over_y = 0;
		}
	}
	context->dy += context->y_step;

	if (context->y < 0 || context->y >= pixbuf->h)
		return false;

	context->pixels = pixbuf->pixels + context->y * pixbuf->rowbytes;
	context->dx = 0;

	return true;
}

static eint pixbuf_io_init(PixbufContext *context, eint w, eint h, eint channels, eint negative)
{
	GalPixbuf *pixbuf;

	if (context->pixbuf)
		return 0;

	if (context->x_scale == 1.0) {
		context->shift_w = w << SCALE_SHIFT;
		context->x_step  = 1 << SCALE_SHIFT;
	}
	else {
		context->x_step  = context->x_scale * (1 << SCALE_SHIFT);
		context->shift_w = context->x_step * w;
	}

	if (context->y_scale == 1.0) {
		context->shift_h = h << SCALE_SHIFT;
		context->y_step  = 1 << SCALE_SHIFT;
	}
	else {
		context->y_step  = context->y_scale * (1 << SCALE_SHIFT);
		context->shift_h = context->x_step * h;
	}

	context->w = context->shift_w >> SCALE_SHIFT;
	context->h = context->shift_h >> SCALE_SHIFT;
	context->pixbuf = egal_pixbuf_new((channels == 4), context->w, context->h);
	if (!context->pixbuf)
		return -1;

	pixbuf = context->pixbuf;
	e_memset(pixbuf->pixels, 0, pixbuf->h * pixbuf->rowbytes);
	context->pixels = pixbuf->pixels;

	if (pixbuf->pixelbytes == 4)
		context->set_pixel = set_pixel32;
	else
		context->set_pixel = set_pixel24;

	if (negative == 0)
		context->rowbytes = -pixbuf->rowbytes;
	else
		context->rowbytes =  pixbuf->rowbytes;

	context->negative = negative;
	context->dy = 0;

	context->set_x = pixbuf_io_set_x;
	context->set_y = pixbuf_io_set_y;

	return 0;
}

static GalPixbufModule *pixbuf_module_get(echar *buffer, const echar *format_name)
{
	GalPixbufModule *module = pixbuf_modules;

	while (module->name) {
		if (!e_strcmp((echar *)module->name, format_name)) {
			if (!module->fill) {
				module->fill_vtable(module);
				module->fill = true;
			}
			return module;
		}
		module++;
	}

	return NULL;
}

static GalPixbuf *pixbuf_module_load(GalPixbufModule *module, FILE *f, efloat x_scale, efloat y_scale)
{
	PixbufContext *context;
	GalPixbuf     *pixbuf = NULL;

	euchar buffer[4096];
	size_t len;

	if (module->load_begin) {
		context = module->load_begin();
		if (!context) goto err;

		context->pixbuf  = NULL;
		context->x_scale = x_scale;
		context->y_scale = y_scale;
		context->init    = pixbuf_io_init;

		while (!feof(f) && !ferror(f)) {
			len = fread(buffer, 1, sizeof(buffer), f);
			if (!module->load_increment(context, buffer, len))
				break;
		}

		pixbuf = module->load_end(context);
	}

err:
	return pixbuf;
}

GalPixbuf *egal_pixbuf_new_from_file(const echar *filename, efloat x_scale, efloat y_scale)
{
	GalPixbuf *pixbuf;
	FILE *f;
	echar buffer[64];
	GalPixbufModule *pixbuf_module;
	echar *format_name;

	if (filename == NULL)
		return NULL;

	format_name = e_strrchr((const echar *)filename, '.');
	if (!format_name)
		return NULL;
	format_name++;

	pixbuf_module = pixbuf_module_get(buffer, format_name);
	if (pixbuf_module == NULL)
		return NULL;

	f = fopen((const char *)filename, "rb");
	fseek(f, 0, SEEK_SET);
	pixbuf = pixbuf_module_load(pixbuf_module, f, x_scale, y_scale);
	fclose(f);

	return pixbuf;
}

bool egal_pixbuf_load_header(const echar *filename, GalPixbuf *pixbuf)
{
	FILE *f;
	echar buffer[4096];
	GalPixbufModule *pixbuf_module;
	echar *format_name;

	if (filename == NULL)
		return false;

	format_name = e_strrchr(filename, '.');
	if (!format_name)
		return false;
	format_name++;

	pixbuf_module = pixbuf_module_get(buffer, (echar *)format_name);
	if (pixbuf_module == NULL)
		return false;

	f = fopen((const char *)filename, "rb");
	fseek(f, 0, SEEK_SET);

	if (pixbuf_module->load_begin) {
		int len = fread(buffer, 1, sizeof(buffer), f);
		return pixbuf_module->load_header(pixbuf, buffer, len);
	}

	return false;
}

static GalPixbufAnim *pixbuf_module_load_anim(GalPixbufModule *module, FILE *f, efloat x_scale, efloat y_scale)
{
	PixbufContext *context;
	GalPixbufAnim *anim = NULL;

	euchar buffer[4096];
	size_t len;

	if (module->anim_load_begin) {
		context = module->anim_load_begin();
		if (!context)
			goto err;

		context->pixbuf  = NULL;
		context->x_scale = x_scale;
		context->y_scale = y_scale;
		context->init    = pixbuf_io_init;

		while (!feof(f) && !ferror(f)) {
			len = fread(buffer, 1, sizeof(buffer), f);
			if (!module->load_increment(context, buffer, len))
				break;
		}

		anim = module->anim_load_end(context);
	}

err:
	return anim;
}

GalPixbufAnim *egal_pixbuf_anim_new_from_file(const echar *filename, efloat x_scale, efloat y_scale)
{
	GalPixbufAnim *anim;
	FILE *f;
	echar buffer[64];
	GalPixbufModule *pixbuf_module;
	echar *format_name;

	if (filename == NULL)
		return NULL;

	format_name = e_strrchr((const echar *)filename, '.');
	if (!format_name)
		return NULL;
	format_name++;

	pixbuf_module = pixbuf_module_get(buffer, format_name);
	if (pixbuf_module == NULL)
		return NULL;

	f = fopen((const char *)filename, "rb");
	fseek(f, 0, SEEK_SET);
	anim = pixbuf_module_load_anim(pixbuf_module, f, x_scale, y_scale);
	fclose(f);

	return anim;
}

