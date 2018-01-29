#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <setjmp.h>
#endif
#include <png.h>

#include <egal/pixbuf-io.h>

static void png_error_callback(png_structp png_read_ptr, png_const_charp error_msg);

static void png_warning_callback(png_structp png_read_ptr, png_const_charp warning_msg);

static void png_info_callback(png_structp png_read_ptr, png_infop png_info_ptr);

static void png_row_callback(png_structp png_read_ptr, png_bytep new_row, png_uint_32 row_num, int pass_num);

static void png_end_callback(png_structp png_read_ptr, png_infop png_info_ptr);

typedef struct _PNGContext PNGContext;

struct _PNGContext {
	PixbufContext context;
	png_structp png_read_ptr;
	png_infop   png_info_ptr;

	euint8 *dptr;
	png_uint_32 width, height;
	int color_type;
	int channels;

	int first_row_seen_in_chunk;
	int first_pass_seen_in_chunk;
	int last_row_seen_in_chunk;
	int last_pass_seen_in_chunk;
	int max_row_seen_in_chunk;

	euint fatal_error_occurred : 1;
};

static ebool setup_png_transformations(PNGContext *lc, png_structp png_read_ptr, png_infop png_info_ptr)
{
	int bit_depth, filter_type, interlace_type, compression_type;

	bit_depth = png_get_bit_depth(png_read_ptr, png_info_ptr);
	if (bit_depth < 1 || bit_depth > 16) {
		printf("Bits per channel of PNG pixbuf is invalid.");
		return efalse;
	}

	png_get_IHDR(png_read_ptr, png_info_ptr,
			&lc->width, &lc->height,
			&bit_depth,
			&lc->color_type,
			&interlace_type,
			&compression_type,
			&filter_type);

	if (lc->color_type == PNG_COLOR_TYPE_PALETTE && bit_depth <= 8)
		png_set_expand(png_read_ptr);
	else if (lc->color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand(png_read_ptr);
	else if (png_get_valid(png_read_ptr, png_info_ptr, PNG_INFO_tRNS))
		png_set_expand(png_read_ptr);
	else if (bit_depth < 8)
		png_set_expand(png_read_ptr);

	if (bit_depth == 16)
		png_set_strip_16(png_read_ptr);

	if (lc->color_type == PNG_COLOR_TYPE_GRAY ||
			lc->color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_read_ptr);

	if (interlace_type != PNG_INTERLACE_NONE)
		png_set_interlace_handling(png_read_ptr);

	png_read_update_info(png_read_ptr, png_info_ptr);

	png_get_IHDR(png_read_ptr, png_info_ptr,
			&lc->width, &lc->height,
			&bit_depth,
			&lc->color_type,
			&interlace_type,
			&compression_type,
			&filter_type);

	if (lc->width == 0 || lc->height == 0) {
		printf("Transformed PNG has zero width or height.");
		return efalse;
	}

	if (bit_depth != 8) {
		printf("Bits per channel of transformed PNG is not 8.");
		return efalse;
	}

	if (!(lc->color_type == PNG_COLOR_TYPE_RGB
				|| lc->color_type == PNG_COLOR_TYPE_RGB_ALPHA)) {
		printf("Transformed PNG not RGB or RGBA.");
		return efalse;
	}

	lc->channels = png_get_channels(png_read_ptr, png_info_ptr);
	if (!(lc->channels == 3 || lc->channels == 4)) {
		printf("Transformed PNG has unsupported number of channels, must be 3 or 4.");
		return efalse;
	}
	return etrue;
}

static png_voidp png_malloc_callback(png_structp o, png_size_t size)
{
	return e_malloc(size);
}

static void png_free_callback(png_structp o, png_voidp x)
{
	e_free(x);
}

static ePointer png_load_begin(void)
{
	PNGContext* lc;

	lc = e_calloc(sizeof(PNGContext), 1);

	lc->fatal_error_occurred = efalse;
	lc->first_row_seen_in_chunk = -1;
	lc->last_row_seen_in_chunk = -1;
	lc->first_pass_seen_in_chunk = -1;
	lc->last_pass_seen_in_chunk = -1;
	lc->max_row_seen_in_chunk = -1;

#ifdef PNG_USER_MEM_SUPPORTED
	lc->png_read_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
			lc,
			png_error_callback,
			png_warning_callback,
			NULL,
			png_malloc_callback,
			png_free_callback);
#else
	lc->png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			lc,
			png_error_callback,
			png_warning_callback);
