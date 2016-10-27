#include <stdio.h>

#include <egal/pixbuf-io.h>

#define MAXCOLORMAPSIZE		256
#define MAX_LZW_BITS		12

#define INTERLACE			0x40
#define LOCALCOLORMAP		0x80
#define BitSet(byte, bit)	(((byte) & (bit)) == (bit))
#define LM_to_uint(a, b)	(((b) << 8) | (a))

typedef GalPixbufAnim GalPixbufGifAnim;
typedef euchar CMap[3][MAXCOLORMAPSIZE];

enum {
	GIF_START,
	GIF_GET_COLORMAP,
	GIF_GET_NEXT_STEP,
	GIF_GET_FRAME_INFO,
	GIF_GET_EXTENSION,
	GIF_GET_COLORMAP2,
	GIF_PREPARE_LZW,
	GIF_LZW_FILL_BUFFER,
	GIF_LZW_CLEAR_CODE,
	GIF_GET_LZW,
	GIF_DONE
};

typedef struct _Gif89 Gif89;
struct _Gif89 {
	int transparent;
	int delay_time;
	int input_flag;
	int disposal;
};

typedef struct _GifContext GifContext;
struct _GifContext {
	PixbufContext context;

	eint  state;
	euint width;
	euint height;

	ebool has_global_cmap;

	CMap  global_color_map;
	euint  global_colormap_size;
	euint global_bit_pixel;
	euint global_color_resolution;
	euint background_index;
	ebool  stop_after_first_frame;

	ebool  frame_cmap_active;
	CMap  frame_color_map;
	euint frame_colormap_size;
	euint frame_bit_pixel;

	ebool  is_anim;
	euint aspect_ratio;
	GalPixbufGifAnim *anim;
	GalPixbufFrame   *frame;
	Gif89 gif89;

	/* stuff per frame. */
	int frame_len;
	int frame_height;
	int frame_interlace;
	int x_offset;
	int y_offset;

	euchar *buf;
	euint ptr;
	euint size;
	euint amount_needed;

	/* extension context */
	euchar extension_label;
	euchar extension_flag;
	ebool in_loop_extension;

	/* get block context */
	euchar block_count;
	euchar block_buf[280];
	int block_ptr;

	int old_state; /* used by lzw_fill buffer */
	/* get_code context */
	int code_curbit;
	int code_lastbit;
	int code_done;
	int code_last_byte;
	int lzw_code_pending;

	/* lzw context */
	int lzw_fresh;
	int lzw_code_size;
	euchar lzw_set_code_size;
	int lzw_max_code;
	int lzw_max_code_size;
	int lzw_firstcode;
	int lzw_oldcode;
	int lzw_clear_code;
	int lzw_end_code;
	int *lzw_sp;

	int lzw_table[2][(1 << MAX_LZW_BITS)];
	int lzw_stack[(1 << (MAX_LZW_BITS)) * 2 + 1];

	/* painting context */
	int draw_xpos;
	int draw_ypos;
	int draw_pass;
};

static int GetDataBlock(GifContext *, euchar *);

static ebool gif_read(GifContext *context, euchar *buffer, size_t len)
{
	if ((context->size - context->ptr) >= len) {
		e_memcpy(buffer, context->buf + context->ptr, len);
		context->ptr += len;
		context->amount_needed = 0;
		return etrue;
	}
	context->amount_needed = len - (context->size - context->ptr);
	return efalse;
}

static int gif_get_colormap(GifContext *context)
{
	euchar rgb[3];

	while (context->global_colormap_size < context->global_bit_pixel) {
		if (!gif_read(context, rgb, sizeof(rgb)))
			return -1;

		context->global_color_map[0][context->global_colormap_size] = rgb[0];
		context->global_color_map[1][context->global_colormap_size] = rgb[1];
		context->global_color_map[2][context->global_colormap_size] = rgb[2];

		if (context->is_anim && context->global_colormap_size == context->background_index) {
			context->anim->bg_red    = rgb[0];
			context->anim->bg_green  = rgb[1];
			context->anim->bg_blue   = rgb[2];
		}

		context->global_colormap_size++;
	}

	context->state = GIF_GET_NEXT_STEP;
	return 0;
}

static void gif_set_prepare_lzw(GifContext *context)
{
	context->state = GIF_PREPARE_LZW;
	context->lzw_code_pending = -1;
}

