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
#include "gegl-interpolator-lanczos.h"
#include <string.h>
#include <math.h>

enum
{
  PROP_0,
  PROP_INPUT,
  PROP_LANCZOS_WIDTH,
  PROP_LANCZOS_SAMPLES,
  PROP_FORMAT,
  PROP_LAST
};

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglInterpolatorLanczos *self         = GEGL_INTERPOLATOR_LANCZOS (object);
  GeglInterpolator        *interpolator = GEGL_INTERPOLATOR (object);

  switch (prop_id)
    {
      case PROP_INPUT:
        g_value_set_object (value, interpolator->input);
        break;

      case PROP_LANCZOS_WIDTH:
        g_value_set_int (value, self->lanczos_width);
        break;

      case PROP_LANCZOS_SAMPLES:
        g_value_set_int (value, self->lanczos_spp);
        break;

      case PROP_FORMAT:
        g_value_set_pointer (value, interpolator->format);
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
  GeglInterpolatorLanczos *self         = GEGL_INTERPOLATOR_LANCZOS (object);
  GeglInterpolator        *interpolator = GEGL_INTERPOLATOR (object);

  switch (prop_id)
    {
      case PROP_INPUT:
        interpolator->input = GEGL_BUFFER (g_value_dup_object (value));
        break;

      case PROP_LANCZOS_WIDTH:
        self->lanczos_width = g_value_get_int (value);
        break;

      case PROP_LANCZOS_SAMPLES:
        self->lanczos_spp = g_value_get_int (value);
        break;

      case PROP_FORMAT:
        interpolator->format = g_value_get_pointer (value);
        break;

      default:
        break;
    }
}

static void    gegl_interpolator_lanczos_get (GeglInterpolator *self,
                                              gdouble           x,
                                              gdouble           y,
                                              void             *output);

static void    gegl_interpolator_lanczos_prepare (GeglInterpolator *self);

static inline gdouble sinc (gdouble x);
static void           lanczos_lookup (GeglInterpolator *interpolator);

G_DEFINE_TYPE (GeglInterpolatorLanczos, gegl_interpolator_lanczos, GEGL_TYPE_INTERPOLATOR)

static void
finalize (GObject *object)
{
  GeglInterpolatorLanczos *self         = GEGL_INTERPOLATOR_LANCZOS (object);
  GeglInterpolator        *interpolator = GEGL_INTERPOLATOR (object);

  g_free (self->lanczos_lookup);
  g_free (interpolator->buffer);
  G_OBJECT_CLASS (gegl_interpolator_lanczos_parent_class)->finalize (object);
}

