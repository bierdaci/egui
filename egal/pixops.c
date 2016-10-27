#include <math.h>

#include "pixops.h"

#define SUBSAMPLE_BITS 4
#define SUBSAMPLE (1 << SUBSAMPLE_BITS)
#define SUBSAMPLE_MASK ((1 << SUBSAMPLE_BITS)-1)
#define SCALE_SHIFT 16

#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static void
_pixops_scale_real(euchar        *dest_buf,
		int            render_x0,
		int            render_y0,
		int            render_x1,
		int            render_y1,
		int            dest_rowstride,
		int            dest_channels,
		ebool       dest_has_alpha,
		const euchar  *src_buf,
		int            src_width,
		int            src_height,
		int            src_rowstride,
		int            src_channels,
		ebool       src_has_alpha,
		double         scale_x,
		double         scale_y,
		PixopsInterpType  interp_type);

typedef struct _PixopsFilter PixopsFilter;
typedef struct _PixopsFilterDimension PixopsFilterDimension;

struct _PixopsFilterDimension {
	int n;
	double offset;
	double *weights;
};

struct _PixopsFilter {
	PixopsFilterDimension x;
	PixopsFilterDimension y;
	double overall_alpha;
}; 

typedef euchar *(*PixopsLineFunc)(int *weights, int n_x, int n_y,
		euchar *dest, int dest_x, euchar *dest_end,
		int dest_channels, int dest_has_alpha,
		euchar **src, int src_channels,
		ebool src_has_alpha, int x_init,
		int x_step, int src_width, int check_size,
		euint32 color1, euint32 color2);
typedef void (*PixopsPixelFunc)(euchar *dest, int dest_x, int dest_channels,
		int dest_has_alpha, int src_has_alpha,
		int check_size, euint32 color1,
		euint32 color2,
		euint r, euint g, euint b, euint a);

static int get_check_shift(int check_size)
{
	int check_shift = 0;

	if (check_size < 0)
		return 4;

	while (!(check_size & 1)) {
		check_shift++;
		check_size >>= 1;
	}

	return check_shift;
}

static void
pixops_scale_nearest(euchar        *dest_buf,
		int            render_x0,
		int            render_y0,
		int            render_x1,
		int            render_y1,
		int            dest_rowstride,
		int            dest_channels,
		ebool           dest_has_alpha,
		const euchar *src_buf,
		int            src_width,
		int            src_height,
		int            src_rowstride,
		int            src_channels,
		ebool           src_has_alpha,
		double         scale_x,
		double         scale_y)
{
	int i;
	int x;
	int x_step = (int)((1 << SCALE_SHIFT) / scale_x);
	int y_step = (int)((1 << SCALE_SHIFT) / scale_y);
	int xmax, xstart, xstop, x_pos, y_pos;
	const euchar *p;

#define INNER_LOOP(SRC_CHANNELS, DEST_CHANNELS, ASSIGN_PIXEL)     \
	xmax = x + (render_x1 - render_x0) * x_step;              \
	xstart = MIN(0, xmax);                                   \
	xstop = MIN(src_width << SCALE_SHIFT, xmax);             \
	p = src + (CLAMP(x, xstart, xstop) >> SCALE_SHIFT) * SRC_CHANNELS; \
	while (x < xstart)                                        \
	{                                                       \
		dest += DEST_CHANNELS;                                \
		x += x_step;                                          \
	}                                                       \
	while (x < xstop)                                         \
	{                                                       \
		p = src + (x >> SCALE_SHIFT) * SRC_CHANNELS;          \
		ASSIGN_PIXEL;                                         \
		dest += DEST_CHANNELS;                                \
		x += x_step;                                          \
	}                                                       \
	x_pos = x >> SCALE_SHIFT;                                 \
	p = src + CLAMP(x_pos, 0, src_width - 1) * SRC_CHANNELS; \
	while (x < xmax)                                          \
	{                                                       \
		dest += DEST_CHANNELS;                                \
		x += x_step;                                          \
	}

	for (i = 0; i < (render_y1 - render_y0); i++) {
		const euchar *src;
		euchar       *dest;
		y_pos = ((i + render_y0) * y_step + y_step / 2) >> SCALE_SHIFT;
		y_pos = CLAMP(y_pos, 0, src_height - 1);
		src  = src_buf + y_pos * src_rowstride;
		dest = dest_buf + i * dest_rowstride;

		x = render_x0 * x_step + x_step / 2;

		if (src_channels == 3) {
			if (dest_channels == 3) {
				INNER_LOOP(3, 3, dest[0] = p[0]; dest[1] = p[1]; dest[2] = p[2]);
			}
			else {
				INNER_LOOP(3, 4, dest[0] = p[0]; dest[1] = p[1]; dest[2] = p[2]; dest[3] = 0xff);
			}
		}
		else if (src_channels == 4) {
			if (dest_channels == 3) {
				INNER_LOOP(4, 3, dest[0] = p[0]; dest[1] = p[1]; dest[2] = p[2]);
			}
			else {
				euint32 *p32;
				INNER_LOOP(4, 4, p32 = (euint32*)dest; *p32 = *((euint32*)p));
			}
		}
	}
}

