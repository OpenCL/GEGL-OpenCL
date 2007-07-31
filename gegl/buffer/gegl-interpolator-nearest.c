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
#include "gegl-interpolator-nearest.h"
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

static void    gegl_interpolator_nearest_get (GeglInterpolator *self,
                                              gdouble           x,
                                              gdouble           y,
                                              void             *output);

static void    gegl_interpolator_nearest_prepare (GeglInterpolator *self);


G_DEFINE_TYPE (GeglInterpolatorNearest, gegl_interpolator_nearest, GEGL_TYPE_INTERPOLATOR)

static void
gegl_interpolator_nearest_class_init (GeglInterpolatorNearestClass *klass)
{
  GObjectClass          *object_class       = G_OBJECT_CLASS (klass);
  GeglInterpolatorClass *interpolator_class = GEGL_INTERPOLATOR_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  interpolator_class->get     = gegl_interpolator_nearest_get;

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
gegl_interpolator_nearest_init (GeglInterpolatorNearest *self)
{
}

void
gegl_interpolator_nearest_get (GeglInterpolator *self,
                               gdouble           x,
                               gdouble           y,
                               void             *output)
{
  GeglBuffer *input  = self->input;
  gfloat     *buffer;
  gfloat      dst[4];
  gfloat      abyss = 0.;

  gegl_interpolator_fill_buffer (self, x, y);
  buffer = self->cache_buffer;
  if (!buffer)
    return;

  if (x >= 0 &&
      y >= 0 &&
      x < input->width &&
      y < input->height)
    {
      gint u   = (gint) x;
      gint v   = (gint) y;
      gint pos = (v * input->width + u) * 4;
      memcpy (dst, buffer + pos, sizeof (gfloat) * 4);
    }
  else
    {
      dst[0] = abyss;
      dst[1] = abyss;
      dst[2] = abyss;
      dst[3] = abyss;
    }
  babl_process (babl_fish (self->interpolate_format, self->format),
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
