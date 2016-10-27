#ifndef __PIXOPS_H__
#define __PIXOPS_H__

#include <elib/elib.h>

typedef enum {
	PIXOPS_INTERP_NEAREST,
	PIXOPS_INTERP_TILES,
	PIXOPS_INTERP_BILINEAR,
	PIXOPS_INTERP_HYPER
} PixopsInterpType;

void _pixops_composite(euchar         *dest_buf,
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
                        int              overall_alpha);

void _pixops_composite_color(euchar          *dest_buf,
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
                              euint32        color2);

void _pixops_scale    (euchar          *dest_buf,
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
                       PixopsInterpType interp_type);
#endif