static int gif_get_colormap2(GifContext *context)
{
	euchar rgb[3];

	while (context->frame_colormap_size < context->frame_bit_pixel) {
		if (!gif_read(context, rgb, sizeof (rgb)))
			return -1;

		context->frame_color_map[0][context->frame_colormap_size] = rgb[0];
		context->frame_color_map[1][context->frame_colormap_size] = rgb[1];
		context->frame_color_map[2][context->frame_colormap_size] = rgb[2];

		context->frame_colormap_size++;
	}

	gif_set_prepare_lzw(context);
	return 0;
}

static int get_data_block(GifContext *context, euchar *buf, int *empty_block)
{
	if (context->block_count == 0) {
		if (!gif_read(context, &context->block_count, 1))
			return -1;
	}

	if (context->block_count == 0)
		if (empty_block) {
			*empty_block = etrue;
			return 0;
		}

	if (!gif_read(context, buf, context->block_count))
		return -1;

	return 0;
}

static void gif_set_get_extension(GifContext *context)
{
	context->state = GIF_GET_EXTENSION;
	context->extension_flag = etrue;
	context->extension_label = 0;
	context->block_count = 0;
	context->block_ptr = 0;
}

static int gif_get_extension(GifContext *context)
{
	int retval;
	int empty_block = efalse;

	if (context->extension_flag) {
		if (!context->extension_label &&
				!gif_read(context, &context->extension_label , 1))
			return -1;

		switch (context->extension_label) {
		case 0xf9:			/* Graphic Control Extension */
			retval = get_data_block(context, (euchar *)context->block_buf, NULL);
			if (retval != 0) return retval;

			if (context->frame == NULL) {
				context->gif89.disposal = (context->block_buf[0] >> 2) & 0x7;
				context->gif89.input_flag = (context->block_buf[0] >> 1) & 0x1;
				context->gif89.delay_time = LM_to_uint(context->block_buf[1], context->block_buf[2]);

				if ((context->block_buf[0] & 0x1) != 0)
					context->gif89.transparent = context->block_buf[3];
				else
					context->gif89.transparent = -1;
			}

			/* Now we've successfully loaded this one, we continue on our way */
			context->block_count = 0;
			context->extension_flag = efalse;
			break;

		case 0xff: /* application extension */
			if (!context->in_loop_extension) { 
				retval = get_data_block(context, (euchar *)context->block_buf, NULL);
				if (retval != 0)
					return retval;
				if (!e_strncmp(_(context->block_buf), _("NETSCAPE2.0"), 11) ||
						!e_strncmp(_(context->block_buf), _("ANIMEXTS1.0"), 11)) {
					context->in_loop_extension = etrue;
				}
				context->block_count = 0;
			}

			if (context->in_loop_extension) {
				do {
					retval = get_data_block(context, (euchar *)context->block_buf, &empty_block);
					if (retval != 0)
						return retval;
					if (context->is_anim && context->block_buf[0] == 0x01) {
						context->anim->loop = context->block_buf[1] + (context->block_buf[2] << 8);
						if (context->anim->loop != 0) 
							context->anim->loop++;
					}
					context->block_count = 0;
				} while (!empty_block);

				context->in_loop_extension = efalse;
				context->extension_flag = efalse;
				goto step;
			}
			break;
		default:
			break;
		}
	}
	/* read all blocks, until I get an empty block, in case there was an extension I didn't know about. */
	do {
		retval = get_data_block(context, (euchar *)context->block_buf, &empty_block);
		if (retval != 0)
			return retval;
		context->block_count = 0;
	} while (!empty_block);

step:
	context->state = GIF_GET_NEXT_STEP;
	return 0;
}

static int ZeroDataBlock = efalse;

static int GetDataBlock(GifContext *context, euchar *buf)
{
	/*  	euchar count; */

	if (!gif_read(context, &context->block_count, 1)) {
		/*g_message (_("GIF: error in getting DataBlock size\n"));*/
		return -1;
	}

	ZeroDataBlock = context->block_count == 0;

	if ((context->block_count != 0) && (!gif_read(context, buf, context->block_count))) {
		/*g_message (_("GIF: error in reading DataBlock\n"));*/
		return -1;
	}

	return context->block_count;
}


