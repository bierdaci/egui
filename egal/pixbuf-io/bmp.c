#include <stdio.h>
#include <string.h>

#ifdef linux
#include <unistd.h>
#endif

#include <egal/pixbuf-io.h>

#ifndef BI_RGB
#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3
#endif

typedef enum {
	READ_STATE_HEADERS,
	READ_STATE_PALETTE,
	READ_STATE_BITMASKS,
	READ_STATE_DATA,
	READ_STATE_ERROR,
	READ_STATE_PADDING,
	READ_STATE_DONE,
} ReadState;

struct headerpair {
	euint32 size;
	eint32 width;
	eint32 height;
	eint depth;
	euint negative;
	euint n_colors;
};

struct bmp_compression_state {
	eint phase;
	eint run;
	eint count;
	eint x, y, j, j2;
};

struct bmp_context_state {
	PixbufContext context;

	void *user_data;
	euchar *in_buffer;

	ReadState read_state;

	euint lines_width;
	euint lines;

	euint32 request_size;
	euint32 old_size;
	euint32 done_size;
	euint buff_padding;

	euchar (*colormap)[3];

	eint type;
	euint compressed;
	struct bmp_compression_state compr;

	struct headerpair header;

	euint32 r_mask, r_shift, r_bits;
	euint32 g_mask, g_shift, g_bits;
	euint32 b_mask, b_shift, b_bits;
	euint32 a_mask, a_shift, a_bits;

};

static ePointer bmp_load_begin(void);

static GalPixbuf* bmp_load_end(ePointer data);
static bool bmp_increment_load(ePointer data, const euchar *buf, size_t);

static euint lsb_32(const euchar *src)
{
	return src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
}

static short lsb_16(const euchar *src)
{
	return src[0] | (src[1] << 8);
}

static eint decode_bitmasks(const euchar *buf, struct bmp_context_state *state);

static eint decode_header(const euchar *BFH, const euchar *BIH, struct bmp_context_state *context)
{
	struct headerpair *header = &context->header;
	eint clrused;

	if (*BFH != 0x42 || *(BFH + 1) != 0x4D)
		return -1;

	if (context->request_size < lsb_32(&BIH[0]) + 14) {
		context->request_size = lsb_32(&BIH[0]) + 14;
		return 1;
	}

	header->size = lsb_32(&BIH[0]);

	if (header->size == 124
			|| header->size == 108
			|| header->size == 64
			|| header->size == 56
			|| header->size == 40) {
		header->width  = lsb_32(&BIH[4]);
		header->height = lsb_32(&BIH[8]);
		header->depth  = lsb_16(&BIH[14]);
		context->compressed = lsb_32(&BIH[16]);
	}
	else if (header->size == 12) {
		header->width  = lsb_16(&BIH[4]);
		header->height = lsb_16(&BIH[6]);
		header->depth  = lsb_16(&BIH[10]);
		context->compressed = BI_RGB;
	}
	else return -1;

	if (header->size == 12)
		clrused = 1 << header->depth;
	else
		clrused = (eint)(BIH[35] << 24) + (BIH[34] << 16) + (BIH[33] << 8) + (BIH[32]);

	if (clrused != 0)
		header->n_colors = clrused;
	else
		header->n_colors = (1 << header->depth);

	if ((eint)header->n_colors > (1 << header->depth))
		return -1;

	context->type = header->depth;

	if (header->height < 0) {
		header->height = -header->height;
		header->negative = 1;
	}
	else
		header->negative = 0;

	if (header->negative
			&& context->compressed != BI_RGB 
			&& context->compressed != BI_BITFIELDS)
		return -1;

	if (header->width <= 0 || header->height == 0
			|| (context->compressed == BI_RLE4 && context->type != 4)
			|| (context->compressed == BI_RLE8 && context->type != 8)
			|| (context->compressed == BI_BITFIELDS && !(context->type == 16 || context->type == 32))
			|| (context->compressed > BI_BITFIELDS))
		return -1;

	if (context->type == 32)
		context->lines_width = header->width * 4;
	else if (context->type == 24)
		context->lines_width = header->width * 3;
	else if (context->type == 16)
		context->lines_width = header->width * 2;
	else if (context->type == 8)
		context->lines_width = header->width * 1;
	else if (context->type == 4)
		context->lines_width = (header->width + 1) / 2;
	else if (context->type == 1) {
		context->lines_width = header->width / 8;
		if ((header->width & 7) != 0)
			context->lines_width++;
	}
	else
		return -1;

	if (((context->lines_width % 4) > 0)
			&& (context->compressed == BI_RGB || context->compressed == BI_BITFIELDS))
		context->lines_width = (context->lines_width / 4) * 4 + 4;

	if (context->type <= 8) {
		eint samples;
		context->read_state = READ_STATE_PALETTE;
		samples = (header->size == 12 ? 3 : 4);
		context->request_size = header->n_colors * samples;
		context->buff_padding = (lsb_32(&BFH[10]) - 14 - header->size) - context->request_size;
	}
	else if (context->compressed == BI_RGB) {
		if (context->request_size < lsb_32(&BFH[10])) {
			context->read_state = READ_STATE_HEADERS;
			context->request_size = lsb_32(&BFH[10]);
		}
		else {
			context->read_state = READ_STATE_DATA;
			context->request_size = context->lines_width;
		}
	}
	else if (context->compressed == BI_BITFIELDS) {
		if (header->size == 56 || header->size == 108 || header->size == 124) {
			if (decode_bitmasks(&BIH[40], context) < 0)
				return -1;
		}
		else {
			context->read_state = READ_STATE_BITMASKS;
			context->request_size = 12;
		}
	}
	else return -1;

	return 0;
}

