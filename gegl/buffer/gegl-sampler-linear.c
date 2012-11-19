/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General
 * Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"
#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-linear.h"

enum
{
  PROP_0,
  PROP_LAST
};

static void gegl_sampler_linear_get (      GeglSampler* restrict  self,
                                     const gdouble                absolute_x,
                                     const gdouble                absolute_y,
                                           GeglMatrix2           *scale,
                                           void*        restrict  output,
                                           GeglAbyssPolicy        repeat_mode);

G_DEFINE_TYPE (GeglSamplerLinear, gegl_sampler_linear, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_linear_class_init (GeglSamplerLinearClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);

  sampler_class->get = gegl_sampler_linear_get;
}

static void
gegl_sampler_linear_init (GeglSamplerLinear *self)
{
  GEGL_SAMPLER (self)->context_rect[0].x = 0;
  GEGL_SAMPLER (self)->context_rect[0].y = 0;
  GEGL_SAMPLER (self)->context_rect[0].width = 2;
  GEGL_SAMPLER (self)->context_rect[0].height = 2;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static void
gegl_sampler_linear_get (      GeglSampler*    restrict  self,
                         const gdouble                   absolute_x,
                         const gdouble                   absolute_y,
                               GeglMatrix2              *scale,
                               void*           restrict  output,
                               GeglAbyssPolicy           repeat_mode)
{
  const gint pixels_per_buffer_row = 64;
  const gint channels = 4;

  /*
   * The "-1/2"s are there because we want the index of the pixel to
   * the left and top of the location, and with GIMP's convention the
   * top left of the top left pixel is located at
   * (1/2,1/2). Basically, we are converting from a coordinate system
   * in which the origin is at the top left pixel of the pixel with
   * index (0,0), to a coordinate system in which the origin is at the
   * center of the same pixel.
   */
  const gdouble iabsolute_x = absolute_x - (gdouble) 0.5;
  const gdouble iabsolute_y = absolute_y - (gdouble) 0.5;

  const gint ix = GEGL_FAST_PSEUDO_FLOOR (iabsolute_x);
  const gint iy = GEGL_FAST_PSEUDO_FLOOR (iabsolute_y);

  /*
   * Point the data tile pointer to the first channel of the top_left
   * pixel value:
   */
  const gfloat* restrict in_bptr =
    gegl_sampler_get_ptr (self, ix, iy, repeat_mode);

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of the center of the top left pixel. Similarly for
   * y. Range of values: [0,1].
   */
  const gfloat x = iabsolute_x - ix;
  const gfloat y = iabsolute_y - iy;

  /*
   * First bilinear weight:
   */
  const gfloat x_times_y = x * y;

  /*
   * Load top row:
   */
  const gfloat top_left_0 = *in_bptr++;
  const gfloat top_left_1 = *in_bptr++;
  const gfloat top_left_2 = *in_bptr++;
  const gfloat top_left_3 = *in_bptr++;
  const gfloat top_rite_0 = *in_bptr++;
  const gfloat top_rite_1 = *in_bptr++;
  const gfloat top_rite_2 = *in_bptr++;
  const gfloat top_rite_3 = *in_bptr;

  in_bptr += 1 + ( pixels_per_buffer_row - 2 ) * channels;

  {
  /*
   * More bilinear weights:
   *
   * (Note: w = 1-x and z = 1-y.)
   */
  const gfloat w_times_y = y - x_times_y;
  const gfloat x_times_z = x - x_times_y;

  /*
   * Load bottom row:
   */
  const gfloat bot_left_0 = *in_bptr++;
  const gfloat bot_left_1 = *in_bptr++;
  const gfloat bot_left_2 = *in_bptr++;
  const gfloat bot_left_3 = *in_bptr++;
  const gfloat bot_rite_0 = *in_bptr++;
  const gfloat bot_rite_1 = *in_bptr++;
  const gfloat bot_rite_2 = *in_bptr++;
  const gfloat bot_rite_3 = *in_bptr;

  /*
   * Last bilinear weight:
   */
  const gfloat w_times_z = (gfloat) 1. - ( x + w_times_y );

  gfloat newval[4];

  newval[0] =
    x_times_y * bot_rite_0
    +
    w_times_y * bot_left_0
    +
    x_times_z * top_rite_0
    +
    w_times_z * top_left_0;

  newval[1] =
    x_times_y * bot_rite_1
    +
    w_times_y * bot_left_1
    +
    x_times_z * top_rite_1
    +
    w_times_z * top_left_1;

  newval[2] =
    x_times_y * bot_rite_2
    +
    w_times_y * bot_left_2
    +
    x_times_z * top_rite_2
    +
    w_times_z * top_left_2;

  newval[3] =
    x_times_y * bot_rite_3
    +
    w_times_y * bot_left_3
    +
    x_times_z * top_rite_3
    +
    w_times_z * top_left_3;

    babl_process (self->fish, newval, output, 1);
  }
}
