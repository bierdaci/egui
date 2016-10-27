#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <jerror.h>

#include <egal/pixbuf-io.h>

#define JPEG_PROG_BUF_SIZE 65536

typedef struct {
	struct jpeg_source_mgr pub;
	JOCTET buffer[JPEG_PROG_BUF_SIZE];
	elong  skip_next;
} my_source_mgr;

struct error_handler_data {
	struct jpeg_error_mgr pub;
#ifdef linux
	sigjmp_buf setjmp_buffer;
#endif
};

typedef struct {
	PixbufContext context;

	euchar *lines[4];
	euchar *dptr;
	euint progressive_num;
	euint rowbytes;

	ebool error;
	ebool did_prescan;
	ebool got_header;
	ebool src_initialized;
	ebool in_output;
	struct jpeg_decompress_struct cinfo;
	struct error_handler_data     jerr;
} JpegProgContext;

static void fatal_error_handler(j_common_ptr cinfo)
{
	struct error_handler_data *errmgr;
	char buffer[JMSG_LENGTH_MAX];

	errmgr = (struct error_handler_data *)cinfo->err;

	cinfo->err->format_message(cinfo, buffer);
#ifdef linux
	siglongjmp(errmgr->setjmp_buffer, 1);
#endif
}

static void output_message_handler(j_common_ptr cinfo)
{
}

static void init_source(j_decompress_ptr cinfo)
{
	my_source_mgr *src = (my_source_mgr *)cinfo->src;
	src->skip_next = 0;
}

static void term_source(j_decompress_ptr cinfo)
{
}

static boolean fill_input_buffer(j_decompress_ptr cinfo)
{
	return efalse;
}

static void skip_input_data(j_decompress_ptr cinfo, elong num_bytes)
{
	my_source_mgr *src = (my_source_mgr *)cinfo->src;
	elong   num_can_do;

	if (num_bytes > 0) {
		num_can_do = MIN((elong)src->pub.bytes_in_buffer, num_bytes);
		src->pub.next_input_byte += (size_t)num_can_do;
		src->pub.bytes_in_buffer -= (size_t)num_can_do;
		src->skip_next = num_bytes - num_can_do;
	}
}

static ePointer jpeg_load_begin(void)
{
	JpegProgContext *context;
	my_source_mgr   *src;

	context = e_malloc(sizeof(JpegProgContext));
	context->dptr = NULL;
	context->got_header = efalse;
	context->did_prescan = efalse;
	context->src_initialized = efalse;
	context->in_output = efalse;
	context->error = efalse;
	context->progressive_num = 0;

	jpeg_create_decompress(&context->cinfo);

	context->cinfo.src = (struct jpeg_source_mgr *)e_malloc(sizeof(my_source_mgr));
	if (!context->cinfo.src)
		return NULL;
	memset(context->cinfo.src, 0, sizeof(my_source_mgr));

	src = (my_source_mgr *)context->cinfo.src;

	context->cinfo.err = jpeg_std_error(&context->jerr.pub);
	context->jerr.pub.error_exit = fatal_error_handler;
	context->jerr.pub.output_message = output_message_handler;

	src = (my_source_mgr *)context->cinfo.src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = term_source;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;

	return (ePointer)context;
}

static GalPixbuf *jpeg_load_end(ePointer data)
{
	JpegProgContext *context = (JpegProgContext *)data;
	GalPixbuf *pixbuf = context->context.pixbuf;

	if (!context) return NULL;
#ifdef linux
	if (sigsetjmp(context->jerr.setjmp_buffer, 1))
		context->error = etrue;
	else
#endif
		jpeg_finish_decompress(&context->cinfo);

	jpeg_destroy_decompress(&context->cinfo);

	if (context->dptr)
		e_free(context->dptr);
	if (context->cinfo.src) {
		my_source_mgr *src = (my_source_mgr *)context->cinfo.src;
		e_free(src);
	}

	e_free(context);

	if (context->error && pixbuf) {
		egal_pixbuf_free(pixbuf);
		pixbuf = NULL;
	}
	return pixbuf;
}

#define EXIF_JPEG_MARKER   JPEG_APP0+1

