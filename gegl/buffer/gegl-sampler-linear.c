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


/*
 * FAST_PSEUDO_FLOOR is a floor and floorf replacement which has been
 * found to be faster on several linux boxes than the library
 * version. It returns the floor of its argument unless the argument
 * is a negative integer, in which case it returns one less than the
 * floor. For example:
 *
 * FAST_PSEUDO_FLOOR(0.5) = 0
 *
 * FAST_PSEUDO_FLOOR(0.f) = 0
 *
 * FAST_PSEUDO_FLOOR(-.5) = -1
 *
 * as expected, but
 *
 * FAST_PSEUDO_FLOOR(-1.f) = -2
 *
 * The locations of the discontinuities of FAST_PSEUDO_FLOOR are the
 * same as floor and floorf; it is just that at negative integers the
 * function is discontinuous on the right instead of the left.
 */
#define FAST_PSEUDO_FLOOR(x) ( (int)(x) - ( (x) < 0. ) )
/*
 * Alternative (if conditional move is fast and correctly identified
 * by the compiler):
 *
 * #define FAST_PSEUDO_FLOOR(x) ( (x)>=0 ? (int)(x) : (int)(x)-1 )
 */

#ifndef restrict
#ifdef __restrict
#define restrict __restrict
#else
#ifdef __restrict__
#define restrict __restrict__
#else
#define restrict
#endif
#endif
#endif

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

static void gegl_sampler_linear_get (GeglSampler* restrict self,
                                     const gdouble         x,
                                     const gdouble         y,
                                     void*        restrict output);

static void set_property (GObject*      gobject,
                          guint         property_id,
                          const GValue* value,
                          GParamSpec*   pspec);

static void get_property (GObject*    gobject,
                          guint       property_id,
                          GValue*     value,
                          GParamSpec* pspec);

G_DEFINE_TYPE (GeglSamplerLinear, gegl_sampler_linear, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_linear_class_init (GeglSamplerLinearClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get = gegl_sampler_linear_get;
}

static void
gegl_sampler_linear_init (GeglSamplerLinear *self)
{
  GEGL_SAMPLER (self)->context_rect.x = 0;
  GEGL_SAMPLER (self)->context_rect.y = 0;
  GEGL_SAMPLER (self)->context_rect.width = 2;
  GEGL_SAMPLER (self)->context_rect.height = 2;
  GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
}

static void
gegl_sampler_linear_get (GeglSampler* restrict self,
                         const gdouble         absolute_x,
                         const gdouble         absolute_y,
                         void*        restrict output)
{
  const gint pixels_per_buffer_row = 64;
  const gint channels = 4;

  /*
   * floor's surrogate FAST_PSEUDO_FLOOR is used to make
   * sure that the transition through 0 is smooth. If it is known that
   * negative absolute_x and absolute_y will never be used, plain
   * cast---that is, const gint ix = absolute_x---is recommended
   * instead.
   */
  const gint ix = FAST_PSEUDO_FLOOR (absolute_x);
  const gint iy = FAST_PSEUDO_FLOOR (absolute_y);

  /*
   * x is the x-coordinate of the sampling point relative to the
   * position of the top left pixel center. Similarly for y. Range of
   * values: [0,1].
   */
  const gfloat x = absolute_x - ix;
  const gfloat y = absolute_y - iy;

  /*
   * Point the data tile pointer to the first channel of the top_left
   * pixel value:
   */
  const gfloat* restrict in_bptr = gegl_sampler_get_ptr (self, ix, iy);

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
  const gfloat w_times_z = 1.f - ( x + w_times_y );

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

static void
set_property (GObject*      gobject,
              guint         property_id,
              const GValue* value,
              GParamSpec*   pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}

static void
get_property (GObject*    gobject,
              guint       property_id,
              GValue*     value,
              GParamSpec* pspec)
{
  G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
}

