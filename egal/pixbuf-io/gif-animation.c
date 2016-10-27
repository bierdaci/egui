#include <config.h>
#include <errno.h>

static void pixbuf_gif_anim_class_init(GalPixbufGifAnimClass *klass);
static void pixbuf_gif_anim_finalize(GObject *object);

static ebool pixbuf_gif_anim_is_static_image(GalPixbufAnimation *animation);
static GalPixbuf* pixbuf_gif_anim_get_static_image(GalPixbufAnimation *animation);

static void pixbuf_gif_anim_get_size(GalPixbufAnimation *anim, int *width, int *height);
static GalPixbufAnimationIter* pixbuf_gif_anim_get_iter(GalPixbufAnimation *anim,
		const GTimeVal     *start_time);


static ePointer parent_class;

eGeneType pixbuf_gif_anim_get_type(void)
{
	static eGeneType gtype = 0;

	if (!gtype) {
		GeneInfo info = {
			sizeof(GalPixbufGifAnim),
			0,
			pixbuf_gif_anim_init_data,
			pixbuf_gif_anim_free_data,
			pixbuf_gif_anim_init_orders,
		};

		gtype = e_register_gene_type(&info, GTYPE_PIXBUF_ANIMATION, NULL);
	}
	return gtype;
}

static eint pixbuf_gif_anim_init_data(Cell *cell, eValist ap)
{
	return 0;
}

static void pixbuf_gif_anim_init_gene(Gene *new, void *this)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GalPixbufAnimationClass *anim_class = GDK_PIXBUF_ANIMATION_CLASS(klass);

	parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = egal_pixbuf_gif_anim_finalize;

	anim_class->is_static_image = pixbuf_gif_anim_is_static_image;
	anim_class->get_static_image = pixbuf_gif_anim_get_static_image;
	anim_class->get_size = pixbuf_gif_anim_get_size;
	anim_class->get_iter = pixbuf_gif_anim_get_iter;
}

static void pixbuf_gif_anim_finalize(GObject *object)
{
	GalPixbufGifAnim *gif_anim = GDK_PIXBUF_GIF_ANIM(object);

	GList *l;
	GalPixbufFrame *frame;

	for (l = gif_anim->frames; l; l = l->next) {
		frame = l->data;
		e_pixbuf_free(frame->pixbuf);
		if (frame->composited)
			e_pixbuf_free(frame->composited);
		if (frame->revert)
			e_pixbuf_free(frame->revert);
		g_free(frame);
	}

	g_list_free(gif_anim->frames);

	G_OBJECT_CLASS(parent_class)->finalize (object);
}

static ebool pixbuf_gif_anim_is_static_image(GalPixbufAnimation *animation)
{
	GalPixbufGifAnim *gif_anim;

	gif_anim = GDK_PIXBUF_GIF_ANIM(animation);

	return (gif_anim->frames != NULL && gif_anim->frames->next == NULL);
}

static GalPixbuf* pixbuf_gif_anim_get_static_image(GalPixbufAnimation *animation)
{
	GalPixbufGifAnim *gif_anim;

	gif_anim = GDK_PIXBUF_GIF_ANIM(animation);

	if (gif_anim->frames == NULL)
		return NULL;
	else
		return GDK_PIXBUF(((GalPixbufFrame*)gif_anim->frames->data)->pixbuf);        
}

static void
pixbuf_gif_anim_get_size(GalPixbufAnimation *anim, int *width, int *height)
{
	GalPixbufGifAnim *gif_anim;

	gif_anim = GDK_PIXBUF_GIF_ANIM(anim);

	if (width)
		*width = gif_anim->width;

	if (height)
		*height = gif_anim->height;
}

static void iter_clear(GalPixbufGifAnimIter *iter)
{
	iter->current_frame = NULL;
}

static void iter_restart(GalPixbufGifAnimIter *iter)
{
	iter_clear(iter);

	iter->current_frame = iter->gif_anim->frames;
}

