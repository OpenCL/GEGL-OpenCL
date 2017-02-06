/* This file is an image processing operation for GEGL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2017 Ell
 *
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_PROPERTIES

property_double (x_period, _("X Period"), 128.0)
  description (_("Period for X axis"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 256.0)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "x")

property_double (y_period, _("Y Period"), 128.0)
  description (_("Period for Y axis"))
  value_range (0.0, G_MAXDOUBLE)
  ui_range    (0.0, 256.0)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "y")

property_double (x_amplitude, _("X Amplitude"), 0.0)
  description (_("Amplitude for X axis (logarithmic scale)"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-2.0, 2.0)
  ui_meta     ("axis", "x")

property_double (y_amplitude, _("Y Amplitude"), 0.0)
  description (_("Amplitude for Y axis (logarithmic scale)"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-2.0, 2.0)
  ui_meta     ("axis", "y")

property_double (x_phase, _("X Phase"), 0.0)
  description (_("Phase for X axis"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-512.0, 512.0)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "x")

property_double (y_phase, _("Y Phase"), 0.0)
  description (_("Phase for Y axis"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-512.0, 512.0)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "y")

property_double (angle, _("Angle"), 90.0)
  description(_("Axis separation angle"))
  value_range (0.0, 360.0)
  ui_meta     ("unit", "degree")

property_double (offset, _("Offset"), 0.0)
  description(_("Value offset"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-1.0, 1.0)

property_double (exponent, _("Exponent"), 0.0)
  description(_("Value exponent (logarithmic scale)"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-2.0, 2.0)

property_double (x_offset, _("X Offset"), 0.0)
  description (_("Offset for X axis"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-512.0, 512.0)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "x")

property_double (y_offset, _("Y Offset"), 0.0)
  description (_("Offset for Y axis"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-512.0, 512.0)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "y")

property_double (rotation, _("Rotation"), 0.0)
  description(_("Pattern rotation angle"))
  value_range (0.0, 360.0)
  ui_meta     ("unit", "degree")

property_int (supersampling, _("Supersampling"), 1)
  description(_("Number of samples along each axis per pixel"))
  value_range (1, 8)

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     linear_sinusoid
#define GEGL_OP_C_SOURCE linear-sinusoid.c

#include "gegl-op.h"
#include <math.h>

static inline gdouble
odd_pow (gdouble base,
         gdouble exponent)
{
  if (base >= 0.0)
    return  pow ( base, exponent);
  else
    return -pow (-base, exponent);
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("Y' float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gdouble         offset;
  gdouble         exponent;
  gdouble         scale;
  gdouble         x_scale, y_scale;
  gdouble         x_amplitude, y_amplitude;
  gdouble         x_angle, y_angle;
  gint            i, j;
  gdouble         x0, y0;
  gdouble         x, y;
  gdouble         i_dx, i_dy;
  gdouble         j_dx, j_dy;
  gdouble         supersampling_scale;
  gdouble         supersampling_scale2;
  gint            m, n;
  gdouble         u0, v0;
  gdouble         u, v;
  gdouble         m_du, m_dv;
  gdouble         n_du, n_dv;
  gfloat         *result = out_buf;

  offset   = o->offset + .5;
  exponent = exp2 (o->exponent);

  if (o->x_period == 0.0 || o->y_period == 0.0)
    {
      gfloat value = odd_pow (offset, exponent);

      gegl_memset_pattern (result, &value, sizeof (value), n_pixels);

      return TRUE;
    }

  scale = 1.0 / (1 << level);

  x_scale = 2 * G_PI * scale / o->x_period;
  y_scale = 2 * G_PI * scale / o->y_period;

  x_amplitude = exp2 (o->x_amplitude) / 4.0;
  y_amplitude = exp2 (o->y_amplitude) / 4.0;

  x_angle = -G_PI *  o->rotation             / 180.0;
  y_angle = -G_PI * (o->rotation + o->angle) / 180.0;

  i_dx = cos (x_angle) * x_scale;
  i_dy = cos (y_angle) * y_scale;

  j_dx = sin (x_angle) * x_scale;
  j_dy = sin (y_angle) * y_scale;

  x0 = o->x_phase * x_scale          +
       (roi->x - o->x_offset) * i_dx +
       (roi->y - o->y_offset) * j_dx;
  y0 = o->y_phase * y_scale          +
       (roi->x - o->x_offset) * i_dy +
       (roi->y - o->y_offset) * j_dy;

  if (o->supersampling != 1)
    {
      gdouble offset;

      supersampling_scale  = 1.0 / o->supersampling;
      supersampling_scale2 = supersampling_scale * supersampling_scale;

      m_du = supersampling_scale * i_dx;
      m_dv = supersampling_scale * i_dy;

      n_du = supersampling_scale * j_dx;
      n_dv = supersampling_scale * j_dy;

      offset  = (1.0 - supersampling_scale) / 2.0;
      x0     -= offset * (i_dx + j_dx);
      y0     -= offset * (i_dy + j_dy);
    }

  for (j = 0; j < roi->height; j++)
    {
      x = x0;
      y = y0;

      for (i = 0; i < roi->width; i++)
        {
          gdouble z;

          if (o->supersampling == 1)
            {
              z = offset                -
                  x_amplitude * cos (x) -
                  y_amplitude * cos (y);
              z = odd_pow (z, exponent);
            }
          else
            {
              z = 0.0;

              u0 = x;
              v0 = y;

              for (n = 0; n < o->supersampling; n++)
                {
                  u = u0;
                  v = v0;

                  for (m = 0; m < o->supersampling; m++)
                    {
                      gdouble w;

                      w = offset                -
                          x_amplitude * cos (u) -
                          y_amplitude * cos (v);
                      w = odd_pow (w, exponent);

                      z += w;

                      u += m_du;
                      v += m_dv;
                    }

                  u0 += n_du;
                  v0 += n_dv;
                }

              z *= supersampling_scale2;
            }

          *result++ = z;

          x += i_dx;
          y += i_dy;
        }

      x0 += j_dx;
      y0 += j_dy;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  point_render_class->process = process;

  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->opencl_support = FALSE;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:linear-sinusoid",
    "title",              _("Linear Sinusoid"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description",        _("Generate a linear sinusoid pattern"),
    NULL);
}

#endif