static void gif_set_lzw_fill_buffer(GifContext *context)
{
	context->block_count = 0;
	context->old_state = context->state;
	context->state = GIF_LZW_FILL_BUFFER;
}

static int gif_lzw_fill_buffer(GifContext *context)
{
	int retval;

	if (context->code_done) {
		if (context->code_curbit >= context->code_lastbit) {
			printf("GIF file was missing some data (perhaps it was truncated somehow?)\n");
			return -2;
		}
		/* Is this supposed to be an error or what? */
		/* g_message ("trying to read more data after we've done stuff\n"); */
		printf("Internal error in the GIF loader \n");

		return -2;
	}

	context->block_buf[0] = context->block_buf[context->code_last_byte - 2];
	context->block_buf[1] = context->block_buf[context->code_last_byte - 1];

	retval = get_data_block(context, &context->block_buf[2], NULL);

	if (retval == -1)
		return -1;

	if (context->block_count == 0)
		context->code_done = etrue;

	context->code_last_byte = 2 + context->block_count;
	context->code_curbit = (context->code_curbit - context->code_lastbit) + 16;
	context->code_lastbit = (2 + context->block_count) * 8;

	context->state = context->old_state;
	return 0;
}

static int get_code(GifContext *context, int code_size)
{
	int i, j, ret;

	if ((context->code_curbit + code_size) >= context->code_lastbit) {
		gif_set_lzw_fill_buffer(context);
		return -3;
	}

	ret = 0;
	for (i = context->code_curbit, j = 0; j < code_size; ++i, ++j)
		ret |= ((context->block_buf[i / 8] & (1 << (i % 8))) != 0) << j;

	context->code_curbit += code_size;

	return ret;
}

static void set_gif_lzw_clear_code(GifContext *context)
{
	context->state = GIF_LZW_CLEAR_CODE;
	context->lzw_code_pending = -1;
}

static int gif_lzw_clear_code(GifContext *context)
{
	int code;

	code = get_code(context, context->lzw_code_size);
	if (code == -3) return -0;

	context->lzw_firstcode = context->lzw_oldcode = code;
	context->lzw_code_pending = code;
	context->state = GIF_GET_LZW;
	return 0;
}

#define CHECK_LZW_SP() do {                                           \
	if ((euchar *)context->lzw_sp >=                                        \
			(euchar *)context->lzw_stack + sizeof(context->lzw_stack)) {       \
		printf("Stack overflow\n");                              				\
		return -2;                                                      \
	}                                                                       \
} while (0)

static int lzw_read_byte(GifContext *context)
{
	int code, incode;
	int retval;
	int my_retval;
	register int i;

	if (context->lzw_code_pending != -1) {
		retval = context->lzw_code_pending;
		context->lzw_code_pending = -1;
		return retval;
	}

	if (context->lzw_fresh) {
		context->lzw_fresh = efalse;
		do {
			retval = get_code(context, context->lzw_code_size);
			if (retval < 0)
				return retval;

			context->lzw_firstcode = context->lzw_oldcode = retval;
		} while (context->lzw_firstcode == context->lzw_clear_code);
		return context->lzw_firstcode;
	}

	if (context->lzw_sp > context->lzw_stack) {
		my_retval = *--(context->lzw_sp);
		return my_retval;
	}

	while ((code = get_code(context, context->lzw_code_size)) >= 0) {
		if (code == context->lzw_clear_code) {
			for (i = 0; i < context->lzw_clear_code; ++i) {
				context->lzw_table[0][i] = 0;
				context->lzw_table[1][i] = i;
			}
			for (; i < (1 << MAX_LZW_BITS); ++i)
				context->lzw_table[0][i] = context->lzw_table[1][i] = 0;
			context->lzw_code_size = context->lzw_set_code_size + 1;
			context->lzw_max_code_size = 2 * context->lzw_clear_code;
			context->lzw_max_code = context->lzw_clear_code + 2;
			context->lzw_sp = context->lzw_stack;

			set_gif_lzw_clear_code(context);
			return -3;
		}
		else if (code == context->lzw_end_code) {
			int count;
			euchar buf[260];

			/*  FIXME - we should handle this case */
			return -2;

			if (ZeroDataBlock)
				return -2;

			while ((count = GetDataBlock(context, buf)) > 0)
				;

			if (count != 0) {
				printf ("GIF: missing EOD in data stream (common occurence)\n");
				return -2;
			}
		}

		incode = code;

		if (code >= context->lzw_max_code) {
			CHECK_LZW_SP();
			*(context->lzw_sp)++ = context->lzw_firstcode;
			code = context->lzw_oldcode;
		}

		while (code >= context->lzw_clear_code) {
			if (code >= (1 << MAX_LZW_BITS))
				return -2;
			CHECK_LZW_SP();
			*(context->lzw_sp)++ = context->lzw_table[1][code];

			if (code == context->lzw_table[0][code])
				return -2;
			code = context->lzw_table[0][code];
		}

		CHECK_LZW_SP();
		*(context->lzw_sp)++ = context->lzw_firstcode = context->lzw_table[1][code];

		if ((code = context->lzw_max_code) < (1 << MAX_LZW_BITS)) {
			context->lzw_table[0][code] = context->lzw_oldcode;
			context->lzw_table[1][code] = context->lzw_firstcode;
			++context->lzw_max_code;
			if ((context->lzw_max_code >= context->lzw_max_code_size) &&
					(context->lzw_max_code_size < (1 << MAX_LZW_BITS))) {
				context->lzw_max_code_size *= 2;
				++context->lzw_code_size;
			}
		}

		context->lzw_oldcode = incode;

		if (context->lzw_sp > context->lzw_stack) {
			my_retval = *--(context->lzw_sp);
			return my_retval;
		}
	}
	return code;
}