static GalPixbufAnimationIter*
pixbuf_gif_anim_get_iter(GalPixbufAnimation *anim, const GTimeVal *start_time)
{
	GalPixbufGifAnimIter *iter;

	iter = g_object_new(GDK_TYPE_PIXBUF_GIF_ANIM_ITER, NULL);

	iter->gif_anim = GDK_PIXBUF_GIF_ANIM(anim);

	g_object_ref(iter->gif_anim);

	iter_restart(iter);

	iter->start_time = *start_time;
	iter->current_time = *start_time;
	iter->first_loop_slowness = 0;

	return GDK_PIXBUF_ANIMATION_ITER(iter);
}

static void pixbuf_gif_anim_iter_class_init(GalPixbufGifAnimIterClass *klass);
static void pixbuf_gif_anim_iter_finalize(GObject *object);

static int pixbuf_gif_anim_iter_get_delay_time(GalPixbufAnimationIter *iter);
static GalPixbuf* pixbuf_gif_anim_iter_get_pixbuf(GalPixbufAnimationIter *iter);
static ebool pixbuf_gif_anim_iter_on_currently_loading_frame(GalPixbufAnimationIter *iter);
static ebool pixbuf_gif_anim_iter_advance(GalPixbufAnimationIter *iter, const GTimeVal *current_time);

static ePointer iter_parent_class;

GType pixbuf_gif_anim_iter_get_type(void)
{
	static GType object_type = 0;

	if (!object_type) {
		static const GTypeInfo object_info = {
			sizeof(GalPixbufGifAnimIterClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) egal_pixbuf_gif_anim_iter_class_init,
			NULL,           /* class_finalize */
			NULL,           /* class_data */
			sizeof (GalPixbufGifAnimIter),
			0,              /* n_preallocs */
			(GInstanceInitFunc) NULL,
		};

		object_type = g_type_register_static(GDK_TYPE_PIXBUF_ANIMATION_ITER,
				g_intern_static_string ("GalPixbufGifAnimIter"),
				&object_info, 0);
	}

	return object_type;
}

static void pixbuf_gif_anim_iter_class_init(GalPixbufGifAnimIterClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GalPixbufAnimationIterClass *anim_iter_class =
		GDK_PIXBUF_ANIMATION_ITER_CLASS(klass);

	iter_parent_class = g_type_class_peek_parent(klass);

	object_class->finalize = egal_pixbuf_gif_anim_iter_finalize;

	anim_iter_class->advance                    = egal_pixbuf_gif_anim_iter_advance;
	anim_iter_class->get_pixbuf                 = egal_pixbuf_gif_anim_iter_get_pixbuf;
	anim_iter_class->get_delay_time             = egal_pixbuf_gif_anim_iter_get_delay_time;
	anim_iter_class->on_currently_loading_frame = egal_pixbuf_gif_anim_iter_on_currently_loading_frame;
}

static void pixbuf_gif_anim_iter_finalize(GObject *object)
{
	GalPixbufGifAnimIter *iter = GDK_PIXBUF_GIF_ANIM_ITER(object);

	iter_clear(iter);

	e_pixbuf_free(iter->gif_anim);

	G_OBJECT_CLASS(iter_parent_class)->finalize(object);
}