static void explode_gray_into_pixbuf(JpegProgContext  *context, euchar **lines, eint num)
{
	struct jpeg_decompress_struct *cinfo = &context->cinfo;
	eint i, j;
	eint w;
	PixbufContext *con = (PixbufContext *)context;

	if (!cinfo) return;
	if (cinfo->output_components != 1)
		return;
	if (cinfo->out_color_space != JCS_GRAYSCALE)
		return;

	w = cinfo->output_width;
	for (i = 0; i < num; i++) {
		euchar *p = lines[i];

		if (!con->set_y(con, -1))
			continue;

		for (j = 0; j < w; j++) {
			con->set_x(con, -1, p[0], p[0], p[0], 0xff);
			p++;
		}
	}
}

static void output_rgb_into_pixbuf(JpegProgContext *context, euchar **lines, eint num)
{
	struct jpeg_decompress_struct *cinfo = &context->cinfo;
	eint i, j;
	eint w;
	PixbufContext *con = (PixbufContext *)context;

	w = cinfo->output_width;
	for (i = 0; i < num; i++) {
		euchar *p = lines[i];

		if (!con->set_y(con, -1))
			continue;

		for (j = 0; j < w; j++) {
			con->set_x(con, -1, p[2], p[1], p[0], 0xff);
			p += cinfo->output_components;
		}
	}
}

static void convert_cmyk_to_rgb(JpegProgContext *context, euchar **lines, eint num)
{
	struct jpeg_decompress_struct *cinfo = &context->cinfo;
	eint i, j;

	PixbufContext *con = (PixbufContext *)context;

	if (!cinfo) return;
	if (cinfo->output_components != 4)
		return;
	if (cinfo->out_color_space != JCS_CMYK)
		return;

	for (i = 0; i < num; i++) {
		euchar *p = lines[i];

		if (!con->set_y(con, -1))
			continue;

		for (j = 0; j < (eint)cinfo->output_width; j++) {
			eint c, m, y, k;
			c = p[0];
			m = p[1];
			y = p[2];
			k = p[3];
			if (cinfo->saw_Adobe_marker) {
				p[0] = k * c / 255;
				p[1] = k * m / 255;
				p[2] = k * y / 255;
			}
			else {
				p[0] = (255 - k) * (255 - c) / 255;
				p[1] = (255 - k) * (255 - m) / 255;
				p[2] = (255 - k) * (255 - y) / 255;
			}

			con->set_x(con, -1, p[2], p[1], p[0], 0xff);
			p += 4;
		}
	}
}

static ebool jpeg_pixbuf_load_lines(JpegProgContext  *context)
{
	struct jpeg_decompress_struct *cinfo = &context->cinfo;
	eint nlines;

	while (cinfo->output_scanline < cinfo->output_height) {

		nlines = jpeg_read_scanlines(cinfo, context->lines, cinfo->rec_outbuf_height);
		if (nlines == 0)
			break;

		switch (cinfo->out_color_space) {
			case JCS_GRAYSCALE:
				explode_gray_into_pixbuf(context, context->lines, nlines);
				break;
			case JCS_RGB:
				output_rgb_into_pixbuf(context, context->lines, nlines);
				break;
			case JCS_CMYK:
				convert_cmyk_to_rgb(context, context->lines, nlines);
				break;
			default:
				return efalse;
		}
	}

	return etrue;
}

