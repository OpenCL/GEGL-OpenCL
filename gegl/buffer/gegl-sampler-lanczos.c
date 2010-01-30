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
 */

/* XXX WARNING: This code compiles, but is functionally broken, and
 * currently not used by the rest of GeglBuffer */


#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-lanczos.h"


enum
{
  PROP_0,
  PROP_LANCZOS_WIDTH,
  PROP_LANCZOS_SAMPLES,
  PROP_LAST
};

static inline gdouble sinc (gdouble x);
static void           lanczos_lookup (GeglSamplerLanczos *sampler);
static void           gegl_sampler_lanczos_get (GeglSampler  *sampler,
                                                gdouble       x,
                                                gdouble       y,
                                                void         *output);
static void           get_property             (GObject      *gobject,
                                                guint         prop_id,
                                                GValue       *value,
                                                GParamSpec   *pspec);
static void           set_property             (GObject      *gobject,
                                                guint         prop_id,
                                                const GValue *value,
                                                GParamSpec   *pspec);
static GObject *
gegl_sampler_lanczos_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params);
static void finalize (GObject *object);


G_DEFINE_TYPE (GeglSamplerLanczos, gegl_sampler_lanczos, GEGL_TYPE_SAMPLER)
static GObjectClass * parent_class = NULL;


static void
gegl_sampler_lanczos_class_init (GeglSamplerLanczosClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  parent_class                = g_type_class_peek_parent (klass);
  object_class->finalize     = finalize;
  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->constructor  = gegl_sampler_lanczos_constructor;

  sampler_class->get     = gegl_sampler_lanczos_get;

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

}

static void
gegl_sampler_lanczos_init (GeglSamplerLanczos *self)
{

}

static GObject *
gegl_sampler_lanczos_constructor (GType                  type,
                                  guint                  n_params,
                                  GObjectConstructParam *params)
{
  GObject            *object;
  GeglSamplerLanczos *self;
  gint                i;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  self   = GEGL_SAMPLER_LANCZOS (object);
  for (i = 0; i < n_params; i++) {
    if (!strcmp (params[i].pspec->name, "lanczos_spp"))
      g_object_set(object, params[i].pspec->name, g_value_get_int (params[i].value), NULL);
    if (!strcmp (params[i].pspec->name, "lanczos_width"))
      g_object_set(object, params[i].pspec->name, g_value_get_int (params[i].value), NULL);
  }

  lanczos_lookup (self);
  return object;
}

static void
finalize (GObject *object)
{
  GeglSamplerLanczos *self    = GEGL_SAMPLER_LANCZOS (object);

  if ( self->lanczos_lookup != NULL )
    {
       g_free (self->lanczos_lookup);
       self->lanczos_lookup = NULL;
     }

  G_OBJECT_CLASS (gegl_sampler_lanczos_parent_class)->finalize (object);
}

void
gegl_sampler_lanczos_get (GeglSampler *self,
                          gdouble      x,
                          gdouble      y,
                          void        *output)
{
  GeglSamplerLanczos      *lanczos      = GEGL_SAMPLER_LANCZOS (self);
  GeglRectangle            context_rect = self->context_rect;
  gfloat                  *sampler_bptr;
  gdouble                  x_sum, y_sum;
  gfloat                   newval[4] = {0.0, 0.0, 0.0, 0.0};
  gint                     i, j;
  gint                     spp    = lanczos->lanczos_spp;
  gint                     width  = lanczos->lanczos_width;
  gint                     width2 = context_rect.width;
  gint                     dx,dy;
  gint                     u,v;

  /* FIXME: move the initialization of these arrays into the _prepare function
   * to speed up actual resampling
   */
  gfloat                  *x_kernel, /* 1-D kernels of Lanczos window coeffs */
                          *y_kernel;

  x_kernel = g_newa (gfloat, width2);
  y_kernel = g_newa (gfloat, width2);

  self->interpolate_format = babl_format ("RaGaBaA float");

  dx = (gint) ((x - ((gint) x)) * spp + 0.5);
  dy = (gint) ((y - ((gint) y)) * spp + 0.5);
  /* fill 1D kernels */
  for (x_sum = y_sum = 0.0, i = width; i >= -width; i--)
    {
      gint pos    = i * spp;
      x_sum += x_kernel[width + i] = lanczos->lanczos_lookup[ABS (dx - pos)];
      y_sum += y_kernel[width + i] = lanczos->lanczos_lookup[ABS (dy - pos)];
    }

  /* normalise the weighted arrays */
  for (i = 0; i < width2; i++)
    {
      x_kernel[i] /= x_sum;
      y_kernel[i] /= y_sum;
    }

  dx = (gint) x;
  dy = (gint) y;
  for (v=dy+context_rect.y, j = 0; v < dy+context_rect.y+context_rect.height; j++, v++)
    for (u=dx+context_rect.x, i = 0; u < dx+context_rect.x+context_rect.width; i++, u++)
      {
         sampler_bptr = gegl_sampler_get_from_buffer (self, u, v);
         newval[0] += y_kernel[j] * x_kernel[i] * sampler_bptr[0];
         newval[1] += y_kernel[j] * x_kernel[i] * sampler_bptr[1];
         newval[2] += y_kernel[j] * x_kernel[i] * sampler_bptr[2];
         newval[3] += y_kernel[j] * x_kernel[i] * sampler_bptr[3];
      }

  babl_process (self->fish, newval, output, 1);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglSamplerLanczos *self         = GEGL_SAMPLER_LANCZOS (object);

  switch (prop_id)
    {
      case PROP_LANCZOS_WIDTH:
        g_value_set_int (value, self->lanczos_width);
        break;

      case PROP_LANCZOS_SAMPLES:
        g_value_set_int (value, self->lanczos_spp);
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
  GeglSamplerLanczos *self         = GEGL_SAMPLER_LANCZOS (object);

  switch (prop_id)
    {
      case PROP_LANCZOS_WIDTH:
        {
        self->lanczos_width = g_value_get_int (value);
        GEGL_SAMPLER (self)->context_rect.x = - self->lanczos_width;
        GEGL_SAMPLER (self)->context_rect.y = - self->lanczos_width;
        GEGL_SAMPLER (self)->context_rect.width = self->lanczos_width*2+1;
        GEGL_SAMPLER (self)->context_rect.height = self->lanczos_width*2+1;
        }
        break;

      case PROP_LANCZOS_SAMPLES:
        self->lanczos_spp = g_value_get_int (value);
        break;

      default:
        break;
    }
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
lanczos_lookup (GeglSamplerLanczos *sampler)
{
  GeglSamplerLanczos *self = GEGL_SAMPLER_LANCZOS (sampler);

  const gint    lanczos_width = self->lanczos_width;
  const gint    samples       = (self->lanczos_spp * (lanczos_width + 1));
  const gdouble dx            = (gdouble) lanczos_width / (gdouble) (samples - 1);
  gdouble x = 0.0;
  gint    i;

  if (self->lanczos_lookup != NULL)
    g_free (self->lanczos_lookup);

  self->lanczos_lookup = g_new (gfloat, samples);

  for (i = 0; i < samples; i++)
    {
      self->lanczos_lookup[i] = ((ABS (x) < lanczos_width) ?
                                 (sinc (x) * sinc (x / lanczos_width)) : 0.0);
      x += dx;
    }
}