static void
gegl_interpolator_lanczos_class_init (GeglInterpolatorLanczosClass *klass)
{
  GObjectClass          *object_class       = G_OBJECT_CLASS (klass);
  GeglInterpolatorClass *interpolator_class = GEGL_INTERPOLATOR_CLASS (klass);

  object_class->finalize     = finalize;
  object_class->set_property = set_property;
  object_class->get_property = get_property;

  interpolator_class->prepare = gegl_interpolator_lanczos_prepare;
  interpolator_class->get     = gegl_interpolator_lanczos_get;

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "Input pad, for image buffer input.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (object_class, PROP_LANCZOS_WIDTH,
                                   g_param_spec_int ("lanczos_width",
                                                     "lanczos_width",
                                                     "Width of the lanczos filter",
                                                     3,
                                                     21,
                                                     3,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_LANCZOS_SAMPLES,
                                   g_param_spec_int ("lanczos_spp",
                                                     "lanczos_spp",
                                                     "Sampels per pixels",
                                                     4000,
                                                     10000,
                                                     4000,
                                                     G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_FORMAT,
                                   g_param_spec_pointer ("format",
                                                         "format",
                                                         "babl format",
                                                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gegl_interpolator_lanczos_init (GeglInterpolatorLanczos *self)
{
}

void
gegl_interpolator_lanczos_prepare (GeglInterpolator *interpolator)
{
  GeglBuffer *input = GEGL_BUFFER (interpolator->input);

  /* calculate lookup */
  lanczos_lookup (interpolator);
  /* fill the internal bufer */
  interpolator->buffer             = g_malloc0 (input->width * input->height * 4 * 4);
  interpolator->interpolate_format = babl_format ("RaGaBaA float");
  gegl_buffer_get (interpolator->input, NULL, 1.0,
                   interpolator->interpolate_format,
                   interpolator->buffer);
}

void
gegl_interpolator_lanczos_get (GeglInterpolator *interpolator,
                               gdouble           x,
                               gdouble           y,
                               void             *output)
{
  GeglInterpolatorLanczos *self   = GEGL_INTERPOLATOR_LANCZOS (interpolator);
  GeglBuffer              *input  = interpolator->input;
  gfloat                  *buffer = interpolator->buffer;
  gfloat                  *buf_ptr;

  gdouble                  x_sum, y_sum, arecip;
  gdouble                  newval[4];

  gfloat                   dst[4];
  gfloat                   abyss = 0.;
  gint                     i, j, pos, pu, pv;
  gint                     lanczos_spp    = self->lanczos_spp;
  gint                     lanczos_width  = self->lanczos_width;
  gint                     lanczos_width2 = lanczos_width * 2 + 1;

  gdouble                  x_kernel[lanczos_width2], /* 1-D kernels of Lanczos window coeffs */
                           y_kernel[lanczos_width2];

  if (x >= 0 &&
      y >= 0 &&
      x < input->width &&
      y < input->height)
    {
      gint u = (gint) x;
      gint v = (gint) y;
      /* get weight for fractional error */
      gint su = (gint) ((x - u) * lanczos_spp + 0.5);
      gint sv = (gint) ((y - v) * lanczos_spp + 0.5);
      /* fill 1D kernels */
      for (x_sum = y_sum = 0.0, i = lanczos_width; i >= -lanczos_width; i--)
        {
          pos    = i * lanczos_spp;
          x_sum += x_kernel[lanczos_width + i] = self->lanczos_lookup[ABS (su - pos)];
          y_sum += y_kernel[lanczos_width + i] = self->lanczos_lookup[ABS (sv - pos)];
        }

      /* normalise the weighted arrays */
      for (i = 0; i < lanczos_width2; i++)
        {
          x_kernel[i] /= x_sum;
          y_kernel[i] /= y_sum;
        }

      newval[0] = newval[1] = newval[2] = newval[3] = 0.0;
      for (j = 0; j < lanczos_width2; j++)
        for (i = 0; i < lanczos_width2; i++)
          {
            pu         = CLAMP (u + i - lanczos_width, 0, input->width - 1);
            pv         = CLAMP (v + j - lanczos_width, 0, input->height - 1);
            buf_ptr    = buffer + ((pv * input->width + pu) * 4);
            newval[0] += y_kernel[j] * x_kernel[i] * buf_ptr[0] * buf_ptr[3];
            newval[1] += y_kernel[j] * x_kernel[i] * buf_ptr[1] * buf_ptr[3];
            newval[2] += y_kernel[j] * x_kernel[i] * buf_ptr[2] * buf_ptr[3];
            newval[3] += y_kernel[j] * x_kernel[i] * buf_ptr[3];
          }
      if (newval[3] <= 0.0)
        {
          arecip    = 0.0;
          newval[3] = 0;
        }
      else if (newval[3] > G_MAXDOUBLE)
        {
          arecip    = 1.0 / newval[3];
          newval[3] = G_MAXDOUBLE;
        }
      else
        {
          arecip = 1.0 / newval[3];
        }

      dst[0] = CLAMP (newval[0] * arecip, 0, G_MAXDOUBLE);
      dst[1] = CLAMP (newval[1] * arecip, 0, G_MAXDOUBLE);
      dst[2] = CLAMP (newval[2] * arecip, 0, G_MAXDOUBLE);
      dst[3] = CLAMP (newval[3], 0, G_MAXDOUBLE);
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



/* Internal lanczos */

static inline gdouble
sinc (gdouble x)
{
  gdouble y = x * G_PI;

  if (ABS (x) < 0.0001)
    return 1.0;

  return sin (y) / y;
}

static void
lanczos_lookup (GeglInterpolator *interpolator)
{
  GeglInterpolatorLanczos *self = GEGL_INTERPOLATOR_LANCZOS (interpolator);

  if (self->lanczos_lookup != NULL)
    g_free (self->lanczos_lookup);

  const gint    lanczos_width = self->lanczos_width;
  const gint    samples       = (self->lanczos_spp * (lanczos_width + 1));
  const gdouble dx            = (gdouble) lanczos_width / (gdouble) (samples - 1);

  self->lanczos_lookup = g_new (gfloat, samples);
  gdouble x = 0.0;
  gint    i;

  for (i = 0; i < samples; i++)
    {
      self->lanczos_lookup[i] = ((ABS (x) < lanczos_width) ?
                                 (sinc (x) * sinc (x / lanczos_width)) : 0.0);
      x += dx;
    }
}