static ebool jpeg_load_increment(ePointer data, const euchar *buf, size_t size)
{
	JpegProgContext *context = (JpegProgContext *)data;
	struct jpeg_decompress_struct *cinfo;
	my_source_mgr *src;
	euint32 num_left, num_copy;
	euint32 last_num_left, last_bytes_left;
	euint32 spinguard;
	ebool first;
	const euchar *bufhd;
	//eint is_otag;
	//char otag_str[5];

	if (!context || !buf) return efalse;

	src = (my_source_mgr *)context->cinfo.src;

	cinfo = &context->cinfo;
#ifdef linux
	if (sigsetjmp(context->jerr.setjmp_buffer, 1)) {
		context->error = etrue;
		return efalse;
	}
#endif
	if (context->src_initialized && src->skip_next) {
		if (src->skip_next > (eint)size) {
			src->skip_next -= size;
			return etrue;
		}
		else {
			num_left = size - src->skip_next;
			bufhd = buf + src->skip_next;
			src->skip_next = 0;
		}
	}
	else {
		num_left = size;
		bufhd = buf;
	}

	if (num_left == 0)
		return etrue;

	last_num_left = num_left;
	last_bytes_left = 0;
	spinguard = 0;
	first = etrue;
	while (etrue) {
		if (num_left > 0) {
			if (src->pub.bytes_in_buffer && src->pub.next_input_byte != src->buffer)
				e_memmove(src->buffer, src->pub.next_input_byte, src->pub.bytes_in_buffer);

			num_copy = MIN(JPEG_PROG_BUF_SIZE - src->pub.bytes_in_buffer, num_left);

			e_memcpy(src->buffer + src->pub.bytes_in_buffer, bufhd, num_copy);
			src->pub.next_input_byte = src->buffer;
			src->pub.bytes_in_buffer += num_copy;
			bufhd += num_copy;
			num_left -= num_copy;
		}

		if (first) {
			last_bytes_left = src->pub.bytes_in_buffer;
			first = efalse;
		}
		else if (src->pub.bytes_in_buffer == last_bytes_left && num_left == last_num_left) {
			spinguard++;
		}
		else {
			last_bytes_left = src->pub.bytes_in_buffer;
			last_num_left = num_left;
		}

		if (spinguard > 2)
			return etrue;

		if (!context->got_header) {
			eint rc, i;
			euchar *rowptr;
			PixbufContext *con = (PixbufContext *)context;
#ifdef linux
			jpeg_save_markers(cinfo, EXIF_JPEG_MARKER, 0xffff);
#endif
			rc = jpeg_read_header(cinfo, etrue);
			context->src_initialized = etrue;

			if (rc == JPEG_SUSPENDED)
				continue;

			context->got_header = etrue;

			//is_otag = get_orientation(cinfo);

			for (cinfo->scale_denom = 2; cinfo->scale_denom <= 8; cinfo->scale_denom *= 2) {
				jpeg_calc_output_dimensions(cinfo);
				if (cinfo->output_width < cinfo->image_width
						|| cinfo->output_height < cinfo->image_height) {
					cinfo->scale_denom /= 2;
					break;
				}
			}
			jpeg_calc_output_dimensions(cinfo);

#ifdef WIN32
			con->init(con, cinfo->output_width, cinfo->output_height, 3, 1);
#else
			con->init(con, cinfo->output_width, cinfo->output_height, 3, 0);
#endif

			//if (is_otag) {
			//	e_snprintf(otag_str, sizeof (otag_str), "%d", is_otag);
			//	gdk_pixbuf_set_option(context->pixbuf, "orientation", otag_str);
			//}

			context->rowbytes = (cinfo->output_width * cinfo->output_components + 7) & ~7;
			context->dptr = e_malloc(context->rowbytes * cinfo->rec_outbuf_height);
			rowptr = context->dptr;
			for (i = 0; i < cinfo->rec_outbuf_height; i++) {
				context->lines[i] = rowptr;
				rowptr += context->rowbytes;
			}
		}
		else if (!context->did_prescan) {
			eint rc;

			cinfo->buffered_image = cinfo->progressive_mode;
			rc = jpeg_start_decompress(cinfo);
			cinfo->do_fancy_upsampling = efalse;
			cinfo->do_block_smoothing = efalse;

			if (rc == JPEG_SUSPENDED)
				continue;

			context->did_prescan = etrue;
		}
		else if (!cinfo->buffered_image) {
			if (!jpeg_pixbuf_load_lines(context)) {
				context->error = etrue;
				return efalse;
			}

			if (cinfo->output_scanline >= cinfo->output_height)
				return etrue;
		}
		else {
			while (!jpeg_input_complete(cinfo)) {
				if (!context->in_output) {
#ifdef linux
					if (jpeg_start_output(cinfo, cinfo->input_scan_number))
						context->in_output = etrue;
					else
#endif
						break;
				}

				if (!jpeg_pixbuf_load_lines(context)) {
					context->error = etrue;
					return efalse;
				}
#ifdef WIN32
				if (cinfo->output_scanline >= cinfo->output_height)
#else
				if (cinfo->output_scanline >= cinfo->output_height && jpeg_finish_output(cinfo))
#endif
					context->in_output = efalse;
				else
					break;
			}
			if (jpeg_input_complete(cinfo))
				return etrue;
			else
				continue;
		}
	}
}