#endif
	if (lc->png_read_ptr == NULL) {
		e_free(lc);
		return NULL;
	}

	//if (setjmp(lc->png_read_ptr->jmpbuf)) {
	//	if (lc->png_info_ptr)
	//		png_destroy_read_struct(&lc->png_read_ptr, NULL, NULL);
	//	e_free(lc);
	//	return NULL;
	//}

	lc->png_info_ptr = png_create_info_struct(lc->png_read_ptr);

	if (lc->png_info_ptr == NULL) {
		png_destroy_read_struct(&lc->png_read_ptr, NULL, NULL);
		e_free(lc);
		return NULL;
	}

	png_set_progressive_read_fn(lc->png_read_ptr,
			lc,
			png_info_callback,
			png_row_callback,
			png_end_callback);

	return lc;
}

static GalPixbuf *png_load_end(ePointer context)
{
	PNGContext *lc = context;
	GalPixbuf *pixbuf = lc->context.pixbuf;

	if (!lc) return NULL;

	png_destroy_read_struct(&lc->png_read_ptr, &lc->png_info_ptr, NULL);

	e_free(lc);

	if (lc->fatal_error_occurred && pixbuf) {
		egal_pixbuf_free(pixbuf);
		pixbuf = NULL;
	}

	return pixbuf;
}

static ebool png_load_increment(ePointer context, const euchar *buf, png_size_t size)
{
	PNGContext *lc = context;

	if (!lc) return efalse;

	lc->first_row_seen_in_chunk = -1;
	lc->last_row_seen_in_chunk = -1;
	lc->first_pass_seen_in_chunk = -1;
	lc->last_pass_seen_in_chunk = -1;
	lc->max_row_seen_in_chunk = -1;

	//if (setjmp(lc->png_read_ptr->jmpbuf))
	//	return efalse;
	//else
		png_process_data(lc->png_read_ptr, lc->png_info_ptr, (png_bytep)buf, size);

	if (lc->fatal_error_occurred)
		return efalse;

	return etrue;
}

static void
png_info_callback(png_structp png_read_ptr, png_infop png_info_ptr)
{
	PNGContext *lc;
	PixbufContext *con;
	ebool have_alpha = efalse;

	lc = png_get_progressive_ptr(png_read_ptr);

	if (lc->fatal_error_occurred)
		return;

	if (!setup_png_transformations(lc, png_read_ptr, png_info_ptr)) {
		lc->fatal_error_occurred = etrue;
		return;
	}

	if (lc->color_type & PNG_COLOR_MASK_ALPHA)
		have_alpha = etrue;

	con = (PixbufContext *)lc;
#ifdef WIN32
	con->init(con, lc->width, lc->height, lc->channels, 1);
#else
	con->init(con, lc->width, lc->height, lc->channels, 0);
#endif

	return;
}

