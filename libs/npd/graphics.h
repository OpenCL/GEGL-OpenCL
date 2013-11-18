/*
 * This file is part of N-point image deformation library.
 *
 * N-point image deformation library is free software: you can
 * redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * N-point image deformation library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with N-point image deformation library.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#ifndef __NPD_GRAPHICS_H__
#define	__NPD_GRAPHICS_H__

#include "npd_common.h"

//#define NPD_RGBA_FLOAT

struct _NPDColor {
#ifdef NPD_RGBA_FLOAT
  gfloat r;
  gfloat g;
  gfloat b;
  gfloat a;
#else
  guint8 r;
  guint8 g;
  guint8 b;
  guint8 a;
#endif
};

typedef enum
{
  NPD_BILINEAR_INTERPOLATION = 1,
  NPD_ALPHA_BLENDING         = 1 << 1
} NPDSettings;

void        npd_create_model_from_image       (NPDModel   *model,
                                               NPDImage   *image,
                                               gint        square_size);
void        npd_create_mesh_from_image        (NPDModel   *model,
                                               gint        width,
                                               gint        height,
                                               gint        position_x,
                                               gint        position_y);
void        npd_draw_model                    (NPDModel   *model,
                                               NPDDisplay *display);
void        npd_draw_mesh                     (NPDModel   *model,
                                               NPDDisplay *display);

void        npd_destroy_image                 (NPDImage   *image);

void        npd_texture_fill_triangle         (gint        x1,
                                               gint        y1,
                                               gint        x2,
                                               gint        y2,
                                               gint        x3,
                                               gint        y3,
                                               NPDMatrix  *A,
                                               NPDImage   *input_image,
                                               NPDImage   *output_image,
                                               NPDSettings settings);
void        npd_texture_quadrilateral         (NPDBone    *reference_bone,
                                               NPDBone    *current_bone,
                                               NPDImage   *input_image,
                                               NPDImage   *output_image,
                                               NPDSettings settings);
void        npd_draw_texture_line             (gint        x1,
                                               gint        x2,
                                               gint        y,
                                               NPDMatrix  *A,
                                               NPDImage   *input_image,
                                               NPDImage   *output_image,
                                               NPDSettings settings);
void      (*npd_draw_line)                    (NPDDisplay *display,
                                               gfloat      x0,
                                               gfloat      y0,
                                               gfloat      x1,
                                               gfloat      y1);
gfloat      npd_bilinear_interpolation        (gfloat      I0,
                                               gfloat      I1,
                                               gfloat      I2,
                                               gfloat      I3,
                                               gfloat      dx,
                                               gfloat      dy);
void        npd_bilinear_color_interpolation  (NPDColor   *I0,
                                               NPDColor   *I1,
                                               NPDColor   *I2,
                                               NPDColor   *I3,
                                               gfloat      dx,
                                               gfloat      dy,
                                               NPDColor   *out);
gfloat      npd_blend_band                    (gfloat      src,
                                               gfloat      dst,
                                               gfloat      src_alpha,
                                               gfloat      dest_alpha,
                                               gfloat      out_alpha);
void        npd_blend_colors                 (NPDColor   *src,
                                               NPDColor   *dst,
                                               NPDColor   *out_color);
void      (*npd_get_pixel_color)              (NPDImage   *image,
                                               gint        x,
                                               gint        y,
                                               NPDColor   *color);
void      (*npd_set_pixel_color)              (NPDImage   *image,
                                               gint        x,
                                               gint        y,
                                               NPDColor   *color);
gboolean    npd_compare_colors                (NPDColor   *c1,
                                               NPDColor   *c2);
gboolean    npd_is_color_transparent          (NPDColor   *color);
gboolean    npd_init_display                  (NPDDisplay *display);
void        npd_destroy_display               (NPDDisplay *display);

#endif	/*__NPD_GRAPHICS_H__ */