static ebool jpeg_load_header(GalPixbuf *pixbuf, ePointer data, euint len)
{
	struct jpeg_decompress_struct de;
	struct jpeg_error_mgr jerr;

	jpeg_create_decompress(&de);
	de.src = (struct jpeg_source_mgr *)e_malloc(sizeof(my_source_mgr));
	if (!de.src)
		goto error;
	memset(de.src, 0, sizeof(my_source_mgr));

	de.src->next_input_byte = data;
	de.src->bytes_in_buffer = len;
	de.src->init_source = init_source;
	de.src->fill_input_buffer = fill_input_buffer;
	de.src->skip_input_data = skip_input_data;
	de.src->resync_to_restart = jpeg_resync_to_restart;
	de.src->term_source = term_source;

	de.err = jpeg_std_error(&jerr);
	jerr.error_exit = fatal_error_handler;
	jerr.output_message = output_message_handler;
#ifdef linux
	jpeg_save_markers(&de, EXIF_JPEG_MARKER, 0xffff);
#endif
	jpeg_read_header(&de, etrue);

	jpeg_calc_output_dimensions(&de);

	pixbuf->w = de.output_width;
	pixbuf->h = de.output_height;

	e_free(de.src);
error:
	jpeg_destroy_decompress(&de);

	return etrue;
}

void _gal_pixbuf_jpeg_fill_vtable(GalPixbufModule *module)
{
	module->load_begin     = jpeg_load_begin;
	module->load_end       = jpeg_load_end;
	module->load_header    = jpeg_load_header;
	module->load_increment = jpeg_load_increment;
	//	module->save = jpeg_pixbuf_save;
	//	module->save_to_callback = jpeg_save_to_callback;
}

#if 0

const char leth[]  = {0x49, 0x49, 0x2a, 0x00};	// Little endian TIFF header
const char beth[]  = {0x4d, 0x4d, 0x00, 0x2a};	// Big endian TIFF header
const char types[] = {0x00, 0x01, 0x01, 0x02, 0x04, 0x08, 0x00, 
	0x08, 0x00, 0x04, 0x08}; 	// size in bytes for EXIF types

#define DE_ENDIAN16(val) endian == G_BIG_ENDIAN ? GUINT16_FROM_BE(val) : GUINT16_FROM_LE(val)
#define DE_ENDIAN32(val) endian == G_BIG_ENDIAN ? GUINT32_FROM_BE(val) : GUINT32_FROM_LE(val)

#define ENDIAN16_IT(val) endian == G_BIG_ENDIAN ? GUINT16_TO_BE(val) : GUINT16_TO_LE(val)
#define ENDIAN32_IT(val) endian == G_BIG_ENDIAN ? GUINT32_TO_BE(val) : GUINT32_TO_LE(val)

#define EXIF_JPEG_MARKER   JPEG_APP0+1
#define EXIF_IDENT_STRING  "Exif\000\000"

static unsigned short de_get16(ePoinetr ptr, guint endian)
{
	unsigned short val;

	memcpy(&val, ptr, sizeof(val));
	val = DE_ENDIAN16(val);

	return val;
}

static unsigned eint de_get32(ePoinetr ptr, guint endian)
{
	unsigned eint val;

	memcpy(&val, ptr, sizeof(val));
	val = DE_ENDIAN32(val);

	return val;
}