static void gif_set_get_lzw(GifContext *context)
{
	context->state = GIF_GET_LZW;
	context->draw_xpos = 0;
	context->draw_ypos = 0;
	context->draw_pass = 0;
}

#if 0
static ebool clip_frame(GifContext *context, int *x, int *y, int *width, int *height)
{
	int orig_x, orig_y;

	orig_x = *x;
	orig_y = *y;
	*x = MAX(0, *x);
	*y = MAX(0, *y);
	*width  = MIN(context->width, orig_x + *width) - *x;
	*height = MIN(context->height, orig_y + *height) - *y;

	if (*width > 0 && *height > 0)
		return etrue;

	*x = 0;
	*y = 0;
	*width = 0;
	*height = 0;

	return efalse;
}
#endif

#define anim_frame_append(anim, frame) \
	STRUCT_LIST_INSERT_BEFORE(GalPixbufFrame, anim->frames, anim->frames_tail, 0, frame, prev, next);

#define anim_frame_delete(anim, frame) \
	STRUCT_LIST_DELETE(anim->frames, anim->frames_tail, frame, prev, next)

static int pixbuf_anim_process(GifContext *context)
{
	PixbufContext *con = (PixbufContext *)context;

	if (context->frame)
		return 0;

	context->frame = e_malloc(sizeof(GalPixbufFrame));
	context->frame->composited = NULL;
	context->frame->revert     = NULL;

	con->pixbuf = NULL;
	if (context->frame_len == 0 || context->frame_height == 0) {
		context->x_offset     = 0;
		context->y_offset     = 0;
		context->frame_len    = 1;
		context->frame_height = 1;
		if (!con->init(con, 1, 1, 4, 1)) {
			euchar *pixels = con->pixbuf->pixels;
			pixels[0] = 0;
			pixels[1] = 0;
			pixels[2] = 0;
			pixels[3] = 0;
		}
	}
	else if (con->init(con, context->frame_len, context->frame_height, 4, 1) < 0)
		return -2;

	context->frame->pixbuf = con->pixbuf;
	if (context->anim->width < con->pixbuf->w)
		context->anim->width = con->pixbuf->w;
	if (context->anim->height < con->pixbuf->h)
		context->anim->height = con->pixbuf->h;

	context->frame->x_offset = context->x_offset;
	context->frame->y_offset = context->y_offset;
	context->frame->need_recomposite = etrue;

	context->frame->delay_time = context->gif89.delay_time * 10;

	if (context->frame->delay_time < 20)
		context->frame->delay_time = 20;

	context->frame->elapsed = context->anim->total_time;
	context->anim->total_time += context->frame->delay_time;

	switch (context->gif89.disposal) {
		case 0:
		case 1:
			context->frame->action = GAL_PIXBUF_FRAME_RETAIN;
			break;
		case 2:
			context->frame->action = GAL_PIXBUF_FRAME_DISPOSE;
			break;
		case 3:
			context->frame->action = GAL_PIXBUF_FRAME_REVERT;
			break;
		default:
			context->frame->action = GAL_PIXBUF_FRAME_RETAIN;
			break;
	}

	context->frame->bg_transparent = (context->gif89.transparent == (eint)context->background_index);

	context->anim->n_frames++;
	anim_frame_append(context->anim, context->frame);

	return 0;
}

