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
 */

#include "config.h"
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-private.h"
#include "gegl-sampler-cubic.h"

enum
{
  PROP_0,
  PROP_B,
  PROP_C,
  PROP_TYPE,
  PROP_LAST
};

static void      gegl_sampler_cubic_get (GeglSampler  *sampler,
                                         gdouble       x,
                                         gdouble       y,
                                         void         *output);
static void      get_property           (GObject      *gobject,
                                         guint         prop_id,
                                         GValue       *value,
                                         GParamSpec   *pspec);
static void      set_property           (GObject      *gobject,
                                         guint         prop_id,
                                         const GValue *value,
                                         GParamSpec   *pspec);
static inline gfloat cubicKernel       (gfloat        x,
                                        gfloat        b,
                                        gfloat        c);


G_DEFINE_TYPE (GeglSamplerCubic, gegl_sampler_cubic, GEGL_TYPE_SAMPLER)

static void
gegl_sampler_cubic_class_init (GeglSamplerCubicClass *klass)
{
  GeglSamplerClass *sampler_class = GEGL_SAMPLER_CLASS (klass);
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;

  sampler_class->get     = gegl_sampler_cubic_get;

  g_object_class_install_property (object_class, PROP_B,
                                   g_param_spec_double ("b",
                                                        "B",
                                                        "B-spline parameter",
                                                        0.0,
                                                        1.0,
                                                        1.0,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_C,
                                   g_param_spec_double ("c",
                                                        "C",
                                                        "C-spline parameter",
                                                        0.0,
                                                        1.0,
                                                        0.0,
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

  g_object_class_install_property (object_class, PROP_TYPE,
                                   g_param_spec_string ("type",
                                                        "type",
                                                        "B-spline type (cubic | catmullrom | formula) 2c+b=1",
                                                        "cubic",
                                                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE));

}

static void
gegl_sampler_cubic_init (GeglSamplerCubic *self)
{
 GEGL_SAMPLER (self)->context_rect.x = -1;
 GEGL_SAMPLER (self)->context_rect.y = -1;
 GEGL_SAMPLER (self)->context_rect.width = 4;
 GEGL_SAMPLER (self)->context_rect.height = 4;
 GEGL_SAMPLER (self)->interpolate_format = babl_format ("RaGaBaA float");
 self->b=1.0;
 self->c=0.0;
 self->type = g_strdup("cubic");
 if (strcmp (self->type, "cubic"))
    {
      /* cubic B-spline */
      self->b = 0.0;
      self->c = 0.5;
    }
  else if (strcmp (self->type, "catmullrom"))
    {
      /* Catmull-Rom spline */
      self->b = 1.0;
      self->c = 0.0;
    }
  else if (strcmp (self->type, "formula"))
    {
      self->c = (1.0 - self->b) / 2.0;
    }
}

void
gegl_sampler_cubic_get (GeglSampler *self,
                        gdouble      x,
                        gdouble      y,
                        void        *output)
{
  GeglSamplerCubic *cubic = (GeglSamplerCubic*)(self);
  GeglRectangle     context_rect;
  const gint        offsets[16]={-4-64*4, 4, 4, 4,
                                (64-3)*4, 4, 4, 4,
                                (64-3)*4, 4, 4, 4,
                                (64-3)*4, 4, 4, 4};
  gfloat           *sampler_bptr;
  gfloat            factor;

#ifdef HAS_G4FLOAT
  g4float            newval4 = g4float_zero;
  gfloat            *newval = &newval4;
#else
  gfloat             newval[4] = {0.0, 0.0, 0.0, 0.0};
#endif

  gint              u,v;
  gint              dx,dy;
  gint              i;

  context_rect = self->context_rect;
  dx = (gint) x;
  dy = (gint) y;
  sampler_bptr = gegl_sampler_get_ptr (self, dx, dy);

#ifdef HAS_G4FLOAT
  if (G_LIKELY (gegl_cpu_accel_get_support () & (GEGL_CPU_ACCEL_X86_SSE|
                                                 GEGL_CPU_ACCEL_X8&_MMX)))
    {
      for (v=dy+context_rect.y, i=0; v < dy+context_rect.y+context_rect.height ; v++)
        for (u=dx+context_rect.x ; u < dx+context_rect.x+context_rect.width  ; u++, i++)
          {
            sampler_bptr += offsets[i];
            factor = cubicKernel (y - v, cubic->b, cubic->c) *
                     cubicKernel (x - u, cubic->b, cubic->c);
            newval4 += g4float_mul(&sampler_bptr[0], factor);
           }
     }
   else
#endif
     {
       for (v=dy+context_rect.y, i=0; v < dy+context_rect.y+context_rect.height ; v++)
         for (u=dx+context_rect.x ; u < dx+context_rect.x+context_rect.width  ; u++, i++)
           {
             /*sampler_bptr = gegl_sampler_get_from_buffer (self, u, v);*/
             sampler_bptr += offsets[i];
             factor = cubicKernel (y - v, cubic->b, cubic->c) *
                      cubicKernel (x - u, cubic->b, cubic->c);

            newval[0] += factor * sampler_bptr[0];
            newval[1] += factor * sampler_bptr[1];
            newval[2] += factor * sampler_bptr[2];
            newval[3] += factor * sampler_bptr[3];
           }
     }

  babl_process (self->fish, newval, output, 1);
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
cubicKernel (gfloat x,
             gfloat b,
             gfloat c)
 {
  gfloat weight, x2, x3;
  gfloat ax = x;
  if (ax < 0.0)
    ax *= -1.0;

  if (ax > 2) return 0;

  x3 = ax * ax * ax;
  x2 = ax * ax;

  if (ax < 1)
    weight = (12 - 9 * b - 6 * c) * x3 +
             (-18 + 12 * b + 6 * c) * x2 +
             (6 - 2 * b);
  else
    weight = (-b - 6 * c) * x3 +
             (6 * b + 30 * c) * x2 +
             (-12 * b - 48 * c) * ax +
             (8 * b + 24 * c);

  return weight / 6.0;
}