static eint get_orientation(j_decompress_ptr cinfo)
{
	/* This function looks through the meta data in the libjpeg decompress structure to
	   determine if an EXIF Orientation tag is present and if so return its value (1-8). 
	   If no EXIF Orientation tag is found 0 (zero) is returned. */

	guint   i;              /* index into working buffer */
	guint   orient_tag_id;  /* endianed version of orientation tag ID */
	guint   ret;            /* Return value */
	guint   offset;        	/* de-endianed offset in various situations */
	guint   tags;           /* number of tags in current ifd */
	guint   type;           /* de-endianed type of tag used as index into types[] */
	guint   count;          /* de-endianed count of elements in a tag */
	guint   tiff = 0;   	/* offset to active tiff header */
	guint   endian = 0;   	/* detected endian of data */

	jpeg_saved_marker_ptr exif_marker;  /* Location of the Exif APP1 marker */
	jpeg_saved_marker_ptr cmarker;	    /* Location to check for Exif APP1 marker */

	/* check for Exif marker (also called the APP1 marker) */
	exif_marker = NULL;
	cmarker = cinfo->marker_list;
	while (cmarker) {
		if (cmarker->marker == EXIF_JPEG_MARKER) {
			/* The Exif APP1 marker should contain a unique
			   identification string ("Exif\0\0"). Check for it. */
			if (!memcmp(cmarker->data, EXIF_IDENT_STRING, 6))
				exif_marker = cmarker;
		}
		cmarker = cmarker->next;
	}

	/* Did we find the Exif APP1 marker? */
	if (exif_marker == NULL)
		return 0;

	/* Do we have enough data? */
	if (exif_marker->data_length < 32)
		return 0;

	/* Check for TIFF header and catch endianess */
	i = 0;

	/* Just skip data until TIFF header - it should be within 16 bytes from marker start.
	   Normal structure relative to APP1 marker -
0x0000: APP1 marker entry = 2 bytes
0x0002: APP1 length entry = 2 bytes
0x0004: Exif Identifier entry = 6 bytes
0x000A: Start of TIFF header (Byte order entry) - 4 bytes  
- This is what we look for, to determine endianess.
0x000E: 0th IFD offset pointer - 4 bytes

exif_marker->data points to the first data after the APP1 marker
and length entries, which is the exif identification string.
The TIFF header should thus normally be found at i=6, below,
and the pointer to IFD0 will be at 6+4 = 10.
*/

	while (i < 16) {

		/* Little endian TIFF header */
		if (memcmp(&exif_marker->data[i], leth, 4) == 0)
			endian = G_LITTLE_ENDIAN;

		/* Big endian TIFF header */
		else if (memcmp (&exif_marker->data[i], beth, 4) == 0)
			endian = G_BIG_ENDIAN;
		/* Keep looking through buffer */
		else {
			i++;
			continue;
		}
		/* We have found either big or little endian TIFF header */
		tiff = i;
		break;
	}

	/* So did we find a TIFF header or did we just hit end of buffer? */
	if (tiff == 0) 
		return 0;

	/* Endian the orientation tag ID, to locate it more easily */
	orient_tag_id = ENDIAN16_IT(0x112);

	/* Read out the offset pointer to IFD0 */
	offset  = de_get32(&exif_marker->data[i] + 4, endian);
	i       = i + offset;

	/* Check that we still are within the buffer and can read the tag count */
	if ((i + 2) > exif_marker->data_length)
		return 0;

	/* Find out how many tags we have in IFD0. As per the TIFF spec, the first
	   two bytes of the IFD contain a count of the number of tags. */
	tags    = de_get16(&exif_marker->data[i], endian);
	i       = i + 2;

	/* Check that we still have enough data for all tags to check. The tags
	   are listed in consecutive 12-byte blocks. The tag ID, type, size, and
	   a pointer to the actual value, are packed into these 12 byte entries. */
	if ((i + tags * 12) > exif_marker->data_length)
		return 0;

	/* Check through IFD0 for tags of interest */
	while (tags--){
		type   = de_get16(&exif_marker->data[i + 2], endian);
		count  = de_get32(&exif_marker->data[i + 4], endian);

		/* Is this the orientation tag? */
		if (memcmp (&exif_marker->data[i], (char *) &orient_tag_id, 2) == 0){ 

			/* Check that type and count fields are OK. The orientation field 
			   will consist of a single (count=1) 2-byte integer (type=3). */
			if (type != 3 || count != 1) return 0;

			/* Return the orientation value. Within the 12-byte block, the
			   pointer to the actual data is at offset 8. */
			ret =  de_get16(&exif_marker->data[i + 8], endian);
			return ret <= 8 ? ret : 0;
		}
		/* move the pointer to the next 12-byte tag field. */
		i = i + 12;
	}

	return 0; /* No EXIF Orientation tag found */
}