static int gif_get_lzw(GifContext *context)
{
	int  lower_bound, upper_bound;
	ebool bound_flag;
	int  first_pass;
	int  v;
	PixbufContext *con = (PixbufContext *)context;

	if (context->is_anim) {
		if ((v = pixbuf_anim_process(context)) < 0)
			return v;
	}
	else if (con->pixbuf == NULL) {
		con->init(con, context->frame_len, context->frame_height, 4, 1);
	}

	bound_flag  = efalse;
	lower_bound = upper_bound = context->draw_ypos;
	first_pass  = context->draw_pass;

	while (etrue) {
		euchar (*cmap)[MAXCOLORMAPSIZE];
		euchar a;

		if (context->frame_cmap_active)
			cmap = context->frame_color_map;
		else
			cmap = context->global_color_map;

		v = lzw_read_byte(context);
		if (v < 0)
			goto finished_data;
		bound_flag = etrue;

		a = (euchar)((v == context->gif89.transparent) ? 0 : 255);
		con->set_x(con, -1, cmap[2][v], cmap[1][v], cmap[0][v], a);

		context->draw_xpos++;
		if (context->draw_xpos == context->frame_len) {
			context->draw_xpos = 0;
			if (context->frame_interlace) {
				switch (context->draw_pass) {
					case 0:
					case 1:
						context->draw_ypos += 8;
						break;
					case 2:
						context->draw_ypos += 4;
						break;
					case 3:
						context->draw_ypos += 2;
						break;
				}

				if (context->draw_ypos >= context->frame_height) {
					context->draw_pass++;
					switch (context->draw_pass) {
						case 1:
							context->draw_ypos = 4;
							break;
						case 2:
							context->draw_ypos = 2;
							break;
						case 3:
							context->draw_ypos = 1;
							break;
						default:
							goto done;
					}
				}

				con->set_y(con, context->draw_ypos);
			}
			else {
				context->draw_ypos++;
				con->set_y(con, -1);
			}

			if (context->draw_pass != first_pass) {
				if (context->draw_ypos > lower_bound) {
					lower_bound = 0;
					upper_bound = context->frame_height;
				}
			} 
			else {
				upper_bound = context->draw_ypos;
			}
		}
		if (context->draw_ypos >= context->frame_height)
			break;
	}

done:
	context->state = GIF_GET_NEXT_STEP;
	v = 0;

finished_data:
	if (bound_flag && context->frame)
		context->frame->need_recomposite = etrue;

	if (context->state == GIF_GET_NEXT_STEP) {
		context->frame = NULL;
		context->frame_cmap_active = efalse;
		if (!context->is_anim || context->stop_after_first_frame)
			context->state =  GIF_DONE;
	}

	return v;
}

static int gif_prepare_lzw(GifContext *context)
{
	int i;

	if (!gif_read(context, &(context->lzw_set_code_size), 1))
		return -1;

	if (context->lzw_set_code_size > MAX_LZW_BITS) {
		printf("GIF pixbuf is corrupt (incorrect LZW compression)\n");
		return -2;
	}

	context->lzw_code_size     = context->lzw_set_code_size + 1;
	context->lzw_clear_code    = 1 << context->lzw_set_code_size;
	context->lzw_end_code      = context->lzw_clear_code + 1;
	context->lzw_max_code      = context->lzw_clear_code + 2;
	context->lzw_max_code_size = 2 * context->lzw_clear_code;
	context->lzw_fresh         = etrue;
	context->code_curbit       = 0;
	context->code_lastbit      = 0;
	context->code_last_byte    = 0;
	context->code_done         = efalse;

	e_assert(context->lzw_clear_code <= TABLES_SIZEOF(context->lzw_table[0]));

	for (i = 0; i < context->lzw_clear_code; ++i) {
		context->lzw_table[0][i] = 0;
		context->lzw_table[1][i] = i;
	}
	for (; i < (1 << MAX_LZW_BITS); ++i)
		context->lzw_table[0][i] = context->lzw_table[1][0] = 0;

	context->lzw_sp = context->lzw_stack;
	gif_set_get_lzw(context);

	return 0;
}

