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
                                               gint        width,
                                               gint        height,
                                               gint        position_x,
                                               gint        position_y,
                                               gint        square_size);
void        npd_draw_model_into_image         (NPDModel   *model,
                                               NPDImage   *image);
void        npd_draw_mesh                     (NPDModel   *model,
                                               NPDDisplay *display);
gboolean    npd_is_color_transparent          (NPDColor *color);
void      (*npd_draw_line)                    (NPDDisplay *display,
                                               gfloat      x0,
                                               gfloat      y0,
                                               gfloat      x1,
                                               gfloat      y1);
void      (*npd_get_pixel_color)              (NPDImage   *image,
                                               gint        x,
                                               gint        y,
                                               NPDColor   *color);
void      (*npd_set_pixel_color)              (NPDImage   *image,
                                               gint        x,
                                               gint        y,
                                               NPDColor   *color);

#endif	/*__NPD_GRAPHICS_H__ */