#define TO_FUNCTION_BUF_SIZE 4096

typedef struct {
	struct jpeg_destination_mgr pub;
	JOCTET             *buffer;
	GalPixbufSaveFunc   save_func;
	ePointer            user_data;
	GError            **error;
} ToFunctionDestinationManager;

void to_callback_init(j_compress_ptr cinfo)
{
	ToFunctionDestinationManager *destmgr;

	destmgr	= (ToFunctionDestinationManager*) cinfo->dest;
	destmgr->pub.next_output_byte = destmgr->buffer;
	destmgr->pub.free_in_buffer = TO_FUNCTION_BUF_SIZE;
}

static void to_callback_do_write(j_compress_ptr cinfo, gsize length)
{
	ToFunctionDestinationManager *destmgr;

	destmgr	= (ToFunctionDestinationManager*) cinfo->dest;
	if (!destmgr->save_func (destmgr->buffer,
				length,
				destmgr->error,
				destmgr->user_data)) {
		struct error_handler_data *errmgr;

		errmgr = (struct error_handler_data *) cinfo->err;
		/* Use a default error message if the callback didn't set one,
		 * which it should have.
		 */
		if (errmgr->error && *errmgr->error == NULL) {
			g_set_error (errmgr->error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_CORRUPT_IMAGE,
					"write function failed");
		}
#ifdef linux
		siglongjmp(errmgr->setjmp_buffer, 1);
#endif
		//g_assert_not_reached ();
	}
}

static ebool to_callback_empty_output_buffer(j_compress_ptr cinfo)
{
	ToFunctionDestinationManager *destmgr;

	destmgr	= (ToFunctionDestinationManager*) cinfo->dest;
	to_callback_do_write (cinfo, TO_FUNCTION_BUF_SIZE);
	destmgr->pub.next_output_byte = destmgr->buffer;
	destmgr->pub.free_in_buffer = TO_FUNCTION_BUF_SIZE;
	return etrue;
}

void to_callback_terminate(j_compress_ptr cinfo)
{
	ToFunctionDestinationManager *destmgr;

	destmgr	= (ToFunctionDestinationManager*) cinfo->dest;
	to_callback_do_write (cinfo, TO_FUNCTION_BUF_SIZE - destmgr->pub.free_in_buffer);
}