static int gif_init(GifContext *context)
{
	euchar buf[16], *p;
	echar version[4];

	if (!gif_read(context, buf, 13))
		return -1;

	p = buf;
	if (e_strncmp((echar *)p, _("GIF"), 3) != 0)
		return -2;

	p += 3;
	e_strncpy(version, (echar *)p, 3);
	version[3] = '\0';

	if ((e_strcmp(version, _("87a")) != 0) && (e_strcmp(version, _("89a")) != 0)) {
		printf("Version %s of the GIF file format is not supported\n", version);
		return -2;
	}

	p += 3;
	//context->width  = LM_to_uint(p[0], p[1]);
	//context->height = LM_to_uint(p[2], p[3]);
	context->width  = 0;
	context->height = 0;
	context->global_bit_pixel        = 2 << (p[4] & 0x07);
	context->global_color_resolution = (((p[4] & 0x70) >> 3) + 1);
	context->has_global_cmap         = (p[4] & 0x80) != 0;
	context->background_index        = p[5];
	context->aspect_ratio            = p[6];

	if (context->is_anim) {
		context->anim->bg_red   = 0;
		context->anim->bg_green = 0;
		context->anim->bg_blue  = 0;

		context->anim->width    = 0;
		context->anim->height   = 0;
	}

	if (context->has_global_cmap)  {
		context->global_colormap_size = 0;
		context->state = GIF_GET_COLORMAP;
	}
	else
		context->state = GIF_GET_NEXT_STEP;

	return 0;
}

static int gif_get_frame_info(GifContext *context)
{
	euchar buf[9];

	if (!gif_read(context, buf, 9))
		return -1;

	context->x_offset     = LM_to_uint(buf[0], buf[1]);
	context->y_offset     = LM_to_uint(buf[2], buf[3]);
	context->frame_len    = LM_to_uint(buf[4], buf[5]);
	context->frame_height = LM_to_uint(buf[6], buf[7]);

	if (context->is_anim
			&& !context->anim->frames
			&& context->gif89.disposal == 3)
		context->gif89.disposal = 0;

	context->frame_interlace = BitSet(buf[8], INTERLACE);

	if (BitSet(buf[8], LOCALCOLORMAP)) {
		context->frame_bit_pixel     = 1 << ((buf[8] & 0x07) + 1);
		context->frame_cmap_active   = etrue;
		context->frame_colormap_size = 0;
		context->state = GIF_GET_COLORMAP2;
		return 0;
	}

	if (!context->has_global_cmap) {
		context->state = GIF_DONE;
		return -2;
	}

	gif_set_prepare_lzw(context);
	return 0;
}

static int gif_get_next_step(GifContext *context)
{
	euchar c;
	while (etrue) {
		if (!gif_read(context, &c, 1))
			return -1;

		if (c == ';') {
			context->state = GIF_DONE;
			return 0;
		}

		if (c == '!') {
			gif_set_get_extension(context);
			return 0;
		}

		if (c != ',')
			continue;

		context->state = GIF_GET_FRAME_INFO;
		return 0;
	}
}

static int gif_main_loop(GifContext *context)
{
	int retval = 0;

	do {
		switch (context->state) {
			case GIF_START:
				retval = gif_init(context);
				break;

			case GIF_GET_COLORMAP:
				retval = gif_get_colormap(context);
				break;

			case GIF_GET_NEXT_STEP:
				retval = gif_get_next_step(context);
				break;

			case GIF_GET_FRAME_INFO:
				retval = gif_get_frame_info(context);
				break;

			case GIF_GET_EXTENSION:
				retval = gif_get_extension(context);
				break;

			case GIF_GET_COLORMAP2:
				retval = gif_get_colormap2(context);
				break;

			case GIF_PREPARE_LZW:
				retval = gif_prepare_lzw(context);
				break;

			case GIF_LZW_FILL_BUFFER:
				retval = gif_lzw_fill_buffer(context);
				break;

			case GIF_LZW_CLEAR_CODE:
				retval = gif_lzw_clear_code(context);
				break;

			case GIF_GET_LZW:
				retval = gif_get_lzw(context);
				break;

			case GIF_DONE:
			default:
				retval = 0;
				goto done;
		};
	} while ((retval == 0) || (retval == -3));

done:
	return retval;
}

