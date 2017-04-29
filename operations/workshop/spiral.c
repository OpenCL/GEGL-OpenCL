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

#ifdef GEGL_PROPERTIES

enum_start (gegl_spiral_type)
  enum_value (GEGL_SPIRAL_TYPE_LINEAR,      "linear",      N_("Linear"))
  enum_value (GEGL_SPIRAL_TYPE_LOGARITHMIC, "logarithmic", N_("Logarithmic"))
enum_end (GeglSpiralType)

property_enum (type, _("Type"),
               GeglSpiralType, gegl_spiral_type,
               GEGL_SPIRAL_TYPE_LINEAR)
  description (_("Spiral type"))

property_double (x, _("X"), 0.5)
  description (_("Spiral origin X coordinate"))
  ui_range    (0.0, 1.0)
  ui_meta     ("unit", "relative-coordinate")
  ui_meta     ("axis", "x")

property_double (y, _("Y"), 0.5)
  description (_("Spiral origin Y coordinate"))
  ui_range    (0.0, 1.0)
  ui_meta     ("unit", "relative-coordinate")
  ui_meta     ("axis", "y")

property_double (radius, _("Radius"), 100.0)
  description (_("Spiral radius"))
  value_range (1.0, G_MAXDOUBLE)
  ui_range    (1.0, 400.0)
  ui_meta     ("unit", "pixel-distance")

property_double (base, _("Base"), 2.0)
  description (_("Logarithmic spiral base"))
  value_range (1.0, G_MAXDOUBLE)
  ui_range    (1.0, 20.0)
  ui_gamma    (2.0)

property_double (balance, _("Balance"), 0.0)
  description (_("Area balance between the two colors"))
  value_range (-1.0, 1.0)

property_double (rotation, _("Rotation"), 0.0)
  description (_("Spiral rotation"))
  value_range (0.0, 360.0)
  ui_meta     ("unit", "degree")

enum_start (gegl_spiral_direction)
  enum_value (GEGL_SPIRAL_DIRECTION_CLOCKWISE,        "cw",  N_("Clockwise"))
  enum_value (GEGL_SPIRAL_DIRECTION_COUNTERCLOCKWISE, "ccw", N_("Counter-clockwise"))
enum_end (GeglSpiralDirection)

property_enum (direction, _("Direction"),
               GeglSpiralDirection, gegl_spiral_direction,
               GEGL_SPIRAL_DIRECTION_CLOCKWISE)
  description (_("Spiral swirl direction"))

property_color  (color1, _("Color 1"), "black")
  ui_meta     ("role", "color-primary")

property_color  (color2, _("Color 2"), "white")
  ui_meta     ("role", "color-secondary")

property_int    (width, _("Width"), 1024)
  description (_("Width of the generated buffer"))
  value_range (0, G_MAXINT)
  ui_range    (0, 4096)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "x")
  ui_meta     ("role", "output-extent")

property_int    (height, _("Height"), 768)
  description (_("Height of the generated buffer"))
  value_range (0, G_MAXINT)
  ui_range    (0, 4096)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "y")
  ui_meta     ("role", "output-extent")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     spiral
#define GEGL_OP_C_SOURCE spiral.c

#include "gegl-op.h"
#include <math.h>

#define RAD_TO_REV ((gfloat) (1.0 / (2.0 * G_PI)))

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static inline void
blend (const gfloat *color1,
       const gfloat *color2,
       gfloat        a,
       gfloat       *result)
{
  if (a == 0.0f)
    {
      memcpy (result, color1, 4 * sizeof (gfloat));
    }
  else if (a == 1.0f)
    {
      memcpy (result, color2, 4 * sizeof (gfloat));
    }
  else
    {
      gfloat alpha = color1[3] + a * (color2[3] - color1[3]);
      gint   c;

      if (alpha)
        {
          gfloat ratio = a * color2[3] / alpha;

          for (c = 0; c < 3; c++)
            result[c] = color1[c] + ratio * (color2[c] - color1[c]);
        }
      else
        {
          memcpy (result, color1, 3 * sizeof (gfloat));
        }

      result[3] = alpha;
    }
}

static void
process_linear (gfloat       *out,
                gint          width,
                gint          height,
                gfloat        x0,
                gfloat        y0,
                gfloat        radius,
                gfloat        thickness,
                gfloat        angle,
                gboolean      clockwise,
                const gfloat *color1,
                const gfloat *color2)
{
  gfloat lim;
  gfloat x;
  gfloat y;
  gint   i;
  gint   j;

  if (thickness > 0.5)
    {
      const gfloat *temp;

      thickness = 1.0 - thickness;
      angle     = fmod (angle + thickness, 1.0);

      temp   = color1;
      color1 = color2;
      color2 = temp;
    }

  if (thickness == 0.0 || radius == 1.0)
    {
      gfloat color[4];

      blend (color2, color1, thickness, color);

      gegl_memset_pattern (out, color, sizeof (color), width * height);

      return;
    }

  lim = thickness * radius;

  y = y0;

  for (j = height; j; j--, y++)
    {
      x = x0;

      for (i = width; i; i--, x++)
        {
          gfloat r;
          gfloat t;
          gfloat s;
          gfloat a;

          r = sqrtf (x * x + y * y);
          t = RAD_TO_REV * atan2f (clockwise ? y : -y, x) - angle;

          t += t < 0.0f;

          s = t * radius;

          if (r >= s)
            {
              r = fmodf (r - s, radius);

              if (r < 0.5f)
                {
                  a = MIN (lim, r + 0.5f);
                }
              else if (r > radius - 0.5f)
                {
                  a = MAX (lim - (r - 0.5f), 0.0f) +
                      MIN (lim, (r + 0.5f) - radius);
                }
              else
                {
                  a = CLAMP (lim - (r - 0.5f), 0.0f, 1.0f);
                }
            }
          else
            {
              gfloat l = lim - (radius - s);

              a = CLAMP ((r + 0.5f) - s, 0.0f, lim);

              if (t <= 0.5f)
                {
                  if (r < 0.5f)
                    {
                      a += CLAMP (l + radius / 2.0f, 0.0f, 0.5f - r);
                    }
                }
              else
                {
                  if (r < 0.5f)
                    {
                      a += CLAMP (l, 0.0f, r + 0.5f) +
                           CLAMP ((0.5f - r) - (s - radius / 2.0f), 0.0f, lim);
                    }
                  else
                    {
                      a += CLAMP (l - (r - 0.5f), 0.0f, 1.0f);
                    }
                }
            }

          blend (color2, color1, a, out);

          out += 4;
        }
    }
}