static void
pixops_composite_nearest(euchar        *dest_buf,
		int            render_x0,
		int            render_y0,
		int            render_x1,
		int            render_y1,
		int            dest_rowstride,
		int            dest_channels,
		ebool       dest_has_alpha,
		const euchar  *src_buf,
		int            src_width,
		int            src_height,
		int            src_rowstride,
		int            src_channels,
		ebool       src_has_alpha,
		double         scale_x,
		double         scale_y,
		int            overall_alpha)
{
	int i;
	int x;
	int x_step = (int)((1 << SCALE_SHIFT) / scale_x);
	int y_step = (int)((1 << SCALE_SHIFT) / scale_y);
	int xmax, xstart, xstop, x_pos, y_pos;
	const euchar *p;
	unsigned int  a0;

	for (i = 0; i < (render_y1 - render_y0); i++) {
		const euchar *src;
		euchar       *dest;
		y_pos = ((i + render_y0) * y_step + y_step / 2) >> SCALE_SHIFT;
		y_pos = CLAMP(y_pos, 0, src_height - 1);
		src  = src_buf + y_pos * src_rowstride;
		dest = dest_buf + i * dest_rowstride;

		x = render_x0 * x_step + x_step / 2;
		INNER_LOOP(src_channels, dest_channels, 
				if (src_has_alpha)
					a0 = (p[3] * overall_alpha) / 0xff;
				else
					a0 = overall_alpha;

				switch (a0) {
					case 0:
						break;
					case 255:
						dest[0] = p[0];
						dest[1] = p[1];
						dest[2] = p[2];
						if (dest_has_alpha)
							dest[3] = 0xff;
						break;
					default:
						if (dest_has_alpha) {
							unsigned int w0 = 0xff * a0;
							unsigned int w1 = (0xff - a0) * dest[3];
							unsigned int w = w0 + w1;

							dest[0] = (w0 * p[0] + w1 * dest[0]) / w;
							dest[1] = (w0 * p[1] + w1 * dest[1]) / w;
							dest[2] = (w0 * p[2] + w1 * dest[2]) / w;
							dest[3] = w / 0xff;
						}
						else {
							unsigned int a1 = 0xff - a0;
							unsigned int tmp;

							tmp = a0 * p[0] + a1 * dest[0] + 0x80;
							dest[0] = (tmp + (tmp >> 8)) >> 8;
							tmp = a0 * p[1] + a1 * dest[1] + 0x80;
							dest[1] = (tmp + (tmp >> 8)) >> 8;
							tmp = a0 * p[2] + a1 * dest[2] + 0x80;
							dest[2] = (tmp + (tmp >> 8)) >> 8;
						}
						break;
				}
		);
	}
}

static void
pixops_composite_color_nearest(euchar        *dest_buf,
		int            render_x0,
		int            render_y0,
		int            render_x1,
		int            render_y1,
		int            dest_rowstride,
		int            dest_channels,
		ebool       dest_has_alpha,
		const euchar  *src_buf,
		int            src_width,
		int            src_height,
		int            src_rowstride,
		int            src_channels,
		ebool       src_has_alpha,
		double         scale_x,
		double         scale_y,
		int            overall_alpha,
		int            check_x,
		int            check_y,
		int            check_size,
		euint32        color1,
		euint32        color2)
{
	int i, j;
	int x;
	int x_step = (int)((1 << SCALE_SHIFT) / scale_x);
	int y_step = (int)((1 << SCALE_SHIFT) / scale_y);
	int r1, g1, b1, r2, g2, b2;
	int check_shift = get_check_shift(check_size);
	int xmax, xstart, xstop, x_pos, y_pos;
	const euchar *p;
	unsigned int  a0;

	for (i = 0; i < (render_y1 - render_y0); i++) {
		const euchar *src;
		euchar       *dest;
		y_pos = ((i + render_y0) * y_step + y_step / 2) >> SCALE_SHIFT;
		y_pos = CLAMP(y_pos, 0, src_height - 1);
		src  = src_buf + y_pos * src_rowstride;
		dest = dest_buf + i * dest_rowstride;

		x = render_x0 * x_step + x_step / 2;


		if (((i + check_y) >> check_shift) & 1) {
			r1 = (color2 & 0xff0000) >> 16;
			g1 = (color2 & 0xff00) >> 8;
			b1 = color2 & 0xff;

			r2 = (color1 & 0xff0000) >> 16;
			g2 = (color1 & 0xff00) >> 8;
			b2 = color1 & 0xff;
		}
		else {
			r1 = (color1 & 0xff0000) >> 16;
			g1 = (color1 & 0xff00) >> 8;
			b1 = color1 & 0xff;

			r2 = (color2 & 0xff0000) >> 16;
			g2 = (color2 & 0xff00) >> 8;
			b2 = color2 & 0xff;
		}

		j = 0;
		INNER_LOOP(src_channels, dest_channels,
			if (src_has_alpha)
				a0 = (p[3] * overall_alpha + 0xff) >> 8;
			else
				a0 = overall_alpha;

			switch (a0)
			{
			case 0:
			if (((j + check_x) >> check_shift) & 1) {
				dest[0] = r2; 
				dest[1] = g2; 
				dest[2] = b2;
			}
			else {
				dest[0] = r1; 
				dest[1] = g1; 
				dest[2] = b1;
			}
			break;
			case 255:
			dest[0] = p[0];
			dest[1] = p[1];
			dest[2] = p[2];
			break;
			default:
			{
				unsigned int tmp;
				if (((j + check_x) >> check_shift) & 1) {
					tmp = ((int) p[0] - r2) * a0;
					dest[0] = r2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
					tmp = ((int) p[1] - g2) * a0;
					dest[1] = g2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
					tmp = ((int) p[2] - b2) * a0;
					dest[2] = b2 + ((tmp + (tmp >> 8) + 0x80) >> 8);
				}
				else {
					tmp = ((int) p[0] - r1) * a0;
					dest[0] = r1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
					tmp = ((int) p[1] - g1) * a0;
					dest[1] = g1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
					tmp = ((int) p[2] - b1) * a0;
					dest[2] = b1 + ((tmp + (tmp >> 8) + 0x80) >> 8);
				}
			}
			break;
			}

			if (dest_channels == 4)
				dest[3] = 0xff;

			j++;
		);
	}
}
#undef INNER_LOOP