static ebool pixbuf_gif_anim_iter_advance(
		GalPixbufAnimationIter *anim_iter, const GTimeVal *current_time)
{
	GalPixbufGifAnimIter *iter;
	gint elapsed;
	gint loop;
	GList *tmp;
	GList *old;

	iter = GDK_PIXBUF_GIF_ANIM_ITER(anim_iter);

	iter->current_time = *current_time;

	elapsed =
		(((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
		  iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;

	if (elapsed < 0) {
		iter->start_time = iter->current_time;
		elapsed = 0;
	}

	e_assert(iter->gif_anim->total_time > 0);

	if (iter->gif_anim->loading)
		loop = 0;
	else {
		if (iter->current_frame == NULL)
			iter->first_loop_slowness = MAX(0, elapsed - iter->gif_anim->total_time);

		loop = (elapsed - iter->first_loop_slowness) / iter->gif_anim->total_time;
		elapsed = (elapsed - iter->first_loop_slowness) % iter->gif_anim->total_time;
	}

	iter->position = elapsed;

	/* Now move to the proper frame */
	if (iter->gif_anim->loop == 0 || loop < iter->gif_anim->loop) 
		tmp = iter->gif_anim->frames;
	else 
		tmp = NULL;
	while (tmp != NULL) {
		GalPixbufFrame *frame = tmp->data;

		if (iter->position >= frame->elapsed &&
				iter->position < (frame->elapsed + frame->delay_time))
			break;

		tmp = tmp->next;
	}

	old = iter->current_frame;

	iter->current_frame = tmp;

	return iter->current_frame != old;
}

int pixbuf_gif_anim_iter_get_delay_time(GalPixbufAnimationIter *anim_iter)
{
	GalPixbufFrame *frame;
	GalPixbufGifAnimIter *iter;

	iter = GDK_PIXBUF_GIF_ANIM_ITER(anim_iter);

	if (iter->current_frame) {
		frame = iter->current_frame->data;
		return frame->delay_time - (iter->position - frame->elapsed);
	} else 
		return -1;
}

void pixbuf_gif_anim_frame_composite(GalPixbufGifAnim *gif_anim, GalPixbufFrame *frame)
{  
	GalPixbufFrame *t = frame;

	if (frame->need_recomposite || frame->composited == NULL) {
		while (t) {
			if (t->need_recomposite) {
				if (t->composited) {
					e_pixbuf_free(t->composited);
					t->composited = NULL;
				}
			}

			if (t->composited)
				break;

			t = t->prev;
		}

		/* Go forward, compositing all frames up to the current frame */
		if (t == NULL)
			t = gif_anim->frames;

		while (t) {
			gint clipped_width, clipped_height;

			if (!t->pixbuf) return;

			clipped_width = MIN(gif_anim->width - t->x_offset, egal_pixbuf_get_width(t->pixbuf));
			clipped_height = MIN(gif_anim->height - t->y_offset, egal_pixbuf_get_height(t->pixbuf));

			if (t->need_recomposite) {
				if (t->composited) {
					e_pixbuf_free(t->composited);
					t->composited = NULL;
				}
			}

			if (t->composited)
				goto next;

			if (t->prev == NULL) {
				t->composited = egal_pixbuf_new(etrue, gif_anim->width, gif_anim->height);

				if (t->composited == NULL)
					return;

				egal_pixbuf_fill(t->composited,
						(gif_anim->bg_red << 24) |
						(gif_anim->bg_green << 16) |
						(gif_anim->bg_blue << 8));

				if (clipped_width > 0 && clipped_height > 0)
					egal_pixbuf_composite(
							t->composited,
							t->x_offset, t->y_offset,
							clipped_width, clipped_height,
							t->pixbuf,
							t->x_offset, t->y_offset,
							1.0, 1.0,
							GDK_INTERP_BILINEAR,
							255);

				if (t->action == GDK_PIXBUF_FRAME_REVERT)
					e_warning("First frame of GIF has bad dispose mode, GIF loader should not have loaded this image");

				t->need_recomposite = FALSE;
			}
			else {
				GalPixbufFrame *prev_frame;
				gint prev_clipped_width;
				gint prev_clipped_height;

				prev_frame = t->prev;

				prev_clipped_width = MIN(gif_anim->width - prev_frame->x_offset, egal_pixbuf_get_width (prev_frame->pixbuf));
				prev_clipped_height = MIN(gif_anim->height - prev_frame->y_offset, egal_pixbuf_get_height (prev_frame->pixbuf));

				/* Init t->composited with what we should have after the previous
				 * frame
				 */

				if (prev_frame->action == GDK_PIXBUF_FRAME_RETAIN) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;
				}
				else if (prev_frame->action == GDK_PIXBUF_FRAME_DISPOSE) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;

					if (prev_clipped_width > 0 && prev_clipped_height > 0) {
						/* Clear area of previous frame to background */
						GalPixbuf *area;

						area = egal_pixbuf_new_subpixbuf(t->composited,
								prev_frame->x_offset,
								prev_frame->y_offset,
								prev_clipped_width,
								prev_clipped_height);

						if (area == NULL)
							return;

						egal_pixbuf_fill(area,
								(gif_anim->bg_red << 24) |
								(gif_anim->bg_green << 16) |
								(gif_anim->bg_blue << 8));
						e_pixbuf_free(area);
					}
				}
				else if (prev_frame->action == GDK_PIXBUF_FRAME_REVERT) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;

					if (prev_frame->revert != NULL && prev_clipped_width > 0 && prev_clipped_height > 0) {
						/* Copy in the revert frame */
						egal_pixbuf_copy_area(prev_frame->revert,
								0, 0,
								egal_pixbuf_get_width(prev_frame->revert),
								egal_pixbuf_get_height(prev_frame->revert),
								t->composited,
								prev_frame->x_offset,
								prev_frame->y_offset);
					}
				}
				else {
					e_warning("Unknown revert action for GIF frame");
				}

				if (t->revert == NULL && t->action == GDK_PIXBUF_FRAME_REVERT) {
					if (clipped_width > 0 && clipped_height > 0) {
						/* We need to save the contents before compositing */
						GalPixbuf *area;

						area = egal_pixbuf_new_subpixbuf(t->composited,
								t->x_offset,
								t->y_offset,
								clipped_width,
								clipped_height);

						if (area == NULL)
							return;

						t->revert = egal_pixbuf_copy(area);

						e_pixbuf_free(area);

						if (t->revert == NULL)
							return;
					}
				}

				if (clipped_width > 0 && clipped_height > 0 &&
						t->pixbuf != NULL && t->composited != NULL) {
					egal_pixbuf_composite(
							t->composited,
							t->x_offset, t->y_offset,
							clipped_width, clipped_height,
							t->pixbuf,
							t->x_offset, t->y_offset,
							1.0, 1.0,
							GDK_INTERP_NEAREST,
							255);
				}

				t->need_recomposite = FALSE;
			}
next:
			if (t == frames)
				break;
			t = t->next;
		}
	}
}

