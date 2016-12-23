/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2012 Nicolas Robidoux based on earlier code
 *           2012 Massimo Valentini
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-sampler-cubic.h"

enum
{
  PROP_0,
  PROP_B,
  PROP_C,
  PROP_TYPE,
  PROP_LAST
};

static void gegl_sampler_cubic_finalize (      GObject         *gobject);
static void gegl_sampler_cubic_get      (      GeglSampler     *sampler,
                                         const gdouble          absolute_x,
                                         const gdouble          absolute_y,
                                               GeglMatrix2     *scale,
                                               void            *output,
                                               GeglAbyssPolicy  repeat_mode);
static void get_property                (      GObject         *gobject,
                                               guint            prop_id,
                                               GValue          *value,
                                               GParamSpec      *pspec);
static void set_property                (      GObject         *gobject,
                                               guint            prop_id,
                                         const GValue          *value,
                                               GParamSpec      *pspec);
static inline gfloat cubicKernel        (const gfloat           x,
                                         const gdouble          b,
                                         const gdouble          c);


G_DEFINE_TYPE (GeglSamplerCubic, gegl_sampler_cubic, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_cubic_class_init (GeglSamplerCubicClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->finalize     = gegl_sampler_cubic_finalize;

  sampler_class->get     = gegl_sampler_cubic_get;

  g_object_class_install_property ( object_class, PROP_B,
    g_param_spec_double ("b",
                         "B",
                         "B-spline parameter",
                         0.0,
                         1.0,
                         1.0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property ( object_class, PROP_C,
    g_param_spec_double ("c",
                         "C",
                         "C-spline parameter",
                         0.0,
                         1.0,
                         0.0,
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property ( object_class, PROP_TYPE,
    g_param_spec_string ("type",
                         "type",
                         "B-spline type (cubic | catmullrom | formula) 2c+b=1",
                         "cubic",
                         G_PARAM_CONSTRUCT | G_PARAM_READWRITE));
}

static void
gegl_sampler_cubic_finalize (GObject *object)
{
  g_free (GEGL_SAMPLER_CUBIC (object)->type);
  G_OBJECT_CLASS (gegl_sampler_cubic_parent_class)->finalize (object);
}

static void
gegl_sampler_cubic_init (GeglSamplerCubic *self)
{
  /*
   * In principle, x=y=-1 and width=height=4 are enough. The following
   * values are chosen so as to make the context_rect symmetrical
   * w.r.t. the anchor point. This is so that enough elbow room is
   * added with transformations that reflect the context rect. If the
   * context_rect is not symmetrical, the transformation may turn
   * right into left, and if the context_rect does not stretch far
   * enough on the left, pixel lookups will fail.
   */
  GEGL_SAMPLER (self)->level[0].context_rect.x = -2;
  GEGL_SAMPLER (self)->level[0].context_rect.y = -2;
  GEGL_SAMPLER (self)->level[0].context_rect.width = 5;
  GEGL_SAMPLER (self)->level[0].context_rect.height = 5;
  GEGL_SAMPLER (self)->interpolate_format = gegl_babl_rgbA_linear_float ();

  self->b=1.0;
  self->c=0.0;
  self->type = g_strdup("cubic");
  if (strcmp (self->type, "cubic"))
    {
      /*
       * The following values are actually not the correct ones for
       * cubic B-spline smoothing. The correct values are b=1 and c=0.
       */
      /* cubic B-spline */
      self->b = 0.0;
      self->c = 0.5;
    }
  else if (strcmp (self->type, "catmullrom"))
    {
      /*
       * The following values are actually not the correct ones for
       * Catmull-Rom. The correct values are b=0 and c=0.5.
       */
      /* Catmull-Rom spline */
      self->b = 1.0;
      self->c = 0.0;
    }
  else if (strcmp (self->type, "formula"))
    {
      /*
       * This ensures that the spline is a Keys spline. The c of
       * BC-splines is the alpha of Keys.
       */
      self->c = 0.5 * (1.0 - self->b);
    }
}

static void inline
gegl_sampler_box_get (GeglSampler*    restrict  self,
                      const gdouble             absolute_x,
                      const gdouble             absolute_y,
                      GeglMatrix2              *scale,
                      void*           restrict  output,
                      GeglAbyssPolicy           repeat_mode)
{
  gfloat result[4] = {0,0,0,0};
  const float iabsolute_x = (float) absolute_x - 0.5;
  const float iabsolute_y = (float) absolute_y - 0.5;
  const gint ix = floorf (iabsolute_x - scale->coeff[0][0]/2);
  const gint iy = floorf (iabsolute_y - scale->coeff[1][1]/2);
  const gint xx = ceilf (iabsolute_x + scale->coeff[0][0]/2);
  const gint yy = ceilf (iabsolute_y + scale->coeff[1][1]/2);
  int u, v;
  int count = 0;
  int hskip = scale->coeff[0][0] / 5;
  int vskip = scale->coeff[1][1] / 5;

  if (hskip <= 0)
    hskip = 1;
  if (vskip <= 0)
    vskip = 1;

  for (v = iy; v < yy; v += vskip)
  {
    for (u = ix; u < xx; u += hskip)
    {
      int c;
      gfloat input[4];
      GeglRectangle rect = {u, v, 1, 1};
      gegl_buffer_get (self->buffer, &rect, 1.0, self->interpolate_format, input, GEGL_AUTO_ROWSTRIDE, repeat_mode);
      for (c = 0; c < 4; c++)
        result[c] += input[c];
      count ++;
    }
  }
  result[0] /= count;
  result[1] /= count;
  result[2] /= count;
  result[3] /= count;
  babl_process (self->fish, result, output, 1);
}

void
gegl_sampler_cubic_get (      GeglSampler     *self,
                        const gdouble          absolute_x,
                        const gdouble          absolute_y,
                              GeglMatrix2     *scale,
                              void            *output,
                              GeglAbyssPolicy  repeat_mode)
{
  if (scale && 
      (scale->coeff[0][0] * scale->coeff[0][0] +
      scale->coeff[1][1] * scale->coeff[1][1])
    > 8.0)
  {
    gegl_sampler_box_get (self, absolute_x, absolute_y, scale, output, repeat_mode);
  }
  else
  {
    GeglSamplerCubic *cubic       = (GeglSamplerCubic*)(self);
    const gint        offsets[16] = {
                                      -4-GEGL_SAMPLER_MAXIMUM_WIDTH   *4, 4, 4, 4,
                                        (GEGL_SAMPLER_MAXIMUM_WIDTH-3)*4, 4, 4, 4,
                                        (GEGL_SAMPLER_MAXIMUM_WIDTH-3)*4, 4, 4, 4,
                                        (GEGL_SAMPLER_MAXIMUM_WIDTH-3)*4, 4, 4, 4
                                    };
    gfloat           *sampler_bptr;
    gfloat            factor;
    gfloat            newval[4]   = {0, 0, 0, 0};
    gint              i,
                      j,
                      k           = 0;

    /*
     * The "-1/2"s are there because we want the index of the pixel
     * center to the left and top of the location, and with GIMP's
     * convention the top left of the top left pixel is located at
     * (0,0), and its center is at (1/2,1/2), so that anything less than
     * 1/2 needs to go negative. Another way to look at this is that we
     * are converting from a coordinate system in which the origin is at
     * the top left corner of the pixel with index (0,0), to a
     * coordinate system in which the origin is at the center of the
     * same pixel.
     */
    const double iabsolute_x = (double) absolute_x - 0.5;
    const double iabsolute_y = (double) absolute_y - 0.5;

    const gint ix = floorf (iabsolute_x);
    const gint iy = floorf (iabsolute_y);

    /*
     * x is the x-coordinate of the sampling point relative to the
     * position of the center of the top left pixel. Similarly for
     * y. Range of values: [0,1].
     */
    const gfloat x = iabsolute_x - ix;
    const gfloat y = iabsolute_y - iy;

    sampler_bptr = gegl_sampler_get_ptr (self, ix, iy, repeat_mode);

    for (j=-1; j<3; j++)
      for (i=-1; i<3; i++)
        {
          sampler_bptr += offsets[k++];

          factor = cubicKernel (y - j, cubic->b, cubic->c) *
                   cubicKernel (x - i, cubic->b, cubic->c);

          newval[0] += factor * sampler_bptr[0];
          newval[1] += factor * sampler_bptr[1];
          newval[2] += factor * sampler_bptr[2];
          newval[3] += factor * sampler_bptr[3];
        }

    babl_process (self->fish, newval, output, 1);
  }
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSamplerCubic *self = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        g_value_set_double (value, self->b);
        break;

      case PROP_TYPE:
        g_value_set_string (value, self->type);
        break;

      default:
        break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglSamplerCubic *self = GEGL_SAMPLER_CUBIC (object);

  switch (prop_id)
    {
      case PROP_B:
        self->b = g_value_get_double (value);
        break;

      case PROP_TYPE:
        if (self->type)
          g_free (self->type);
        self->type = g_value_dup_string (value);
        break;

      default:
        break;
    }
}

static inline gfloat
cubicKernel (const gfloat  x,
             const gdouble b,
             const gdouble c)
{
  const gfloat x2 = x*x;
  const gfloat ax = ( x<(gfloat) 0. ? -x : x );

  if (x2 <= (gfloat) 1.) return ( (gfloat) ((12-9*b-6*c)/6) * ax +
                                  (gfloat) ((-18+12*b+6*c)/6) ) * x2 +
                                  (gfloat) ((6-2*b)/6);

  if (x2 < (gfloat) 4.) return ( (gfloat) ((-b-6*c)/6) * ax +
                                 (gfloat) ((6*b+30*c)/6) ) * x2 +
                                 (gfloat) ((-12*b-48*c)/6) * ax +
                                 (gfloat) ((8*b+24*c)/6);

  return (gfloat) 0.;
}