static bool bmp_load_header(GalPixbuf *pixbuf, ePointer data, euint size)
{
	struct bmp_context_state context;

	context.request_size = size;
	if (decode_header(data, (char *)data + 14, &context) != 0)
		return false;
	pixbuf->w = context.header.width;
	pixbuf->h = context.header.height;
	return true;
}

static eint decode_colormap(const euchar *buff, struct bmp_context_state *context)
{
	euint i;
	eint samples;

	if (context->read_state != READ_STATE_PALETTE)
		return -1;

	samples = (context->header.size == 12 ? 3 : 4);
	if (context->request_size < context->header.n_colors * samples) {
		context->request_size = context->header.n_colors * samples;
		return 1;
	}

	context->colormap = e_malloc((1 << context->header.depth) * sizeof(*context->colormap));
	for (i = 0; i < context->header.n_colors; i++) {
		context->colormap[i][0] = buff[i * samples];
		context->colormap[i][1] = buff[i * samples + 1];
		context->colormap[i][2] = buff[i * samples + 2];
	}

	if (!(context->compressed == BI_RGB || context->compressed == BI_BITFIELDS))
		context->request_size = 1;
	else
		context->request_size = context->lines_width;

	if (context->buff_padding > 0) {
		context->read_state = READ_STATE_PADDING;
		if (context->lines_width < context->buff_padding)
			context->request_size = context->buff_padding;
	}
	else {
		context->read_state = READ_STATE_DATA;
	}
	
	return 0;
}

static eint decode_padding(const euchar *buff, struct bmp_context_state *context)
{
	context->read_state = READ_STATE_DATA;
	return 0;
}

static void find_bits(euint32 n, euint32 *lowest, euint32 *n_set)
{
	eint i;
	*n_set = 0;
	for (i = 31; i >= 0; i--) {
		if (n & (1 << i)) {
			*lowest = i;
			(*n_set)++;
		}
	}
}

