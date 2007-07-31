/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#include "gegl-interpolator-linear.h"
#include "gegl-buffer-private.h"
#include <string.h>
enum
{
  PROP_0,
  PROP_INPUT,
  PROP_FORMAT,
  PROP_LAST
};



static void     get_property (GObject    *gobject,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);

static void    gegl_interpolator_linear_get (GeglInterpolator *self,
                                             gdouble           x,
                                             gdouble           y,
                                             void             *output);

static void    gegl_interpolator_linear_prepare (GeglInterpolator *self);


G_DEFINE_TYPE (GeglInterpolatorLinear, gegl_interpolator_linear, GEGL_TYPE_INTERPOLATOR)

static void
gegl_interpolator_linear_class_init (GeglInterpolatorLinearClass *klass)
{
  GObjectClass          *object_class       = G_OBJECT_CLASS (klass);
  GeglInterpolatorClass *interpolator_class = GEGL_INTERPOLATOR_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  interpolator_class->prepare = gegl_interpolator_linear_prepare;
  interpolator_class->get     = gegl_interpolator_linear_get;

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format",
                                                         "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gegl_interpolator_linear_init (GeglInterpolatorLinear *self)
{
}

void
gegl_interpolator_linear_prepare (GeglInterpolator *interpolator)
{
}

void
gegl_interpolator_linear_get (GeglInterpolator *interpolator,
                              gdouble           x,
                              gdouble           y,
                              void             *output)
{
  GeglBuffer *input  = interpolator->input;
  gfloat     *buffer;
  gfloat      dst[4];
  gfloat      abyss = 0.;

  gegl_interpolator_fill_buffer (interpolator, x, y);
  buffer = interpolator->cache_buffer;
  if (!buffer)
    return;

  if (x >= 0 &&
      y >= 0 &&
      x < input->width &&
      y < input->height)
    {
      gint    i;
      gint    x0 = input->x;
      gint    y0 = input->y;
      gint    x1 = input->x + input->width - 1;
      gint    y1 = input->y + input->height - 1;

      gint    u  = (gint) x;
      gint    v  = (gint) y;
      gdouble uf = x - u;
      gdouble vf = y - v;
      gdouble q1 = vf * uf;
      gdouble q2 = (1 - uf) * vf;
      gdouble q3 = (1 - uf) * (1 - vf);
      gdouble q4 = uf * (1 - vf);

      gfloat *p00 = buffer + (v * input->width + u) * 4;
      u = CLAMP ((gint) x + 0, x0, x1);
      v = CLAMP ((gint) y + 1, y0, y1);
      gfloat *p01 = buffer + (v * input->width + u) * 4;
      u = CLAMP ((gint) x + 1, x0, x1);
      v = CLAMP ((gint) y + 0, y0, y1);
      gfloat *p10 = buffer + (v * input->width + u) * 4;
      u = CLAMP ((gint) x + 1, x0, x1);
      v = CLAMP ((gint) y + 1, y0, y1);
      gfloat *p11 = buffer + (v * input->width + u) * 4;

      for (i = 0; i < 4; i++)
        {
          dst[i] = q1 * p00[i] + q2 * p01[i] + q3 * p10[i] + q4 * p11[i];
        }
    }
  else
    {
      dst[0] = abyss;
      dst[1] = abyss;
      dst[2] = abyss;
      dst[3] = abyss;
    }
  babl_process (babl_fish (interpolator->interpolate_format, interpolator->format),
                dst, output, 1);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglInterpolator *self = GEGL_INTERPOLATOR (object);

  switch (prop_id)
    {
      case PROP_INPUT:
        g_value_set_object (value, self->input);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, self->format);
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
  GeglInterpolator *self = GEGL_INTERPOLATOR (object);

  switch (prop_id)
    {
      case PROP_INPUT:
        self->input = GEGL_BUFFER (g_value_dup_object (value));
        break;

      case PROP_FORMAT:
        self->format = g_value_get_pointer (value);
        break;

      default:
        break;
    }
}