static void
process_logarithmic (gfloat       *out,
                     gint          width,
                     gint          height,
                     gfloat        x0,
                     gfloat        y0,
                     gfloat        radius,
                     gfloat        base,
                     gfloat        thickness,
                     gfloat        angle,
                     gboolean      clockwise,
                     const gfloat *color1,
                     const gfloat *color2)
{
  gfloat log_radius;
  gfloat base_inv;
  gfloat log_base;
  gfloat log_base_inv;
  gfloat lim;
  gfloat ratio;
  gfloat x;
  gfloat y;
  gint   i;
  gint   j;

  if (thickness == 0.0 || thickness == 1.0 || base == 1.0)
    {
      const gfloat *color;

      color = thickness > 0.5f ? color1 : color2;

      gegl_memset_pattern (out, color, 4 * sizeof (gfloat), width * height);

      return;
    }

  log_radius   = log (radius);
  base_inv     = 1.0 / base;
  log_base     = log (base);
  log_base_inv = 1.0 / log_base;
  lim          = exp (log_base * thickness);
  ratio        = (lim - 1.0) / (base - 1.0);

  y = y0;

  for (j = height; j; j--, y++)
    {
      x = x0;

      for (i = width; i; i--, x++)
        {
          gfloat r;
          gfloat t;
          gfloat a;
          gfloat s;
          gfloat l;
          gfloat S;

          r = sqrtf (x * x + y * y);
          t = RAD_TO_REV * atan2f (clockwise ? y : -y, x) - angle;

          t += t < 0.0f;

          s = log_base_inv * (logf (r) - log_radius);
          s = expf (log_base * (floorf (s - t) + t) + log_radius);

          l = s * lim;
          S = s * base;

          if (r >= s + 0.5f)
            {
              a = CLAMP (l - (r - 0.5f), 0.0f, 1.0f);

              if (r > S - 0.5f)
                a += MIN (l * base - S, r + 0.5f - S);
            }
          else
            {
              if (s - s * base_inv >= 0.5f)
                {
                  a  = MIN (l - s, r + 0.5f - s);
                  a += MAX (l * base_inv - (r - 0.5f), 0.0f);

                  if (r > S - 0.5f)
                    a += MIN (l * base - S, r + 0.5f - S);
                }
              else
                {
                  a = ratio;
                }
            }

          blend (color2, color1, a, out);

          out += 4;
        }
    }
}

static gboolean
process (GeglOperation       *operation,
         void                *out,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o         = GEGL_PROPERTIES (operation);
  const Babl     *format    = babl_format ("R'G'B'A float");
  gfloat          scale     = 1.0 / (1 << level);
  gfloat          x0;
  gfloat          y0;
  gfloat          radius    = scale * o->radius;
  gfloat          thickness = (1.0 - o->balance) / 2.0;
  gfloat          angle     = o->rotation / 360.0;
  gboolean        clockwise = o->direction == GEGL_SPIRAL_DIRECTION_CLOCKWISE;
  gfloat          color1[4];
  gfloat          color2[4];

  x0 = roi->x -
       scale * gegl_coordinate_relative_to_pixel (o->x, o->width)  + 0.5;
  y0 = roi->y -
       scale * gegl_coordinate_relative_to_pixel (o->y, o->height) + 0.5;

  if (clockwise)
    angle = 1.0 - angle;

  angle = fmod (angle + thickness / 2.0, 1.0);

  gegl_color_get_pixel (o->color1, format, color1);
  gegl_color_get_pixel (o->color2, format, color2);

  switch (o->type)
    {
    case GEGL_SPIRAL_TYPE_LINEAR:
      process_linear (out, roi->width, roi->height,
                      x0, y0, radius, thickness, angle, clockwise,
                      color1, color2);
      break;

    case GEGL_SPIRAL_TYPE_LOGARITHMIC:
      process_logarithmic (out, roi->width, roi->height,
                           x0, y0, radius, o->base, thickness, angle, clockwise,
                           color1, color2);
      break;

    default:
      g_return_val_if_reached (FALSE);
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
    "name",               "gegl:spiral",
    "title",              _("Spiral"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description",        _("Spiral renderer"),
    NULL);
}

#endif