static void
composite_pixel(euchar *dest, int dest_x, int dest_channels, int dest_has_alpha,
		int src_has_alpha, int check_size, euint32 color1, euint32 color2,
		euint r, euint g, euint b, euint a)
{
	if (dest_has_alpha) {
		unsigned int w0 = a - (a >> 8);
		unsigned int w1 = ((0xff0000 - a) >> 8) * dest[3];
		unsigned int w = w0 + w1;

		if (w != 0) {
			dest[0] = (r - (r >> 8) + w1 * dest[0]) / w;
			dest[1] = (g - (g >> 8) + w1 * dest[1]) / w;
			dest[2] = (b - (b >> 8) + w1 * dest[2]) / w;
			dest[3] = w / 0xff00;
		}
		else {
			dest[0] = 0;
			dest[1] = 0;
			dest[2] = 0;
			dest[3] = 0;
		}
	}
	else {
		dest[0] = (r + (0xff0000 - a) * dest[0]) / 0xff0000;
		dest[1] = (g + (0xff0000 - a) * dest[1]) / 0xff0000;
		dest[2] = (b + (0xff0000 - a) * dest[2]) / 0xff0000;
	}
}

static euchar *
composite_line(int *weights, int n_x, int n_y,
		euchar *dest, int dest_x, euchar *dest_end, int dest_channels, int dest_has_alpha,
		euchar **src, int src_channels, ebool src_has_alpha,
		int x_init, int x_step, int src_width,
		int check_size, euint32 color1, euint32 color2)
{
	int x = x_init;
	int i, j;

	while (dest < dest_end) {
		int x_scaled = x >> SCALE_SHIFT;
		unsigned int r = 0, g = 0, b = 0, a = 0;
		int *pixel_weights;

		pixel_weights = weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * n_x * n_y;

		for (i = 0; i < n_y; i++) {
			euchar *q = src[i] + x_scaled * src_channels;
			int *line_weights = pixel_weights + n_x * i;

			for (j = 0; j < n_x; j++) {
				unsigned int ta;

				if (src_has_alpha)
					ta = q[3] * line_weights[j];
				else
					ta = 0xff * line_weights[j];

				r += ta * q[0];
				g += ta * q[1];
				b += ta * q[2];
				a += ta;

				q += src_channels;
			}
		}

		if (dest_has_alpha) {
			unsigned int w0 = a - (a >> 8);
			unsigned int w1 = ((0xff0000 - a) >> 8) * dest[3];
			unsigned int w = w0 + w1;

			if (w != 0) {
				dest[0] = (r - (r >> 8) + w1 * dest[0]) / w;
				dest[1] = (g - (g >> 8) + w1 * dest[1]) / w;
				dest[2] = (b - (b >> 8) + w1 * dest[2]) / w;
				dest[3] = w / 0xff00;
			}
			else {
				dest[0] = 0;
				dest[1] = 0;
				dest[2] = 0;
				dest[3] = 0;
			}
		}
		else {
			dest[0] = (r + (0xff0000 - a) * dest[0]) / 0xff0000;
			dest[1] = (g + (0xff0000 - a) * dest[1]) / 0xff0000;
			dest[2] = (b + (0xff0000 - a) * dest[2]) / 0xff0000;
		}

		dest += dest_channels;
		x += x_step;
	}

	return dest;
}

static euchar *
composite_line_22_4a4(int *weights, int n_x, int n_y,
		euchar *dest, int dest_x, euchar *dest_end, int dest_channels, int dest_has_alpha,
		euchar **src, int src_channels, ebool src_has_alpha,
		int x_init, int x_step, int src_width,
		int check_size, euint32 color1, euint32 color2)
{
	int x = x_init;
	euchar *src0 = src[0];
	euchar *src1 = src[1];

	if (src_channels == 3 || !src_has_alpha)
		return dest;

	while (dest < dest_end) {
		int x_scaled = x >> SCALE_SHIFT;
		unsigned int r, g, b, a, ta;
		int *pixel_weights;
		euchar *q0, *q1;
		int w1, w2, w3, w4;

		q0 = src0 + x_scaled * 4;
		q1 = src1 + x_scaled * 4;

		pixel_weights = (int *)((char *)weights +
				((x >> (SCALE_SHIFT - SUBSAMPLE_BITS - 4)) & (SUBSAMPLE_MASK << 4)));

		w1 = pixel_weights[0];
		w2 = pixel_weights[1];
		w3 = pixel_weights[2];
		w4 = pixel_weights[3];

		a = w1 * q0[3];
		r = a * q0[0];
		g = a * q0[1];
		b = a * q0[2];

		ta = w2 * q0[7];
		r += ta * q0[4];
		g += ta * q0[5];
		b += ta * q0[6];
		a += ta;

		ta = w3 * q1[3];
		r += ta * q1[0];
		g += ta * q1[1];
		b += ta * q1[2];
		a += ta;

		ta = w4 * q1[7];
		r += ta * q1[4];
		g += ta * q1[5];
		b += ta * q1[6];
		a += ta;

		dest[0] = ((0xff0000 - a) * dest[0] + r) >> 24;
		dest[1] = ((0xff0000 - a) * dest[1] + g) >> 24;
		dest[2] = ((0xff0000 - a) * dest[2] + b) >> 24;
		dest[3] = a >> 16;

		dest += 4;
		x += x_step;
	}

	return dest;
}

static void
composite_pixel_color(euchar *dest, int dest_x, int dest_channels,
		int dest_has_alpha, int src_has_alpha, int check_size,
		euint32 color1, euint32 color2, euint r, euint g,
		euint b, euint a)
{
	int dest_r, dest_g, dest_b;
	int check_shift = get_check_shift(check_size);

	if ((dest_x >> check_shift) & 1) {
		dest_r = (color2 & 0xff0000) >> 16;
		dest_g = (color2 & 0xff00) >> 8;
		dest_b = color2 & 0xff;
	}
	else {
		dest_r = (color1 & 0xff0000) >> 16;
		dest_g = (color1 & 0xff00) >> 8;
		dest_b = color1 & 0xff;
	}

	dest[0] = ((0xff0000 - a) * dest_r + r) >> 24;
	dest[1] = ((0xff0000 - a) * dest_g + g) >> 24;
	dest[2] = ((0xff0000 - a) * dest_b + b) >> 24;

	if (dest_has_alpha)
		dest[3] = 0xff;
	else if (dest_channels == 4)
		dest[3] = a >> 16;
}