static GifContext *new_context(ebool is_anim)
{
	GifContext *context;

	context = e_malloc(sizeof(GifContext));
	if (context == NULL)
		return NULL;

	memset(context, 0, sizeof(GifContext));

	context->is_anim = is_anim;
	if (is_anim) {
		context->anim = (GalPixbufGifAnim *)e_calloc(1, sizeof(GalPixbufGifAnim));
		context->anim->loop = 1;
	}
	context->state             = GIF_START;
	context->buf               = NULL;
	context->frame             = NULL;
	context->amount_needed     = 0;
	context->gif89.disposal    = -1;
	context->gif89.delay_time  = -1;
	context->gif89.input_flag  = -1;
	context->gif89.transparent = -1;
	context->in_loop_extension = efalse;
	context->stop_after_first_frame = efalse;

	return context;
}

static ePointer gif_load_begin(void)
{
	GifContext *context;

	context = new_context(efalse);

	return (ePointer)context;
}

static GalPixbuf *gif_load_end(ePointer data)
{
	GifContext *context = (GifContext *)data;
	GalPixbuf  *pixbuf  = ((PixbufContext *)data)->pixbuf;

	if (context->state != GIF_DONE)
		printf("GIF pixbuf was truncated or incomplete.\n");

	e_free(context);
	return pixbuf;
}

static ebool gif_load_increment(ePointer data, const euchar *buf, euint size)
{
	int retval;
	GifContext *context = (GifContext *)data;

	if (context->amount_needed == 0) {
		context->buf = (euchar *)buf;
		context->ptr = 0;
		context->size = size;
	}
	else {
		if (size < context->amount_needed) {
			context->amount_needed -= size;
			e_memcpy(context->buf + context->size, buf, size);
			context->size += size;
			return etrue;
		}
		else if (size == context->amount_needed) {
			e_memcpy(context->buf + context->size, buf, size);
			context->size += size;
		}
		else {
			context->buf = e_realloc(context->buf, context->size + size);
			e_memcpy(context->buf + context->size, buf, size);
			context->size += size;
		}
	}

	retval = gif_main_loop(context);

	if (retval == -2) {
		if (context->buf == buf)
			context->buf = NULL;
		return efalse;
	}

	if (retval == -1) {
		if (context->buf == buf) {
			e_assert(context->size == size);
			context->buf = e_malloc(context->amount_needed + (context->size - context->ptr));
			e_memcpy(context->buf, buf + context->ptr, context->size - context->ptr);
		}
		else {
			e_memmove(context->buf, context->buf + context->ptr, context->size - context->ptr);
			context->buf = e_realloc(context->buf, context->amount_needed + (context->size - context->ptr));
		}
		context->size = context->size - context->ptr;
		context->ptr = 0;
	}
	else {
		if (context->buf == buf)
			context->buf = NULL;
	}

	if (context->state == GIF_DONE)
		return efalse;
	return etrue;
}

static ePointer gif_anim_load_begin(void)
{
	GifContext *context;

	context = new_context(etrue);

	return (ePointer)context;
}

static void gif_anim_free(GalPixbufAnim *anim)
{
	GalPixbufFrame *frame = anim->frames;
	while (frame) {
		GalPixbufFrame *t = frame;
		egal_pixbuf_free(t->pixbuf);
		e_free(t);
	}
}

static GalPixbufAnim *gif_anim_load_end(ePointer data)
{
	GifContext    *context = (GifContext *)data;
	GalPixbufAnim *anim    = context->anim;

	if (context->state != GIF_DONE) {
		printf("GIF pixbuf was truncated or incomplete.\n");
		gif_anim_free(context->anim);
		anim = NULL;
	}

	e_free(context->buf);
	e_free(context);
	return anim;
}

void _gal_pixbuf_gif_fill_vtable(GalPixbufModule *module)
{
	module->load_end        = gif_load_end;
	module->load_begin      = gif_load_begin;
	module->load_increment  = gif_load_increment;
	module->anim_load_end   = gif_anim_load_end;
	module->anim_load_begin = gif_anim_load_begin;
}