static ebool real_save_jpeg(GalPixbuf          *pixbuf,
		echar             **keys,
		echar             **values,
		GError            **error,
		ebool            to_callback,
		FILE               *f,
		GalPixbufSaveFunc   save_func,
		ePointer            user_data)
{
	/* FIXME error handling is broken */

	struct jpeg_compress_struct cinfo;
	euchar *buf = NULL;
	euchar *ptr;
	euchar *pixels = NULL;
	JSAMPROW *jbuf;
	eint y = 0;
	volatile eint quality = 75; /* default; must be between 0 and 100 */
	eint i, j;
	eint w, h = 0;
	eint rowstride = 0;
	eint n_channels;
	struct error_handler_data jerr;
	ToFunctionDestinationManager to_callback_destmgr;

	to_callback_destmgr.buffer = NULL;

	if (keys && *keys) {
		echar **kiter = keys;
		echar **viter = values;

		while (*kiter) {
			if (strcmp (*kiter, "quality") == 0) {
				char *endptr = NULL;
				quality = strtol (*viter, &endptr, 10);

				if (endptr == *viter) {
					g_set_error (error,
							GDK_PIXBUF_ERROR,
							GDK_PIXBUF_ERROR_BAD_OPTION,
							_("JPEG quality must be a value between 0 and 100; value '%s' could not be parsed."),
							*viter);

					return efalse;
				}

				if (quality < 0 ||
						quality > 100) {
					/* This is a user-visible error;
					 * lets people skip the range-checking
					 * in their app.
					 */
					g_set_error (error,
							GDK_PIXBUF_ERROR,
							GDK_PIXBUF_ERROR_BAD_OPTION,
							_("JPEG quality must be a value between 0 and 100; value '%d' is not allowed."),
							quality);

					return efalse;
				}
			} else {
				g_warning ("Unrecognized parameter (%s) passed to JPEG saver.", *kiter);
			}

			++kiter;
			++viter;
		}
	}

	rowstride = gdk_pixbuf_get_rowstride (pixbuf);
	n_channels = gdk_pixbuf_get_n_channels (pixbuf);

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	pixels = gdk_pixbuf_get_pixels (pixbuf);

	/* Allocate a small buffer to convert pixbuf data,
	 * and a larger buffer if doing to_callback save.
	 */
	buf = g_try_malloc (w * 3 * sizeof (euchar));
	if (!buf) {
		g_set_error (error,
				GDK_PIXBUF_ERROR,
				GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
				_("Couldn't allocate memory for loading JPEG file"));
		return efalse;
	}
	if (to_callback) {
		to_callback_destmgr.buffer = g_try_malloc (TO_FUNCTION_BUF_SIZE);
		if (!to_callback_destmgr.buffer) {
			g_set_error (error,
					GDK_PIXBUF_ERROR,
					GDK_PIXBUF_ERROR_INSUFFICIENT_MEMORY,
					_("Couldn't allocate memory for loading JPEG file"));
			return efalse;
		}
	}

	/* set up error handling */
	jerr.pub.error_exit = fatal_error_handler;
	jerr.pub.output_message = output_message_handler;
	jerr.error = error;

	cinfo.err = jpeg_std_error(&(jerr.pub));
#ifdef linux
	if (sigsetjmp(jerr.setjmp_buffer, 1)) {
		jpeg_destroy_compress(&cinfo);
		e_free(buf);
		e_free(to_callback_destmgr.buffer);
		return efalse;
	}
#endif
	/* setup compress params */
	jpeg_create_compress(&cinfo);
	if (to_callback) {
		to_callback_destmgr.pub.init_destination    = to_callback_init;
		to_callback_destmgr.pub.empty_output_buffer = to_callback_empty_output_buffer;
		to_callback_destmgr.pub.term_destination    = to_callback_terminate;
		to_callback_destmgr.error = error;
		to_callback_destmgr.save_func = save_func;
		to_callback_destmgr.user_data = user_data;
		cinfo.dest = (struct jpeg_destination_mgr*) &to_callback_destmgr;
	}
	else
		jpeg_stdio_dest(&cinfo, f);

	cinfo.image_width      = w;
	cinfo.image_height     = h;
	cinfo.input_components = 3; 
	cinfo.in_color_space   = JCS_RGB;

	/* set up jepg compression parameters */
	jpeg_set_defaults (&cinfo);
	jpeg_set_quality (&cinfo, quality, etrue);
	jpeg_start_compress (&cinfo, etrue);
	/* get the start pointer */
	ptr = pixels;
	/* go one scanline at a time... and save */
	i = 0;
	while (cinfo.next_scanline < cinfo.image_height) {
		/* convert scanline from ARGB to RGB packed */
		for (j = 0; j < w; j++)
			memcpy (&(buf[j*3]), &(ptr[i*rowstride + j*n_channels]), 3);

		/* write scanline */
		jbuf = (JSAMPROW *)(&buf);
		jpeg_write_scanlines (&cinfo, jbuf, 1);
		i++;
		y++;
	}

	/* finish off */
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	g_free(buf);
	g_free(to_callback_destmgr.buffer);
	return etrue;
}

static ebool jpeg_pixbuf_save(FILE *f, 
		GalPixbuf     *pixbuf, 
		echar        **keys,
		echar        **values)
{
	return real_save_jpeg(pixbuf, keys, values, efalse, f, NULL, NULL);
}

static ebool jpeg_save_to_callback(GalPixbufSaveFunc   save_func,
		ePointer            user_data,
		GalPixbuf          *pixbuf, 
		echar			**keys,
		echar			**values)
{
	return real_save_jpeg(pixbuf, keys, values, etrue, NULL, save_func, user_data);
}

#endif