static euchar *
composite_line_color(int *weights, int n_x, int n_y, euchar *dest,
		int dest_x, euchar *dest_end, int dest_channels,
		int dest_has_alpha, euchar **src, int src_channels,
		ebool src_has_alpha, int x_init, int x_step,
		int src_width, int check_size, euint32 color1,
		euint32 color2)
{
	int x = x_init;
	int i, j;
	int check_shift = get_check_shift(check_size);
	int dest_r1, dest_g1, dest_b1;
	int dest_r2, dest_g2, dest_b2;

	if (check_size == 0)
		return dest;

	dest_r1 = (color1 & 0xff0000) >> 16;
	dest_g1 = (color1 & 0xff00) >> 8;
	dest_b1 = color1 & 0xff;

	dest_r2 = (color2 & 0xff0000) >> 16;
	dest_g2 = (color2 & 0xff00) >> 8;
	dest_b2 = color2 & 0xff;

	while (dest < dest_end) {
		int x_scaled = x >> SCALE_SHIFT;
		unsigned int r = 0, g = 0, b = 0, a = 0;
		int *pixel_weights;

		pixel_weights = weights + ((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * n_x * n_y;

		for (i = 0; i<n_y; i++) {
			euchar *q = src[i] + x_scaled * src_channels;
			int *line_weights = pixel_weights + n_x * i;

			for (j = 0; j<n_x; j++) {
				unsigned int ta;

				if (src_has_alpha)
					ta = q[3] * line_weights[j];
				else
					ta = 0xff * line_weights[j];

				r += ta * q[0];
				g += ta * q[1];
				b += ta * q[2];
				a += ta;

				q += src_channels;
			}
		}

		if ((dest_x >> check_shift) & 1) {
			dest[0] = ((0xff0000 - a) * dest_r2 + r) >> 24;
			dest[1] = ((0xff0000 - a) * dest_g2 + g) >> 24;
			dest[2] = ((0xff0000 - a) * dest_b2 + b) >> 24;
		}
		else {
			dest[0] = ((0xff0000 - a) * dest_r1 + r) >> 24;
			dest[1] = ((0xff0000 - a) * dest_g1 + g) >> 24;
			dest[2] = ((0xff0000 - a) * dest_b1 + b) >> 24;
		}

		if (dest_has_alpha)
			dest[3] = 0xff;
		else if (dest_channels == 4)
			dest[3] = a >> 16;

		dest += dest_channels;
		x += x_step;
		dest_x++;
	}

	return dest;
}


static void
scale_pixel(euchar *dest, int dest_x, int dest_channels, int dest_has_alpha,
		int src_has_alpha, int check_size, euint32 color1, euint32 color2,
		euint r, euint g, euint b, euint a)
{
	if (src_has_alpha) {
		if (a) {
			dest[0] = r / a;
			dest[1] = g / a;
			dest[2] = b / a;
			dest[3] = a >> 16;
		}
		else {
			dest[0] = 0;
			dest[1] = 0;
			dest[2] = 0;
			dest[3] = 0;
		}
	}
	else {
		dest[0] = (r + 0xffffff) >> 24;
		dest[1] = (g + 0xffffff) >> 24;
		dest[2] = (b + 0xffffff) >> 24;

		if (dest_has_alpha)
			dest[3] = 0xff;
	}
}

static euchar *
scale_line(int *weights, int n_x, int n_y, euchar *dest, int dest_x,
		euchar *dest_end, int dest_channels, int dest_has_alpha,
		euchar **src, int src_channels, ebool src_has_alpha, int x_init,
		int x_step, int src_width, int check_size, euint32 color1,
		euint32 color2)
{
	int x = x_init;
	int i, j;

	while (dest < dest_end) {
		int x_scaled = x >> SCALE_SHIFT;
		int *pixel_weights;

		pixel_weights = weights +
			((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * n_x * n_y;

		if (src_has_alpha) {
			unsigned int r = 0, g = 0, b = 0, a = 0;
			for (i = 0; i < n_y; i++) {
				euchar *q = src[i] + x_scaled * src_channels;
				int *line_weights  = pixel_weights + n_x * i;

				for (j = 0; j < n_x; j++) {
					unsigned int ta;

					ta = q[3] * line_weights[j];
					r += ta * q[0];
					g += ta * q[1];
					b += ta * q[2];
					a += ta;

					q += src_channels;
				}
			}

			if (a) {
				dest[0] = r / a;
				dest[1] = g / a;
				dest[2] = b / a;
				dest[3] = a >> 16;
			}
			else {
				dest[0] = 0;
				dest[1] = 0;
				dest[2] = 0;
				dest[3] = 0;
			}
		}
		else {
			unsigned int r = 0, g = 0, b = 0;
			for (i = 0; i < n_y; i++) {
				euchar *q = src[i] + x_scaled * src_channels;
				int *line_weights  = pixel_weights + n_x * i;

				for (j = 0; j < n_x; j++) {
					unsigned int ta = line_weights[j];

					r += ta * q[0];
					g += ta * q[1];
					b += ta * q[2];

					q += src_channels;
				}
			}

			dest[0] = (r + 0xffff) >> 16;
			dest[1] = (g + 0xffff) >> 16;
			dest[2] = (b + 0xffff) >> 16;

			if (dest_has_alpha)
				dest[3] = 0xff;
		}

		dest += dest_channels;

		x += x_step;
	}

	return dest;
}


static euchar *
scale_line_22_33(int *weights, int n_x, int n_y, euchar *dest, int dest_x,
		euchar *dest_end, int dest_channels, int dest_has_alpha,
		euchar **src, int src_channels, ebool src_has_alpha,
		int x_init, int x_step, int src_width,
		int check_size, euint32 color1, euint32 color2)
{
	int x = x_init;
	euchar *src0 = src[0];
	euchar *src1 = src[1];

	while (dest < dest_end) {
		unsigned int r, g, b;
		int x_scaled = x >> SCALE_SHIFT;
		int *pixel_weights;
		euchar *q0, *q1;
		int w1, w2, w3, w4;

		q0 = src0 + x_scaled * 3;
		q1 = src1 + x_scaled * 3;

		pixel_weights = weights +
			((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) * 4;

		w1 = pixel_weights[0];
		w2 = pixel_weights[1];
		w3 = pixel_weights[2];
		w4 = pixel_weights[3];

		r = w1 * q0[0];
		g = w1 * q0[1];
		b = w1 * q0[2];

		r += w2 * q0[3];
		g += w2 * q0[4];
		b += w2 * q0[5];

		r += w3 * q1[0];
		g += w3 * q1[1];
		b += w3 * q1[2];

		r += w4 * q1[3];
		g += w4 * q1[4];
		b += w4 * q1[5];

		dest[0] = (r + 0x8000) >> 16;
		dest[1] = (g + 0x8000) >> 16;
		dest[2] = (b + 0x8000) >> 16;

		dest += 3;
		x += x_step;
	}

	return dest;
}

static void
process_pixel(int *weights, int n_x, int n_y, euchar *dest, int dest_x,
		int dest_channels, int dest_has_alpha, euchar **src,
		int src_channels, ebool src_has_alpha, int x_start,
		int src_width, int check_size, euint32 color1, euint32 color2,
		PixopsPixelFunc pixel_func)
{
	unsigned int r = 0, g = 0, b = 0, a = 0;
	int i, j;

	for (i = 0; i < n_y; i++) {
		int *line_weights  = weights + n_x * i;

		for (j = 0; j < n_x; j++) {
			unsigned int ta;
			euchar *q;

			if (x_start + j < 0)
				q = src[i];
			else if (x_start + j < src_width)
				q = src[i] + (x_start + j) * src_channels;
			else
				q = src[i] + (src_width - 1) * src_channels;

			if (src_has_alpha)
				ta = q[3] * line_weights[j];
			else
				ta = 0xff * line_weights[j];

			r += ta * q[0];
			g += ta * q[1];
			b += ta * q[2];
			a += ta;
		}
	}

	(*pixel_func)(dest, dest_x, dest_channels, dest_has_alpha, src_has_alpha,
			check_size, color1, color2, r, g, b, a);
}

static void 
correct_total(int    *weights, 
		int    n_x, 
		int    n_y,
		int    total, 
		double overall_alpha)
{
	int correction = (int)(0.5 + 65536 * overall_alpha) - total;
	int remaining, c, d, i;

	if (correction != 0) {
		remaining = correction;
		for (d = 1, c = correction; c != 0 && remaining != 0; d++, c = correction / d) 
			for (i = n_x * n_y - 1; i >= 0 && c != 0 && remaining != 0; i--) 
				if (*(weights + i) + c >= 0) {
					*(weights + i) += c;
					remaining -= c;
					if ((0 < remaining && remaining < c) ||
							(0 > remaining && remaining > c))
						c = remaining;
				}
	}
}

static int *make_filter_table(PixopsFilter *filter)
{
	int i_offset, j_offset;
	int n_x = filter->x.n;
	int n_y = filter->y.n;
	int *weights = e_calloc(sizeof(int), SUBSAMPLE * SUBSAMPLE * n_x * n_y);

	for (i_offset = 0; i_offset < SUBSAMPLE; i_offset++)
		for (j_offset = 0; j_offset < SUBSAMPLE; j_offset++) {
			double weight;
			int *pixel_weights = weights + ((i_offset*SUBSAMPLE) + j_offset) * n_x * n_y;
			int total = 0;
			int i, j;

			for (i = 0; i < n_y; i++)
				for (j = 0; j < n_x; j++) {
					weight = filter->x.weights[(j_offset * n_x) + j] *
						filter->y.weights[(i_offset * n_y) + i] *
						filter->overall_alpha * 65536 + 0.5;

					total += (int)weight;

					*(pixel_weights + n_x * i + j) = (int)weight;
				}

			correct_total(pixel_weights, n_x, n_y, total, filter->overall_alpha);
		}

	return weights;
}

static void
pixops_process(euchar         *dest_buf,
		int             render_x0,
		int             render_y0,
		int             render_x1,
		int             render_y1,
		int             dest_rowstride,
		int             dest_channels,
		ebool            dest_has_alpha,
		const euchar *src_buf,
		int             src_width,
		int             src_height,
		int             src_rowstride,
		int             src_channels,
		ebool            src_has_alpha,
		double          scale_x,
		double          scale_y,
		int             check_x,
		int             check_y,
		int             check_size,
		euint32       color1,
		euint32       color2,
		PixopsFilter   *filter,
		PixopsLineFunc  line_func,
		PixopsPixelFunc pixel_func)
{
	int i, j;
	int x, y;

	euchar **line_bufs = e_calloc(sizeof(euchar *), filter->y.n);
	int *filter_weights = make_filter_table(filter);

	int x_step = (int)((1 << SCALE_SHIFT) / scale_x);
	int y_step = (int)((1 << SCALE_SHIFT) / scale_y);

	int check_shift = check_size ? get_check_shift(check_size) : 0;

	int scaled_x_offset = (int)floor(filter->x.offset * (1 << SCALE_SHIFT));

#define MYDIV(a,b) ((a) > 0 ? (a) / (b) : ((a) - (b) + 1) / (b))

	int run_end_x = (((src_width - filter->x.n + 1) << SCALE_SHIFT) - scaled_x_offset);
	int run_end_index = MYDIV(run_end_x + x_step - 1, x_step) - render_x0;
	run_end_index = MIN(run_end_index, render_x1 - render_x0);

	y = (int)(render_y0 * y_step + floor(filter->y.offset * (1 << SCALE_SHIFT)));
	for (i = 0; i < (render_y1 - render_y0); i++) {
		int dest_x;
		int y_start = y >> SCALE_SHIFT;
		int x_start;
		int *run_weights = filter_weights +
			((y >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) *
			filter->x.n * filter->y.n * SUBSAMPLE;
		euchar *new_outbuf;
		euint32 tcolor1, tcolor2;

		euchar *outbuf = dest_buf + dest_rowstride * i;
		euchar *outbuf_end = outbuf + dest_channels * (render_x1 - render_x0);

		if (((i + check_y) >> check_shift) & 1) {
			tcolor1 = color2;
			tcolor2 = color1;
		}
		else {
			tcolor1 = color1;
			tcolor2 = color2;
		}

		for (j = 0; j < filter->y.n; j++) {
			if (y_start <  0)
				line_bufs[j] = (euchar *)src_buf;
			else if (y_start < src_height)
				line_bufs[j] = (euchar *)src_buf + src_rowstride * y_start;
			else
				line_bufs[j] = (euchar *)src_buf + src_rowstride * (src_height - 1);

			y_start++;
		}

		dest_x = check_x;
		x = render_x0 * x_step + scaled_x_offset;
		x_start = x >> SCALE_SHIFT;

		while (x_start < 0 && outbuf < outbuf_end) {
			process_pixel(run_weights +
					((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) *
					(filter->x.n * filter->y.n), filter->x.n, filter->y.n,
					outbuf, dest_x, dest_channels, dest_has_alpha,
					line_bufs, src_channels, src_has_alpha,
					x >> SCALE_SHIFT, src_width,
					check_size, tcolor1, tcolor2, pixel_func);

			x += x_step;
			x_start = x >> SCALE_SHIFT;
			dest_x++;
			outbuf += dest_channels;
		}

		new_outbuf = line_func(run_weights, filter->x.n, filter->y.n,
				outbuf, dest_x, dest_buf + dest_rowstride *
				i + run_end_index * dest_channels,
				dest_channels, dest_has_alpha,
				line_bufs, src_channels, src_has_alpha,
				x, x_step, src_width, check_size, tcolor1,
				tcolor2);

		dest_x += (new_outbuf - outbuf) / dest_channels;

		x = (dest_x - check_x + render_x0) * x_step + scaled_x_offset;
		outbuf = new_outbuf;

		while (outbuf < outbuf_end) {
			process_pixel(run_weights +
					((x >> (SCALE_SHIFT - SUBSAMPLE_BITS)) & SUBSAMPLE_MASK) *
				   	(filter->x.n * filter->y.n), filter->x.n, filter->y.n,
					outbuf, dest_x, dest_channels, dest_has_alpha,
					line_bufs, src_channels, src_has_alpha,
					x >> SCALE_SHIFT, src_width,
					check_size, tcolor1, tcolor2, pixel_func);

			x += x_step;
			dest_x++;
			outbuf += dest_channels;
		}

		y += y_step;
	}

	e_free(line_bufs);
	e_free(filter_weights);
}

static void tile_make_weights(PixopsFilterDimension *dim, double scale)
{
	int n = (int)ceil(1 / scale + 1);
	double *pixel_weights = e_calloc(sizeof(double), SUBSAMPLE * n);
	int offset;
	int i;

	dim->n = n;
	dim->offset = 0;
	dim->weights = pixel_weights;

	for (offset = 0; offset < SUBSAMPLE; offset++) {
		double x = (double)offset / SUBSAMPLE;
		double a = x + 1 / scale;

		for (i = 0; i < n; i++) {
			if (i < x) {
				if (i + 1 > x)
					*(pixel_weights++) = (MIN(i + 1, a) - x) * scale;
				else
					*(pixel_weights++) = 0;
			}
			else {
				if (a > i)
					*(pixel_weights++) = (MIN(i + 1, a) - i) * scale;
				else
					*(pixel_weights++) = 0;
			}
		}
	}
}

static void
bilinear_magnify_make_weights(PixopsFilterDimension *dim, double scale)
{
	double *pixel_weights;
	int n;
	int offset;
	int i;

	if (scale > 1.0) {
		n = 2;
		dim->offset = 0.5 * (1 / scale - 1);
	}
	else {
		n = (int)ceil(1.0 + 1.0 / scale);
		dim->offset = 0.0;
	}

	dim->n = n;
	dim->weights = e_calloc(sizeof(double), SUBSAMPLE * n);

	pixel_weights = dim->weights;

	for (offset=0; offset < SUBSAMPLE; offset++) {
		double x = (double)offset / SUBSAMPLE;

		if (scale > 1.0) {
			for (i = 0; i < n; i++)
				*(pixel_weights++) = (((i == 0) ? (1 - x) : x) / scale) * scale;
		}
		else {
			double a = x + 1 / scale;

			for (i = 0; i < n; i++) {
				if (i < x) {
					if (i + 1 > x)
						*(pixel_weights++) = (MIN(i + 1, a) - x) * scale;
					else
						*(pixel_weights++) = 0;
				}
				else {
					if (a > i)
						*(pixel_weights++) = (MIN(i + 1, a) - i) * scale;
					else
						*(pixel_weights++) = 0;
				}
			}
		}
	}
}

static double linear_box_half(double b0, double b1)
{
	double a0, a1;
	double x0, x1;

	a0 = 0.;
	a1 = 1.;

	if (a0 < b0) {
		if (a1 > b0) {
			x0 = b0;
			x1 = MIN (a1, b1);
		}
		else
			return 0;
	}
	else {
		if (b1 > a0) {
			x0 = a0;
			x1 = MIN (a1, b1);
		}
		else
			return 0;
	}

	return 0.5 * (x1*x1 - x0*x0);
}

static void
bilinear_box_make_weights(PixopsFilterDimension *dim, double scale)
{
	int n = (int)ceil (1 / scale + 3.0);
	double *pixel_weights = e_calloc(sizeof(double), SUBSAMPLE * n);
	double w;
	int offset, i;

	dim->offset = -1.0;
	dim->n = n;
	dim->weights = pixel_weights;

	for (offset = 0; offset < SUBSAMPLE; offset++) {
		double x = (double)offset / SUBSAMPLE;
		double a = x + 1 / scale;

		for (i = 0; i < n; i++) {
			w  = linear_box_half(0.5 + i - a, 0.5 + i - x);
			w += linear_box_half(1.5 + x - i, 1.5 + a - i);

			*(pixel_weights++) = w * scale;
		}
	}
}

static void
make_weights(PixopsFilter     *filter,
		PixopsInterpType  interp_type,	      
		double            scale_x,
		double            scale_y)
{
	switch (interp_type)
	{
		case PIXOPS_INTERP_NEAREST:
			//g_assert_not_reached();
			break;

		case PIXOPS_INTERP_TILES:
			tile_make_weights(&filter->x, scale_x);
			tile_make_weights(&filter->y, scale_y);
			break;

		case PIXOPS_INTERP_BILINEAR:
			bilinear_magnify_make_weights(&filter->x, scale_x);
			bilinear_magnify_make_weights(&filter->y, scale_y);
			break;

		case PIXOPS_INTERP_HYPER:
			bilinear_box_make_weights(&filter->x, scale_x);
			bilinear_box_make_weights(&filter->y, scale_y);
			break;
	}
}

static void
_pixops_composite_color_real(euchar          *dest_buf,
		int              render_x0,
		int              render_y0,
		int              render_x1,
		int              render_y1,
		int              dest_rowstride,
		int              dest_channels,
		ebool         dest_has_alpha,
		const euchar    *src_buf,
		int              src_width,
		int              src_height,
		int              src_rowstride,
		int              src_channels,
		ebool         src_has_alpha,
		double           scale_x,
		double           scale_y,
		PixopsInterpType interp_type,
		int              overall_alpha,
		int              check_x,
		int              check_y,
		int              check_size,
		euint32          color1,
		euint32          color2)
{
	PixopsFilter filter;
	PixopsLineFunc line_func;

	if (dest_channels == 3 && dest_has_alpha)
		return;
	if (src_channels == 3 && src_has_alpha)
		return;

	if (scale_x == 0 || scale_y == 0)
		return;

	if (interp_type == PIXOPS_INTERP_NEAREST) {
		pixops_composite_color_nearest(dest_buf, render_x0, render_y0,
				render_x1, render_y1, dest_rowstride,
				dest_channels, dest_has_alpha, src_buf,
				src_width, src_height, src_rowstride,
				src_channels, src_has_alpha, scale_x,
				scale_y, overall_alpha, check_x, check_y,
				check_size, color1, color2);
		return;
	}

	filter.overall_alpha = overall_alpha / 255.;
	make_weights(&filter, interp_type, scale_x, scale_y);

	line_func = composite_line_color;

	pixops_process(dest_buf, render_x0, render_y0, render_x1, render_y1,
			dest_rowstride, dest_channels, dest_has_alpha,
			src_buf, src_width, src_height, src_rowstride, src_channels,
			src_has_alpha, scale_x, scale_y, check_x, check_y, check_size, color1, color2,
			&filter, line_func, composite_pixel_color);

	e_free(filter.x.weights);
	e_free(filter.y.weights);
}

void _pixops_composite_color(euchar       *dest_buf,
							int              dest_width,
							int              dest_height,
							int              dest_rowstride,
							int              dest_channels,
							ebool             dest_has_alpha,
							const euchar  *src_buf,
							int              src_width,
							int              src_height,
							int              src_rowstride,
							int              src_channels,
							ebool             src_has_alpha,
							int              dest_x,
							int              dest_y,
							int              dest_region_width,
							int              dest_region_height,
							double           offset_x,
							double           offset_y,
							double           scale_x,
							double           scale_y,
							PixopsInterpType interp_type,
							int              overall_alpha,
							int              check_x,
							int              check_y,
							int              check_size,
							euint32        color1,
							euint32        color2)
{
	euchar *new_dest_buf;
	int render_x0;
	int render_y0;
	int render_x1;
	int render_y1;

	if (!src_has_alpha && overall_alpha == 255) {
		_pixops_scale(dest_buf, dest_width, dest_height, dest_rowstride,
				dest_channels, dest_has_alpha, src_buf, src_width,
				src_height, src_rowstride, src_channels, src_has_alpha,
				dest_x, dest_y, dest_region_width, dest_region_height,
				offset_x, offset_y, scale_x, scale_y, interp_type);
		return;
	}

	new_dest_buf = dest_buf + dest_y * dest_rowstride + dest_x * dest_channels;
	render_x0 = (int)(dest_x - offset_x);
	render_y0 = (int)(dest_y - offset_y);
	render_x1 = (int)(dest_x + dest_region_width  - offset_x);
	render_y1 = (int)(dest_y + dest_region_height - offset_y);

	_pixops_composite_color_real(new_dest_buf, render_x0, render_y0, render_x1,
			render_y1, dest_rowstride, dest_channels,
			dest_has_alpha, src_buf, src_width,
			src_height, src_rowstride, src_channels,
			src_has_alpha, scale_x, scale_y,
			(PixopsInterpType)interp_type, overall_alpha,
			check_x, check_y, check_size, color1, color2);
}

static void
_pixops_composite_real(euchar          *dest_buf,
		int              render_x0,
		int              render_y0,
		int              render_x1,
		int              render_y1,
		int              dest_rowstride,
		int              dest_channels,
		ebool         dest_has_alpha,
		const euchar    *src_buf,
		int              src_width,
		int              src_height,
		int              src_rowstride,
		int              src_channels,
		ebool         src_has_alpha,
		double           scale_x,
		double           scale_y,
		PixopsInterpType interp_type,
		int              overall_alpha)
{
	PixopsFilter filter;
	PixopsLineFunc line_func;

	if (dest_channels == 3 && dest_has_alpha)
		return;
	if (src_channels == 3 && src_has_alpha)
		return;

	if (scale_x == 0 || scale_y == 0)
		return;

	if (interp_type == PIXOPS_INTERP_NEAREST) {
		pixops_composite_nearest(dest_buf, render_x0, render_y0, render_x1,
				render_y1, dest_rowstride, dest_channels,
				dest_has_alpha, src_buf, src_width, src_height,
				src_rowstride, src_channels, src_has_alpha,
				scale_x, scale_y, overall_alpha);
		return;
	}

	filter.overall_alpha = overall_alpha / 255.;
	make_weights(&filter, interp_type, scale_x, scale_y);

	if (filter.x.n == 2 && filter.y.n == 2 && dest_channels == 4 &&
			src_channels == 4 && src_has_alpha && !dest_has_alpha)
		line_func = composite_line_22_4a4;
	else
		line_func = composite_line;

	pixops_process(dest_buf, render_x0, render_y0, render_x1, render_y1,
			dest_rowstride, dest_channels, dest_has_alpha,
			src_buf, src_width, src_height, src_rowstride, src_channels,
			src_has_alpha, scale_x, scale_y, 0, 0, 0, 0, 0, 
			&filter, line_func, composite_pixel);

	e_free(filter.x.weights);
	e_free(filter.y.weights);
}

void
_pixops_composite(euchar     *dest_buf,
		int              dest_width,
		int              dest_height,
		int              dest_rowstride,
		int              dest_channels,
		ebool             dest_has_alpha,
		const euchar    *src_buf,
		int              src_width,
		int              src_height,
		int              src_rowstride,
		int              src_channels,
		ebool             src_has_alpha,
		int              dest_x,
		int              dest_y,
		int              dest_region_width,
		int              dest_region_height,
		double           offset_x,
		double           offset_y,
		double           scale_x,
		double           scale_y,
		PixopsInterpType interp_type,
		int              overall_alpha)
{
	euchar *new_dest_buf;
	int render_x0;
	int render_y0;
	int render_x1;
	int render_y1;

	if (dest_region_width + dest_x > dest_width)
		dest_region_width = dest_width - dest_x;
	if (dest_region_height + dest_y > dest_height)
		dest_region_height = dest_height - dest_y;

	if (!src_has_alpha && overall_alpha == 255) {
		_pixops_scale(dest_buf, dest_width, dest_height, dest_rowstride,
				dest_channels, dest_has_alpha, src_buf, src_width,
				src_height, src_rowstride, src_channels, src_has_alpha,
				dest_x, dest_y, dest_region_width, dest_region_height,
				offset_x, offset_y, scale_x, scale_y, interp_type);
		return;
	}

	new_dest_buf = dest_buf + dest_y * dest_rowstride + dest_x * dest_channels;
	render_x0 = (int)offset_x;
	render_y0 = (int)offset_y;
	render_x1 = (int)(dest_region_width  + offset_x);
	render_y1 = (int)(dest_region_height + offset_y);

	_pixops_composite_real(new_dest_buf, render_x0, render_y0, render_x1,
			render_y1, dest_rowstride, dest_channels,
			dest_has_alpha, src_buf, src_width, src_height,
			src_rowstride, src_channels, src_has_alpha, scale_x,
			scale_y, (PixopsInterpType)interp_type,
			overall_alpha);
}

static void
_pixops_scale_real(euchar        *dest_buf,
		int            render_x0,
		int            render_y0,
		int            render_x1,
		int            render_y1,
		int            dest_rowstride,
		int            dest_channels,
		ebool       dest_has_alpha,
		const euchar  *src_buf,
		int            src_width,
		int            src_height,
		int            src_rowstride,
		int            src_channels,
		ebool       src_has_alpha,
		double         scale_x,
		double         scale_y,
		PixopsInterpType  interp_type)
{
	PixopsFilter filter;
	PixopsLineFunc line_func;

	if (dest_channels == 3 && dest_has_alpha)
		return;
	if (src_channels == 3 && src_has_alpha)
		return;
	if (src_has_alpha && !dest_has_alpha)
		return;

	if (scale_x == 0 || scale_y == 0)
		return;

	if (interp_type == PIXOPS_INTERP_NEAREST) {
		pixops_scale_nearest(dest_buf, render_x0, render_y0, render_x1,
				render_y1, dest_rowstride, dest_channels,
				dest_has_alpha, src_buf, src_width, src_height,
				src_rowstride, src_channels, src_has_alpha,
				scale_x, scale_y);
		return;
	}

	filter.overall_alpha = 1.0;
	make_weights(&filter, interp_type, scale_x, scale_y);

	if (filter.x.n == 2 && filter.y.n == 2 && dest_channels == 3 && src_channels == 3)
		line_func = scale_line_22_33;
	else
		line_func = scale_line;

	pixops_process(dest_buf, render_x0, render_y0, render_x1, render_y1,
			dest_rowstride, dest_channels, dest_has_alpha,
			src_buf, src_width, src_height, src_rowstride, src_channels,
			src_has_alpha, scale_x, scale_y, 0, 0, 0, 0, 0,
			&filter, line_func, scale_pixel);

	e_free(filter.x.weights);
	e_free(filter.y.weights);
}

void
_pixops_scale(euchar          *dest_buf,
		int              dest_width,
		int              dest_height,
		int              dest_rowstride,
		int              dest_channels,
		int              dest_has_alpha,
		const euchar    *src_buf,
		int              src_width,
		int              src_height,
		int              src_rowstride,
		int              src_channels,
		int              src_has_alpha,
		int              dest_x,
		int              dest_y,
		int              dest_region_width,
		int              dest_region_height,
		double           offset_x,
		double           offset_y,
		double           scale_x,
		double           scale_y,
		PixopsInterpType interp_type)
{
	euchar *new_dest_buf;
	int render_x0;
	int render_y0;
	int render_x1;
	int render_y1;

	new_dest_buf = dest_buf + dest_y * dest_rowstride + dest_x * dest_channels;
	render_x0    = (int)offset_x;
	render_y0    = (int)offset_y;
	render_x1    = (int)(dest_region_width  + offset_x);
	render_y1    = (int)(dest_region_height + offset_y);

	_pixops_scale_real(new_dest_buf, render_x0, render_y0, render_x1,
			render_y1, dest_rowstride, dest_channels,
			dest_has_alpha, src_buf, src_width, src_height,
			src_rowstride, src_channels, src_has_alpha,
			scale_x, scale_y, (PixopsInterpType)interp_type);
}