static eint decode_bitmasks(const euchar *buf, struct bmp_context_state *context)
{
	context->a_mask = context->a_shift = context->a_bits = 0;
	context->b_mask = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	buf += 4;

	context->g_mask = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	buf += 4;

	context->r_mask = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);

	find_bits(context->r_mask, &context->r_shift, &context->r_bits);
	find_bits(context->g_mask, &context->g_shift, &context->g_bits);
	find_bits(context->b_mask, &context->b_shift, &context->b_bits);

	if (context->header.size == 108 || context->header.size == 124) {
		buf += 4;
		context->a_mask = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
		find_bits(context->a_mask, &context->a_shift, &context->a_bits);
	}

	if (context->r_bits == 0 || context->g_bits == 0 || context->b_bits == 0) {
		if (context->type == 16) {
			context->r_mask = 0x7c00;
			context->r_shift = 10;
			context->g_mask = 0x03e0;
			context->g_shift = 5;
			context->b_mask = 0x001f;
			context->b_shift = 0;

			context->r_bits = context->g_bits = context->b_bits = 5;
		}
		else {
			context->r_mask = 0x00ff0000;
			context->r_shift = 16;
			context->g_mask = 0x0000ff00;
			context->g_shift = 8;
			context->b_mask = 0x000000ff;
			context->b_shift = 0;
			context->a_mask = 0xff000000;
			context->a_shift = 24;

			context->r_bits = context->g_bits = context->b_bits = context->a_bits = 8;
		}
	}

	if (context->r_bits > 8) {
		context->r_shift += context->r_bits - 8;
		context->r_bits = 8;
	}
	if (context->g_bits > 8) {
		context->g_shift += context->g_bits - 8;
		context->g_bits = 8;
	}
	if (context->b_bits > 8) {
		context->b_shift += context->b_bits - 8;
		context->b_bits = 8;
	}
	if (context->a_bits > 8) {
		context->a_shift += context->a_bits - 8;
		context->a_bits = 8;
	}

	context->read_state = READ_STATE_DATA;
	context->request_size = context->lines_width;

	return 0;
}

static ePointer bmp_load_begin(void)
{
	struct bmp_context_state *context;
	
	context = e_malloc(sizeof(struct bmp_context_state));

	context->in_buffer = NULL;

	context->request_size = 26;
	context->old_size = 0;
	context->done_size = 0;
	context->buff_padding = 0;

	context->colormap = NULL;

	context->lines = 0;
	context->type = 0;
	context->read_state = READ_STATE_HEADERS;

	e_memset(&context->header, 0, sizeof(struct headerpair));
	e_memset(&context->compr, 0, sizeof(struct bmp_compression_state));
	
	return (void *)context;
}

static GalPixbuf* bmp_load_end(void *data)
{
	struct bmp_context_state *context = (struct bmp_context_state *)data;
	GalPixbuf *pixbuf = context->context.pixbuf;

	if (!context) return NULL;

	if (context->in_buffer)
		e_free(context->in_buffer);

	if (context->colormap)
		e_free(context->colormap);

	if ((context->read_state != READ_STATE_DONE) && pixbuf) {
		egal_pixbuf_free(pixbuf);
		pixbuf = NULL;
	}

	e_free(context);
	return pixbuf;
}

static void one_line32(const euchar *src, struct bmp_context_state *context)
{
	eint x;

	PixbufContext *con = (PixbufContext *)context;
	if (context->compressed == BI_BITFIELDS) {
		eint r_lshift, r_rshift;
		eint g_lshift, g_rshift;
		eint b_lshift, b_rshift;
		eint a_lshift, a_rshift;
		euint8 v, r, g, b, a;

		r_lshift = 8 - context->r_bits;
		g_lshift = 8 - context->g_bits;
		b_lshift = 8 - context->b_bits;
		a_lshift = 8 - context->a_bits;

		r_rshift = context->r_bits - r_lshift;
		g_rshift = context->g_bits - g_lshift;
		b_rshift = context->b_bits - b_lshift;
		a_rshift = context->a_bits - a_lshift;

		for (x = 0; x < context->header.width; x++, src += 4) {
			v = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
			r = (v & context->r_mask) >> context->r_shift;
			g = (v & context->g_mask) >> context->g_shift;
			b = (v & context->b_mask) >> context->b_shift;
			a = (v & context->a_mask) >> context->a_shift;

			r = (r << r_lshift) | (r >> r_rshift);
			g = (g << g_lshift) | (g >> g_rshift);
			b = (b << b_lshift) | (b >> b_rshift);
			if (context->a_bits)
				a = 0xff - ((a << a_lshift) | (a >> a_rshift));
			else
				a = 0xff;
			con->set_x(con, -1, r, g, b, a);
		}
	}
	else {
		euint8 r, g, b;
		for (x = 0; x < context->header.width; x++, src += 4) {
			r = src[0];
			g = src[1];
			b = src[2];
			con->set_x(con, -1, r, g, b, 0xff);
		}
	}

	return;
}