GalPixbuf* pixbuf_gif_anim_iter_get_pixbuf(GalPixbufAnimationIter *anim_iter)
{
	GalPixbufGifAnimIter *iter;
	GalPixbufFrame *frame;

	iter = GDK_PIXBUF_GIF_ANIM_ITER(anim_iter);

	frame = iter->current_frame ? iter->current_frame->data : g_list_last (iter->gif_anim->frames)->data;

	if (frame == NULL)
		return NULL;

	pixbuf_gif_anim_frame_composite(iter->gif_anim, frame);

	return frame->composited;
}

static ebool pixbuf_gif_anim_iter_on_currently_loading_frame(GalPixbufAnimationIter *anim_iter)
{
	GalPixbufGifAnimIter *iter;

	iter = GDK_PIXBUF_GIF_ANIM_ITER(anim_iter);

	return iter->current_frame == NULL || iter->current_frame->next == NULL;  
}

void pixbuf_gif_anim_frame_composite(GalPixbufGifAnim *gif_anim, GalPixbufFrame *frame)
{  
	GalPixbufFrame *t = frame;

	if (frame->need_recomposite || frame->composited == NULL) {
		while (t) {
			if (t->need_recomposite) {
				if (t->composited) {
					egal_pixbuf_free(t->composited);
					t->composited = NULL;
				}
			}

			if (t->composited)
				break;

			t = t->prev;
		}

		/* Go forward, compositing all frames up to the current frame */
		if (t == NULL)
			t = gif_anim->frames;

		while (t) {
			eint clipped_width, clipped_height;

			if (!t->pixbuf) return;

			clipped_width = MIN(gif_anim->width - t->x_offset, t->pixbuf->w);
			clipped_height = MIN(gif_anim->height - t->y_offset, t->pixbuf->h);

			if (t->need_recomposite) {
				if (t->composited) {
					egal_pixbuf_free(t->composited);
					t->composited = NULL;
				}
			}

			if (t->composited)
				goto next;

			if (t->prev == NULL) {
				t->composited = egal_pixbuf_new(etrue, gif_anim->width, gif_anim->height);

				if (t->composited == NULL)
					return;

				egal_pixbuf_fill(t->composited,
						(gif_anim->bg_red << 24) |
						(gif_anim->bg_green << 16) |
						(gif_anim->bg_blue << 8));

				if (clipped_width > 0 && clipped_height > 0)
					egal_pixbuf_composite(
							t->composited,
							t->x_offset, t->y_offset,
							clipped_width, clipped_height,
							t->pixbuf,
							t->x_offset, t->y_offset,
							1.0, 1.0,
							PIXOPS_INTERP_BILINEAR);

				if (t->action == GAL_PIXBUF_FRAME_REVERT)
					printf("First frame of GIF has bad dispose mode, GIF loader should not have loaded this image\n");

				t->need_recomposite = efalse;
			}
			else {
				GalPixbufFrame *prev_frame;
				eint prev_clipped_width;
				eint prev_clipped_height;

				prev_frame = t->prev;

				prev_clipped_width  = MIN(gif_anim->width  - prev_frame->x_offset, prev_frame->pixbuf->w);
				prev_clipped_height = MIN(gif_anim->height - prev_frame->y_offset, prev_frame->pixbuf->h);

				/* Init t->composited with what we should have after the previous
				 * frame
				 */

				if (prev_frame->action == GAL_PIXBUF_FRAME_RETAIN) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;
				}
				else if (prev_frame->action == GAL_PIXBUF_FRAME_DISPOSE) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;

					if (prev_clipped_width > 0 && prev_clipped_height > 0) {
						/* Clear area of previous frame to background */
						GalPixbuf *area;

						area = egal_pixbuf_new_subpixbuf(t->composited,
								prev_frame->x_offset,
								prev_frame->y_offset,
								prev_clipped_width,
								prev_clipped_height);

						if (area == NULL)
							return;

						egal_pixbuf_fill(area,
								(gif_anim->bg_red << 24) |
								(gif_anim->bg_green << 16) |
								(gif_anim->bg_blue << 8));
						egal_pixbuf_free(area);
					}
				}
				else if (prev_frame->action == GAL_PIXBUF_FRAME_REVERT) {
					t->composited = egal_pixbuf_copy(prev_frame->composited);

					if (t->composited == NULL)
						return;

					if (prev_frame->revert != NULL && prev_clipped_width > 0 && prev_clipped_height > 0) {
						/* Copy in the revert frame */
						egal_pixbuf_copy_area(prev_frame->revert,
								0, 0,
								prev_frame->revert->w,
								prev_frame->revert->h,
								t->composited,
								prev_frame->x_offset,
								prev_frame->y_offset);
					}
				}
				else {
					printf("Unknown revert action for GIF frame\n");
				}

				if (t->revert == NULL && t->action == GAL_PIXBUF_FRAME_REVERT) {
					if (clipped_width > 0 && clipped_height > 0) {
						/* We need to save the contents before compositing */
						GalPixbuf *area;

						area = egal_pixbuf_new_subpixbuf(t->composited,
								t->x_offset,
								t->y_offset,
								clipped_width,
								clipped_height);

						if (area == NULL)
							return;

						t->revert = egal_pixbuf_copy(area);

						egal_pixbuf_free(area);

						if (t->revert == NULL)
							return;
					}
				}

				if (clipped_width > 0 && clipped_height > 0 &&
						t->pixbuf != NULL && t->composited != NULL) {
					egal_pixbuf_composite(
							t->composited,
							t->x_offset, t->y_offset,
							clipped_width, clipped_height,
							t->pixbuf,
							t->x_offset, t->y_offset,
							1.0, 1.0,
							PIXOPS_INTERP_NEAREST);
				}

				t->need_recomposite = efalse;
			}
next:
			if (t == frame)
				break;
			t = t->next;
		}
	}
}

