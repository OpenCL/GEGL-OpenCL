/* This file is an image processing operation for GEGL
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
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Colormap-Rotation plug-in. Exchanges two color ranges.
 *
 * Copyright (C) 1999 Sven Anders (anderss@fmi.uni-passau.de)
 *                    Based on code from Pavel Grinfeld (pavel@ml.com)
 * Copyright (C) 2011 Robert Sasu <sasu.robert@gmail.com>
 * Copyright (C) 2011 Mukund Sivaraman <muks@banu.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_color_rotate_gray_mode)
  enum_value (GEGL_COLOR_ROTATE_GRAY_TREAT_AS,  "treat-as",
              N_("Treat as this"))
  enum_value (GEGL_COLOR_ROTATE_GRAY_CHANGE_TO, "change-to",
              N_("Change to this"))
enum_end (GeglColorRotateGrayMode)

property_boolean (src_clockwise, _("Clockwise"), FALSE)
    description (_("Switch to clockwise"))

property_double (src_from, _("From"), 0.0)
    description (_("Start angle of the source color range"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_double (src_to, _("To"), 90.0)
    description (_("End angle of the source color range"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_boolean (dest_clockwise, _("Clockwise"), FALSE)
    description (_("Switch to clockwise"))

property_double (dest_from, _("From"), 0.0)
    description (_("Start angle of the destination color range"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_double (dest_to, _("To"), 90.0)
    description (_("End angle of the destination color range"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_double (threshold, _("Gray threshold"), 0.0)
    description (_("Colors with a saturation less than this will treated "
                   "as gray"))
    value_range (0.0, 1.0)

property_enum   (gray_mode, _("Gray mode"),
    GeglColorRotateGrayMode, gegl_color_rotate_gray_mode,
    GEGL_COLOR_ROTATE_GRAY_CHANGE_TO)
    description (_("Treat as this: Gray colors from above source range "
                   "will be treated as if they had this hue and saturation\n"
                   "Change to this: Change gray colors to this "
                   "hue and saturation"))

property_double (hue, _("Hue"), 0.0)
    description (_("Hue value for above gray settings"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_double (saturation, _("Saturation"), 0.0)
    description (_("Saturation value for above gray settings"))
    value_range (0.0, 1.0)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     color_rotate
#define GEGL_OP_C_SOURCE color-rotate.c

#include "gegl-op.h"

#define TWO_PI        (2 * G_PI)
#define DEG_TO_RAD(d) (((d) * G_PI) / 180.0)

static void
prepare (GeglOperation *operation)
{
  /* gamma-corrected RGB because that's what the HSV conversion
   * functions expect
   */
  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
}

static void
rgb_to_hsv (gfloat  r,
            gfloat  g,
            gfloat  b,
            gfloat *h,
            gfloat *s,
            gfloat *v)
{
  float min;
  float delta;

  *v = MAX (MAX (r, g), b);
  min = MIN (MIN (r, g), b);
  delta = *v - min;

  if (delta == 0.0f)
    {
      *h = 0.0f;
      *s = 0.0f;
    }
  else
    {
      *s = delta / *v;

      if (r == *v)
        {
          *h = (g - b) / delta;
          if (*h < 0.0f)
            *h += 6.0f;
        }
      else if (g == *v)
        {
          *h = 2.0f + (b - r) / delta;
        }
      else
        {
          *h = 4.0f + (r - g) / delta;
        }

      *h /= 6.0f;
    }
}

static void
hsv_to_rgb (gfloat  h,
            gfloat  s,
            gfloat  v,
            gfloat *r,
            gfloat *g,
            gfloat *b)
{
  if (s == 0.0)
    {
      *r = v;
      *g = v;
      *b = v;
    }
  else
    {
      int hi;
      float frac;
      float w, q, t;

      h *= 6.0;

      if (h >= 6.0)
        h -= 6.0;

      hi = (int) h;
      frac = h - hi;
      w = v * (1.0 - s);
      q = v * (1.0 - (s * frac));
      t = v * (1.0 - (s * (1.0 - frac)));

      switch (hi)
        {
        case 0:
          *r = v;
          *g = t;
          *b = w;
          break;
        case 1:
          *r = q;
          *g = v;
          *b = w;
          break;
        case 2:
          *r = w;
          *g = v;
          *b = t;
          break;
        case 3:
          *r = w;
          *g = q;
          *b = v;
          break;
        case 4:
          *r = t;
          *g = w;
          *b = v;
          break;
        case 5:
          *r = v;
          *g = w;
          *b = q;
          break;
        }
    }
}