static void one_line24(const euchar *buf, struct bmp_context_state *context)
{
	eint x;

	PixbufContext *con = (PixbufContext *)context;
	for (x = 0; x < context->header.width; x++) {
		euint8 r = buf[x * 3 + 0];
		euint8 g = buf[x * 3 + 1];
		euint8 b = buf[x * 3 + 2];
		con->set_x(con, -1, r, g, b, 0xff);
	}

	return;
}

static void one_line16(const euchar *src, struct bmp_context_state *context)
{
	eint x;

	PixbufContext *con = (PixbufContext *)context;
	if (context->compressed == BI_BITFIELDS) {
		eint r_lshift, r_rshift;
		eint g_lshift, g_rshift;
		eint b_lshift, b_rshift;

		r_lshift = 8 - context->r_bits;
		g_lshift = 8 - context->g_bits;
		b_lshift = 8 - context->b_bits;

		r_rshift = context->r_bits - r_lshift;
		g_rshift = context->g_bits - g_lshift;
		b_rshift = context->b_bits - b_lshift;

		for (x = 0; x < context->header.width; x++) {
			euint8 v, r, g, b;

			v = (eint)src[0] | ((eint)src[1] << 8);

			r = (v & context->r_mask) >> context->r_shift;
			g = (v & context->g_mask) >> context->g_shift;
			b = (v & context->b_mask) >> context->b_shift;

			r = (r << r_lshift) | (r >> r_rshift);
			g = (g << g_lshift) | (g >> g_rshift);
			b = (b << b_lshift) | (b >> b_rshift);

			con->set_x(con, -1, r, g, b, 0xff);
			src += 2;
		}
	}
	else {
		for (x = 0; x < context->header.width; x++) {
			euint8 v, r, g, b;

			v = src[0] | (src[1] << 8);

			r = (v >> 10) & 0x1f;
			g = (v >> 5) & 0x1f;
			b = v & 0x1f;

			r = (r << 3) | (r >> 2);
			g = (g << 3) | (g >> 2);
			b = (b << 3) | (b >> 2);

			con->set_x(con, -1, r, g, b, 0xff);
			src += 2;
		}
	}
}

static void one_line8(const euchar *buf, struct bmp_context_state *context)
{
	PixbufContext *con = (PixbufContext *)context;
	eint x;

	for (x = 0; x < context->header.width; x++) {
		euint8 r = context->colormap[buf[x]][0];
		euint8 g = context->colormap[buf[x]][1];
		euint8 b = context->colormap[buf[x]][2];
		con->set_x(con, -1, r, g, b, 0xff);
	}
	return;
}

static void one_line4(const euchar *buf, struct bmp_context_state *context)
{
	PixbufContext *con = (PixbufContext *)context;
	eint x = 0;

	while (x < context->header.width) {
		euint8 r, g, b;
		euchar pix;

		pix = buf[x / 2];

		r = context->colormap[pix >> 4][0];
		g = context->colormap[pix >> 4][1];
		b = context->colormap[pix >> 4][2];
		con->set_x(con, -1, r, g, b, 0xff);
		x++;
		if (x < context->header.width) {
			r = context->colormap[pix & 15][0];
			g = context->colormap[pix & 15][1];
			b = context->colormap[pix & 15][2];
			con->set_x(con, -1, r, g, b, 0xff);
			x++;
		}
	}
}

