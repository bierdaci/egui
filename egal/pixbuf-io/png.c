#include <stdio.h>
#include <stdlib.h>
#include <png.h>

#include <pixbuf-io.h>

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

static bool setup_png_transformations(PNGContext *lc, png_structp png_read_ptr, png_infop png_info_ptr)
{
	int bit_depth, filter_type, interlace_type, compression_type;

	bit_depth = png_get_bit_depth(png_read_ptr, png_info_ptr);
	if (bit_depth < 1 || bit_depth > 16) {
		printf("Bits per channel of PNG pixbuf is invalid.");
		return false;
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
		return false;
	}

	if (bit_depth != 8) {
		printf("Bits per channel of transformed PNG is not 8.");
		return false;
	}

	if (!(lc->color_type == PNG_COLOR_TYPE_RGB
				|| lc->color_type == PNG_COLOR_TYPE_RGB_ALPHA)) {
		printf("Transformed PNG not RGB or RGBA.");
		return false;
	}

	lc->channels = png_get_channels(png_read_ptr, png_info_ptr);
	if (!(lc->channels == 3 || lc->channels == 4)) {
		printf("Transformed PNG has unsupported number of channels, must be 3 or 4.");
		return false;
	}
	return true;
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

	lc->fatal_error_occurred = false;
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

	if (setjmp(lc->png_read_ptr->jmpbuf)) {
		if (lc->png_info_ptr)
			png_destroy_read_struct(&lc->png_read_ptr, NULL, NULL);
		e_free(lc);
		return NULL;
	}

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

static bool png_load_increment(ePointer context, const euchar *buf, size_t size)
{
	PNGContext *lc = context;

	if (!lc) return false;

	lc->first_row_seen_in_chunk = -1;
	lc->last_row_seen_in_chunk = -1;
	lc->first_pass_seen_in_chunk = -1;
	lc->last_pass_seen_in_chunk = -1;
	lc->max_row_seen_in_chunk = -1;

	if (setjmp(lc->png_read_ptr->jmpbuf))
		return false;
	else
		png_process_data(lc->png_read_ptr, lc->png_info_ptr, (png_bytep)buf, size);

	if (lc->fatal_error_occurred)
		return false;

	return true;
}

static void
png_info_callback(png_structp png_read_ptr, png_infop png_info_ptr)
{
	PNGContext *lc;
	bool have_alpha = false;

	lc = png_get_progressive_ptr(png_read_ptr);

	if (lc->fatal_error_occurred)
		return;

	if (!setup_png_transformations(lc, png_read_ptr, png_info_ptr)) {
		lc->fatal_error_occurred = true;
		return;
	}

	if (lc->color_type & PNG_COLOR_MASK_ALPHA)
		have_alpha = true;

	PixbufContext *con = (PixbufContext *)lc;
	con->init(con, lc->width, lc->height, lc->channels, 1);

	return;
}

static void
png_row_callback(png_structp png_read_ptr, png_bytep new_row, png_uint_32 row_num, int pass_num)
{
	PNGContext *lc;
	euint8 *p = new_row;
	int i;
	PixbufContext *con;

	lc = png_get_progressive_ptr(png_read_ptr);

	if (lc->fatal_error_occurred)
		return;

	if (row_num >= lc->height) {
		lc->fatal_error_occurred = true;
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

	lc->fatal_error_occurred = true;

	longjmp(png_read_ptr->jmpbuf, 1);
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

static bool png_load_header(GalPixbuf *pixbuf, ePointer data, euint len)
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
	if (!png_read_ptr) return false;

	if (setjmp(png_read_ptr->jmpbuf)) {
		if (png_info_ptr)
			png_destroy_read_struct(&png_read_ptr, NULL, NULL);
		return false;
	}

	png_info_ptr = png_create_info_struct(png_read_ptr);
	if (!png_info_ptr) {
		png_destroy_read_struct(&png_read_ptr, NULL, NULL);
		return false;
	}

	png_set_progressive_read_fn(png_read_ptr,
			pixbuf,
			png_info_callback_head,
			NULL, NULL);

	png_process_data(png_read_ptr, png_info_ptr, data, len);

	png_destroy_read_struct(&png_read_ptr, &png_info_ptr, NULL);
	return true;
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

#if 0
static bool
png_text_to_pixbuf_option(png_text text_ptr, char **key, char **value)
{
	bool is_ascii = true;
	euint i;

	for (i = 0; i < text_ptr.text_length; i++)
		if (text_ptr.text[i] & 0x80) {
			is_ascii = false;
			break;
		}

	if (is_ascii)
		*value = e_strdup(text_ptr.text);
	else
		*value = g_convert(text_ptr.text, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL);

	if (*value) {
		*key = g_strconcat("tEXt::", text_ptr.key, NULL);
		return true;
	}
	else {
		//g_warning("Couldn't convert text chunk value to UTF-8.");
		*key = NULL;
		return false;
	}
}
static void
png_simple_error_callback(png_structp png_save_ptr, png_const_charp error_msg)
{
	//GError **error;

	//error = png_get_error_ptr(png_save_ptr);

	///* I don't trust libpng to call the error callback only once,
	// * so check for already-set error
	// */
	//if (error && *error == NULL) {
	//	g_set_error(error,
	//			GDK_PIXBUF_ERROR,
	//			GDK_PIXBUF_ERROR_FAILED,
	//			_("Fatal error in PNG pixbuf file: %s"),
	//			error_msg);
	//}

	longjmp(png_save_ptr->jmpbuf, 1);
}

static void
png_simple_warning_callback(png_structp png_save_ptr, png_const_charp warning_msg)
{
}

typedef struct {
	GalPixbufSaveFunc save_func;
	ePointer user_data;
	GError **error;
} SaveToFunctionIoPtr;

static void
png_save_to_callback_write_func(png_structp png_ptr, png_bytep data, png_size_t length)
{
	SaveToFunctionIoPtr *ioptr = png_get_io_ptr (png_ptr);

	if (!ioptr->save_func((euchar *)data, length, ioptr->error, ioptr->user_data)) {
		/* If save_func has already set an error, which it
		   should have done, this won't overwrite it. */
		png_error(png_ptr, "write function failed");
	}
}

static void png_save_to_callback_flush_func(png_structp png_ptr)
{
}

static bool real_save_png(GalPixbuf *pixbuf, 
		euchar           **keys,
		euchar           **values,
		bool          to_callback,
		FILE             *f,
		GalPixbufSaveFunc save_func,
		ePointer          user_data)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_textp text_ptr = NULL;
	euchar *ptr;
	euchar *pixels;
	euint y;
	euint i;
	png_bytep row_ptr;
	png_color_8 sig_bit;
	euint w, h, rowstride;
	euint has_alpha;
	euint bpc;
	euint num_keys;
	euint compression = -1;
	bool success = true;
	SaveToFunctionIoPtr to_callback_ioptr;

	num_keys = 0;

	if (keys && *keys) {
		euchar **kiter = keys;
		euchar **viter = values;

		while (*kiter) {
			if (strncmp (*kiter, "tEXt::", 6) == 0) {
				euchar  *key = *kiter + 6;
				euint     len = strlen (key);
				if (len <= 1 || len > 79) {
					g_set_error (error,
							GDK_PIXBUF_ERROR,
							GDK_PIXBUF_ERROR_BAD_OPTION,
							_("Keys for PNG text chunks must have at least 1 and at most 79 characters."));
					return false;
				}
				for (i = 0; i < len; i++) {
					if ((euchar)key[i] > 127) {
						printf("Keys for PNG text chunks must be ASCII characters.");
						return false;
					}
				}
				num_keys++;
			} else if (strcmp (*kiter, "compression") == 0) {
				char *endptr = NULL;
				compression = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error(error,
							GDK_PIXBUF_ERROR,
							GDK_PIXBUF_ERROR_BAD_OPTION,
							_("PNG compression level must be a value between 0 and 9; value '%s' could not be parsed."),
							*viter);
					return false;
				}
				if (compression < 0 || compression > 9) {
					/* This is a user-visible error;
					 * lets people skip the range-checking
					 * in their app.
					 */
					g_set_error (error,
							GDK_PIXBUF_ERROR,
							GDK_PIXBUF_ERROR_BAD_OPTION,
							_("PNG compression level must be a value between 0 and 9; value '%d' is not allowed."),
							compression);
					return false;
				}
			} else {
				g_warning ("Unrecognized parameter (%s) passed to PNG saver.", *kiter);
			}

			++kiter;
			++viter;
		}
	}

	if (num_keys > 0) {
		text_ptr = e_calloc(sizeof(png_text), num_keys);
		for (i = 0; i < num_keys; i++) {
			text_ptr[i].compression = PNG_TEXT_COMPRESSION_NONE;
			text_ptr[i].key  = keys[i] + 6;
			text_ptr[i].text = g_convert(values[i], -1, 
					"ISO-8859-1", "UTF-8", 
					NULL, &text_ptr[i].text_length, 
					NULL);

#ifdef PNG_iTXt_SUPPORTED 
			if (!text_ptr[i].text) {
				text_ptr[i].compression = PNG_ITXT_COMPRESSION_NONE;
				text_ptr[i].text = e_strdup(values[i]);
				text_ptr[i].text_length = 0;
				text_ptr[i].itxt_length = e_strlen(text_ptr[i].text);
				text_ptr[i].lang = NULL;
				text_ptr[i].lang_key = NULL;
			}
#endif

			if (!text_ptr[i].text) {
				g_set_error (error,
						GDK_PIXBUF_ERROR,
						GDK_PIXBUF_ERROR_BAD_OPTION,
						_("Value for PNG text chunk %s cannot be converted to ISO-8859-1 encoding."), keys[i] + 6);
				num_keys = i;
				for (i = 0; i < num_keys; i++)
					e_free(text_ptr[i].text);
				e_free(text_ptr);
				return false;
			}
		}
	}

	bpc = gdk_pixbuf_get_bits_per_sample(pixbuf);
	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
			error,
			png_simple_error_callback,
			png_simple_warning_callback);
	if (png_ptr == NULL) {
		success = false;
		goto cleanup;
	}

	info_ptr = png_create_info_struct (png_ptr);
	if (info_ptr == NULL) {
		success = false;
		goto cleanup;
	}
	if (setjmp(png_ptr->jmpbuf)) {
		success = false;
		goto cleanup;
	}

	if (num_keys > 0) {
		png_set_text(png_ptr, info_ptr, text_ptr, num_keys);
	}

	if (to_callback) {
		to_callback_ioptr.save_func = save_func;
		to_callback_ioptr.user_data = user_data;
		to_callback_ioptr.error = error;
		png_set_write_fn (png_ptr, &to_callback_ioptr,
				png_save_to_callback_write_func,
				png_save_to_callback_flush_func);
	} else {
		png_init_io(png_ptr, f);
	}

	if (compression >= 0)
		png_set_compression_level(png_ptr, compression);

	if (has_alpha) {
		png_set_IHDR(png_ptr, info_ptr, w, h, bpc,
				PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	} else {
		png_set_IHDR(png_ptr, info_ptr, w, h, bpc,
				PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	}
	sig_bit.red = bpc;
	sig_bit.green = bpc;
	sig_bit.blue = bpc;
	sig_bit.alpha = bpc;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	png_write_info(png_ptr, info_ptr);
	png_set_shift(png_ptr, &sig_bit);
	png_set_packing(png_ptr);

	ptr = pixels;
	for (y = 0; y < h; y++) {
		row_ptr = (png_bytep)ptr;
		png_write_rows(png_ptr, &row_ptr, 1);
		ptr += rowstride;
	}

	png_write_end(png_ptr, info_ptr);

cleanup:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	if (num_keys > 0) {
		for (i = 0; i < num_keys; i++)
			e_free (text_ptr[i].text);
		e_free(text_ptr);
	}

	return success;
}

static bool
png_save(FILE *f, GalPixbuf *pixbuf, euchar **keys, euchar **values)
{
	return real_save_png(pixbuf, keys, values, error, false, f, NULL, NULL);
}

static bool
png_save_to_callback(GalPixbufSaveFunc save_func,
		ePointer           user_data,
		GalPixbuf          *pixbuf, 
		euchar          **keys,
		euchar          **values)
{
	return real_save_png(pixbuf, keys, values, true, NULL, save_func, user_data);
}
#endif