static gfloat
angle_mod_2PI (gfloat angle)
{
  if (angle < 0)
    return angle + TWO_PI;
  else if (angle > TWO_PI)
    return angle - TWO_PI;
  else
    return angle;
}

static gfloat
angle_inside_slice (gfloat   hue,
                    gfloat   from,
                    gfloat   to,
                    gboolean cl)
{
  gint cw_ccw = cl ? -1 : 1;

  return angle_mod_2PI (cw_ccw * DEG_TO_RAD (to - hue)) /
         angle_mod_2PI (cw_ccw * DEG_TO_RAD (from - to));
}

static gboolean
is_gray (gfloat  s,
         gdouble threshold)
{
  return (s <= threshold);
}

static gfloat
linear (gfloat A,
        gfloat B,
        gfloat C,
        gfloat D,
        gfloat x)
{
  if (B > A)
    {
      if (A <= x && x <= B)
        return C + (D - C) / (B - A) * (x - A);
      else if (A <= x + TWO_PI && x + TWO_PI <= B)
        return C + (D - C) / (B - A) * (x + TWO_PI - A);
      else
        return x;
    }
  else
    {
      if (B <= x && x <= A)
        return C + (D - C) / (B - A) * (x - A);
      else if (B <= x + TWO_PI && x + TWO_PI <= A)
        return C + (D - C) / (B - A) * (x + TWO_PI - A);
      else
        return x;
    }
}

static gfloat
left_end (gfloat   from,
          gfloat   to,
          gboolean cl)
{
  gfloat alpha  = DEG_TO_RAD (from);
  gfloat beta   = DEG_TO_RAD (to);
  gint   cw_ccw = cl ? -1 : 1;

  switch (cw_ccw)
    {
    case -1:
      if (alpha < beta)
        return alpha + TWO_PI;

    default:
      return alpha; /* 1 */
    }
}

static gfloat
right_end (gfloat   from,
           gfloat   to,
           gboolean cl)
{
  gfloat alpha  = DEG_TO_RAD (from);
  gfloat beta   = DEG_TO_RAD (to);
  gint   cw_ccw = cl ? -1 : 1;

  switch (cw_ccw)
    {
    case 1:
      if (beta < alpha)
        return beta + TWO_PI;

    default:
      return beta; /* -1 */
    }
}

static void
color_rotate (GeglProperties *o,
              gfloat         *input,
              gfloat         *output)
{
  gfloat   h, s, v;
  gboolean skip = FALSE;

  rgb_to_hsv (input[0], input[1], input[2],
              &h, &s, &v);

  if (is_gray (s, o->threshold))
    {
      if (o->gray_mode == GEGL_COLOR_ROTATE_GRAY_TREAT_AS)
        {
          if (angle_inside_slice (o->hue, o->src_from, o->src_to,
                                  o->src_clockwise) <= 1)
            {
              h = DEG_TO_RAD (o->hue) / TWO_PI;
              s = o->saturation;
            }
          else
            {
              skip = TRUE;
            }
        }
      else
        {
          skip = TRUE;

          h = DEG_TO_RAD (o->hue) / TWO_PI;
          s = o->saturation;
        }
    }

  if (! skip)
    {
      h = linear (left_end (o->src_from, o->src_to, o->src_clockwise),
                  right_end (o->src_from, o->src_to, o->src_clockwise),
                  left_end (o->dest_from, o->dest_to, o->dest_clockwise),
                  right_end (o->dest_from, o->dest_to, o->dest_clockwise),
                  h * TWO_PI);

      h = angle_mod_2PI (h) / TWO_PI;
    }

  hsv_to_rgb (h, s, v,
              output, output + 1, output + 2);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                samples,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  gfloat         *input  = in_buf;
  gfloat         *output = out_buf;

  while (samples--)
    {
      color_rotate (o, input, output);

      output[3] = input[3];

      input  += 4;
      output += 4;
    }

  return  TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "categories",   "color",
    "name",         "gegl:color-rotate",
    "title",        _("Color Rotate"),
    "description",  _("Replace a range of colors with another"),
    NULL);
}

#endif