static void one_line1(const euchar *buf, struct bmp_context_state *context)
{
	PixbufContext *con = (PixbufContext *)context;
	eint x = 0;

	while (x < context->header.width) {
		euint8 r, g, b;
		eint bit;

		bit = (buf[x / 8]) >> (7 - (x & 7));
		bit = bit & 1;
		r = context->colormap[bit][1];
		g = context->colormap[bit][1];
		b = context->colormap[bit][2];
		con->set_x(con, -1, r, g, b, 0xff);
		x++;
	}
}

#define NEUTRAL       0
#define ENCODED       1
#define ESCAPE        2   
#define DELTA_X       3
#define DELTA_Y       4
#define ABSOLUT       5
#define SKIP          6

#define END_OF_LINE   0
#define END_OF_BITMAP 1
#define DELTA         2

static void do_compressed(const euchar *buf, struct bmp_context_state *context)
{
	PixbufContext *con = (PixbufContext *)context;

	eint idx, i, j, n;
	euint8 r, g, b;
	euchar c;

	if (context->compr.y >= context->header.height) {
		context->read_state = READ_STATE_DONE;
		return;
	}

	con->set_y(con, context->compr.y);
	n = context->request_size;
	for (i = 0; i < n; i++) {
		c = buf[i];
		switch (context->compr.phase) {
			case NEUTRAL:
				if (c) {
					context->compr.run = c;
					context->compr.phase = ENCODED;
				}
				else {
					context->compr.phase = ESCAPE;
				}
				context->request_size = 1;
				break;
			case ENCODED:
				con->dx = context->compr.x * con->x_step;
				for (j = 0; j < context->compr.run; j++) {
					if (context->compressed == BI_RLE8)
						idx = c;
					else if (j & 1)
						idx = c & 0x0f;
					else 
						idx = (c >> 4) & 0x0f;
					if (context->compr.x < context->header.width) {
						r = context->colormap[idx][0];
						g = context->colormap[idx][1];
						b = context->colormap[idx][2];
						con->set_x(con, -1, r, g, b, 0xff);
						context->compr.x++;
					}
				}
				context->compr.phase = NEUTRAL;
				context->request_size = 1;
				break;
			case ESCAPE:
				switch (c) {
				case END_OF_LINE:
					context->compr.x = 0;
					context->compr.y++;
					con->set_y(con, context->compr.y);
					context->compr.phase = NEUTRAL;
					context->request_size = 1;
					break;
				case END_OF_BITMAP:
					context->compr.x = 0;
					context->compr.y = context->header.height;
					con->set_y(con, context->compr.y);
					context->compr.phase = NEUTRAL;
					context->read_state = READ_STATE_DONE;
					context->request_size = 1;
					break;
				case DELTA:
					context->compr.phase = DELTA_X;
					context->request_size = 1;
					break;
				default:
					context->compr.run = c;
					context->compr.count = 0;
					context->compr.phase = ABSOLUT;
					context->request_size = c;
					break;
				}
				break;
			case DELTA_X:
				context->compr.x += c;
				context->compr.phase = DELTA_Y;
				context->request_size = 1;
				break;
			case DELTA_Y:
				context->compr.y += c;
				con->set_y(con, context->compr.y);
				context->compr.phase = NEUTRAL;
				context->request_size = 1;
				break;
			case ABSOLUT:
				if (context->compressed == BI_RLE8) {
					idx = c;

					con->dx = context->compr.x * con->x_step;
					if (context->compr.x < context->header.width) {
						r = context->colormap[idx][0];
						g = context->colormap[idx][1];
						b = context->colormap[idx][2];
						con->set_x(con, -1, r, g, b, 0xff);
						context->compr.x++;
					}
					context->compr.count++;

					if (context->compr.count == context->compr.run) {
						if (context->compr.run & 1)
							context->compr.phase = SKIP;
						else
							context->compr.phase = NEUTRAL;
						context->request_size = 1;
					}
				}
				else {
					con->dx = context->compr.x * con->x_step;
					for (j = 0; j < 2; j++) {
						if (context->compr.count & 1)
							idx = c & 0x0f;
						else 
							idx = (c >> 4) & 0x0f;
						if (context->compr.x < context->header.width) {
							r = context->colormap[idx][0];
							g = context->colormap[idx][1];
							b = context->colormap[idx][2];
							con->set_x(con, -1, r, g, b, 0xff);
							context->compr.x++;
						}
						context->compr.count++;

						if (context->compr.count == context->compr.run) {
							if ((context->compr.run & 3) == 1 || (context->compr.run & 3) == 2)
								context->compr.phase = SKIP;
							else
								context->compr.phase = NEUTRAL;
							context->request_size = 1;
						}
					}
				}
				break;
			case SKIP:
				context->compr.phase = NEUTRAL;
				context->request_size = 1;
				break;
		}
	}

	return;
}