static void
png_row_callback(png_structp png_read_ptr, png_bytep new_row, png_uint_32 row_num, int pass_num)
{
	PNGContext *lc;
	euint8 *p = new_row;
	euint i;
	PixbufContext *con;

	lc = png_get_progressive_ptr(png_read_ptr);

	if (lc->fatal_error_occurred)
		return;

	if (row_num >= lc->height) {
		lc->fatal_error_occurred = etrue;
		return;
	}

	if (lc->first_row_seen_in_chunk < 0) {
		lc->first_row_seen_in_chunk = row_num;
		lc->first_pass_seen_in_chunk = pass_num;
	}

	lc->max_row_seen_in_chunk = MAX(lc->max_row_seen_in_chunk, ((eint)row_num));
	lc->last_row_seen_in_chunk = row_num;
	lc->last_pass_seen_in_chunk = pass_num;

	con = (PixbufContext *)lc;
	//if (con->set_y(con, row_num))
	if (con->set_y(con, -1)) {
		for (i = 0; i < lc->width; i++) {
			if (lc->channels == 3) {
				con->set_x(con, -1, p[2], p[1], p[0], 0xff);
				p += 3;
			}
			else if (lc->channels == 4) {
				con->set_x(con, -1, p[2], p[1], p[0], p[3]);
				p += 4;
			}
		}
	}
}

static void
png_end_callback(png_structp png_read_ptr, png_infop png_info_ptr)
{
	PNGContext* lc;

	lc = png_get_progressive_ptr(png_read_ptr);

	if (lc->fatal_error_occurred)
		return;
}

static void
png_error_callback(png_structp png_read_ptr, png_const_charp error_msg)
{
	PNGContext* lc;

	lc = png_get_error_ptr(png_read_ptr);

	lc->fatal_error_occurred = etrue;

	//longjmp(png_read_ptr->jmpbuf, 1);
}

static void
png_warning_callback(png_structp png_read_ptr, png_const_charp warning_msg)
{
	PNGContext* lc;
	lc = png_get_error_ptr(png_read_ptr);
}

static void png_info_callback_head(png_structp png_read_ptr, png_infop png_info_ptr)
{
	GalPixbuf *pixbuf;
	png_uint_32 width, height;
	int color_type, interlace_type;
	int compression_type, filter_type;
	int bit_depth;

	pixbuf = png_get_progressive_ptr(png_read_ptr);

	bit_depth = png_get_bit_depth(png_read_ptr, png_info_ptr);
	if (bit_depth < 1 || bit_depth > 16) {
		printf("Bits per channel of PNG pixbuf is invalid.");
		return;
	}

	png_get_IHDR(png_read_ptr, png_info_ptr,
			&width, &height, &bit_depth,
			&color_type, &interlace_type, &compression_type, &filter_type);
	pixbuf->w = width;
	pixbuf->h = height;
}

static ebool png_load_header(GalPixbuf *pixbuf, ePointer data, euint len)
{
	png_structp png_read_ptr;
	png_infop   png_info_ptr;

#ifdef PNG_USER_MEM_SUPPORTED
	png_read_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,
			NULL,
			png_error_callback,
			png_warning_callback,
			NULL,
			png_malloc_callback,
			png_free_callback);
#else
	png_read_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL,
			png_error_callback,
			png_warning_callback);
#endif
	if (!png_read_ptr) return efalse;

	//if (setjmp(png_read_ptr->jmpbuf)) {
	//	if (png_info_ptr)
	//		png_destroy_read_struct(&png_read_ptr, NULL, NULL);
	//	return efalse;
	//}

	png_info_ptr = png_create_info_struct(png_read_ptr);
	if (!png_info_ptr) {
		png_destroy_read_struct(&png_read_ptr, NULL, NULL);
		return efalse;
	}

	png_set_progressive_read_fn(png_read_ptr,
			pixbuf,
			png_info_callback_head,
			NULL, NULL);

	png_process_data(png_read_ptr, png_info_ptr, data, len);

	png_destroy_read_struct(&png_read_ptr, &png_info_ptr, NULL);
	return etrue;
}

void _gal_pixbuf_png_fill_vtable(GalPixbufModule *module)
{
	module->load_begin     = png_load_begin;
	module->load_end       = png_load_end;
	module->load_header    = png_load_header;
	module->load_increment = png_load_increment;
	//module->save = png_save;
	//module->save_to_callback = png_save_to_callback;
}