static void one_line(const euchar *buf, struct bmp_context_state *context)
{
	PixbufContext *con = (PixbufContext *)context;
	
	if (con->set_y(con, -1)) {
		if (context->type == 32)
			one_line32(buf, context);
		else if (context->type == 24)
			one_line24(buf, context);
		else if (context->type == 16)
			one_line16(buf, context);
		else if (context->type == 8)
			one_line8(buf, context);
		else if (context->type == 4)
			one_line4(buf, context);
		else if (context->type == 1)
			one_line1(buf, context);
		else
			exit(1);
	}

	context->lines++;

	if ((eint)context->lines == context->header.height)
		context->read_state = READ_STATE_DONE;
}

static eint _bmp_increment_load(struct bmp_context_state *context, const euchar *buf)
{
	eint status = 0;
	PixbufContext *con = (PixbufContext *)context;

	switch (context->read_state) {
		case READ_STATE_HEADERS:
			if ((status = decode_header(buf, buf + 14, context)) < 0)
				return -1;
			if (status == 0) {
				con->init(con,
						context->header.width,
						context->header.height,
						context->header.depth / 8,
						context->header.negative);
			}
			break;

		case READ_STATE_PALETTE:
			if ((status = decode_colormap(buf, context)) < 0)
				return -1;
			break;

		case READ_STATE_BITMASKS:
			if ((status = decode_bitmasks(buf, context)) < 0)
				return -1;
			break;

		case READ_STATE_DATA:
			if (context->compressed == BI_RLE4 || context->compressed == BI_RLE8)
				do_compressed(buf, context);
			else
				one_line(buf, context);
			break;

		case READ_STATE_PADDING:
			if ((status = decode_padding(buf, context)) < 0)
				return -1;
			break;

		case READ_STATE_DONE:
			break;

		case READ_STATE_ERROR:
		default:
			return -1;
	}

	return status;
}

static bool bmp_increment_load(ePointer data, const euchar *buf, size_t len)
{
	struct bmp_context_state *context = (struct bmp_context_state *)data;
	const euchar *p;
	euint32 bytes;
	euint32 status;

	p = buf;
	while (len > 0) {
		if (context->old_size < context->request_size) {
			context->in_buffer = e_realloc(context->in_buffer, context->request_size);
			if (context->in_buffer == NULL)
				return false;
			context->old_size = context->request_size;
		}

		if (context->done_size < context->request_size) {
			bytes = context->request_size - context->done_size;
			if (bytes > len)
				bytes = len;

			e_memcpy(context->in_buffer + context->done_size, p, bytes);
			len -= bytes;
			p += bytes;
			context->done_size += bytes;
			if (context->done_size < context->request_size)
				return true;
		}

		status = _bmp_increment_load(context, context->in_buffer);
		if (status == 0)
			context->done_size = 0;
		else if (status < 0)
			return false;
	}

	return true;
}

void _gal_pixbuf_bmp_fill_vtable(GalPixbufModule *module)
{
	module->load_begin     = bmp_load_begin;
	module->load_end       = bmp_load_end;
	module->load_header    = bmp_load_header;
	module->load_increment = bmp_increment_load;
}

