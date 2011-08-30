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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_boolean (s_cl, _("Clockwise"), FALSE,
                    _("Switch to clockwise"))
gegl_chant_int (s_fr, _("From:"), 0, 360, 0,
                _("Starting angle for the color rotation"))
gegl_chant_int (s_to, _("To:"), 0, 360, 0,
                _("End angle for the color rotation"))
gegl_chant_boolean (d_cl, _("Clockwise"), FALSE,
                    _("Switch to clockwise"))
gegl_chant_int (d_fr, _("From:"), 0, 360, 0,
                _("Starting angle for the color rotation"))
gegl_chant_int (d_to, _("To:"), 0, 360, 0,
                _("End angle for the color rotation"))

gegl_chant_boolean (gray, _("Grayscale"), FALSE,
                    _("Choose in case of grayscale images"))
gegl_chant_double (hue, _("Hue"), 0.0, 2.0, 0.0,
                   _("The value of hue"))
gegl_chant_double (saturation, _("Saturation"), 0.0, 1.0, 0.0,
                   _("The value of saturation"))
gegl_chant_boolean (change, _("Change/Treat to this"), FALSE,
                   _("Change/Treat to this"))
gegl_chant_double (threshold, _("Threshold"), 0.0, 1.0, 0.0,
                   _("The value of gray threshold"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "color-rotate.c"

#include "gegl-chant.h"
#include <stdio.h>
#include <math.h>

#define TP (2*G_PI)
#define DEG_TO_RAD(d) (((d) * G_PI) / 180.0)

static void prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", 
                             babl_format ("RGBA float"));
  
  gegl_operation_set_format (operation, "output",
                             babl_format ("RGBA float"));
}


static void
gegl_rgb_to_hsv_double (gdouble *red,
			gdouble *green,
			gdouble *blue)
{
  gdouble r, g, b;
  gdouble h, s, v;
  gdouble min, max;
  gdouble delta;

  r = *red;
  g = *green;
  b = *blue;

  h = 0.0; 

  if (r > g)
    {
      max = MAX (r, b);
      min = MIN (g, b);
    }
  else
    {
      max = MAX (g, b);
      min = MIN (r, b);
    }

  v = max;

  if (max != 0.0)
    s = (max - min) / max;
  else
    s = 0.0;

  if (s == 0.0)
    {
      h = 0.0;
    }
  else
    {
      delta = max - min;

      if (r == max)
	h = (g - b) / delta;
      else if (g == max)
	h = 2 + (b - r) / delta;
      else if (b == max)
	h = 4 + (r - g) / delta;

      h /= 6.0;

      if (h < 0.0)
	h += 1.0;
      else if (h > 1.0)
	h -= 1.0;
    }

  *red   = h;
  *green = s;
  *blue  = v;
}

static void
gegl_hsv_to_rgb4 (gfloat  *rgb, 
		  gdouble  hue,
		  gdouble  saturation,
		  gdouble  value)
{
  gdouble h, s, v;
  gdouble f, p, q, t;

  if (saturation == 0.0)
    {
      hue        = value;
      saturation = value;
      value      = value;
    }
  else
    {
      h = hue * 6.0;
      s = saturation;
      v = value;

      if (h == 6.0)
	h = 0.0;

      f = h - (gint) h;
      p = v * (1.0 - s);
      q = v * (1.0 - s * f);
      t = v * (1.0 - s * (1.0 - f));

      switch ((int) h)
	{
	case 0:
	  hue        = v;
	  saturation = t;
	  value      = p;
	  break;

	case 1:
	  hue        = q;
	  saturation = v;
	  value      = p;
	  break;

	case 2:
	  hue        = p;
	  saturation = v;
	  value      = t;
	  break;

	case 3:
	  hue        = p;
	  saturation = q;
	  value      = v;
	  break;

	case 4:
	  hue        = t;
	  saturation = p;
	  value      = v;
	  break;

	case 5:
	  hue        = v;
	  saturation = p;
	  value      = q;
	  break;
	}
    }

  rgb[0] = hue;
  rgb[1] = saturation;
  rgb[2] = value;
}

static gfloat
angle_mod_2PI (gfloat angle)
{
  if (angle < 0)
    return angle + TP;
  else if (angle > TP)
    return angle - TP;
  else
    return angle;
}

static gfloat
angle_inside_slice (gfloat   hue,
                    gint     from,
                    gint     to,
                    gboolean cl)
{
   gint cw_ccw = 1;
   if (!cl) cw_ccw = -1;

   return angle_mod_2PI (cw_ccw * DEG_TO_RAD(to - hue)) /
          angle_mod_2PI (cw_ccw * DEG_TO_RAD(from - to));
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
      if (A<=x && x<=B)
        return C+(D-C)/(B-A)*(x-A);
      else if (A<=x+TP && x+TP<=B)
        return C+(D-C)/(B-A)*(x+TP-A);
      else
        return x;
    }
  else
    {
      if (B<=x && x<=A)
        return C+(D-C)/(B-A)*(x-A);
      else if (B<=x+TP && x+TP<=A)
        return C+(D-C)/(B-A)*(x+TP-A);
      else
        return x;
    }
}

static gfloat
left_end (gint     from,
          gint     to,
          gboolean cl)
{
  gfloat alpha  = DEG_TO_RAD (from);
  gfloat beta   = DEG_TO_RAD (to);
  gint   cw_ccw = cl ? 1 : -1;

  switch (cw_ccw)
    {
    case (-1):
      if (alpha < beta) return alpha + TP;

    default:
      return alpha; /* 1 */
    }
}

static gfloat
right_end (gint     from,
           gint     to,
           gboolean cl)
{
  gfloat alpha  = DEG_TO_RAD (from);
  gfloat beta   = DEG_TO_RAD (to);
  gint   cw_ccw = cl ? 1 : -1;

  switch (cw_ccw)
    {
    case 1:
      if (beta < alpha) return beta + TP;

    default:
      return beta; /* -1 */
    }
}

static void
color_rotate (gfloat     *src,
              gint        offset,
              GeglChantO *o)
{
  gdouble  H,S,V;
  gboolean skip = FALSE;
  gfloat   color[4];
  gint     i; 


  H = src[offset];
  S = src[offset + 1];
  V = src[offset + 2];

  gegl_rgb_to_hsv_double (&H, &S, &V);

  if (is_gray (S, o->threshold))
    {
      if (o->change == FALSE)
         {
            if (angle_inside_slice (o->hue, o->s_fr, o->s_to, o->s_cl) <= 1)
              {
                H = o->hue / TP;
                S = o->saturation;
              }
            else
              {
                skip = TRUE;
              }
         }
      else
         {
            skip = TRUE;
            gegl_hsv_to_rgb4 (color, o->hue / TP, o->saturation, V);
            color[3] = src[offset + 3];
         }
    }

   if (! skip)
     {
     H = linear (left_end (o->s_fr, o->s_to, o->s_cl),
                 right_end (o->s_fr, o->s_to, o->s_cl),
                 left_end (o->d_fr, o->d_to, o->d_cl),
                 right_end (o->d_fr, o->d_to, o->d_cl),
                 H * TP);
     H = angle_mod_2PI (H) / TP;
     gegl_hsv_to_rgb4 (color, H, S, V);
     color[3] = src[offset + 3];
     }

  for (i = 0; i < 4; i++)
     src[offset + i] = color[i];

}


static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO              *o            = GEGL_CHANT_PROPERTIES (operation);
  Babl                    *format       = babl_format ("RGBA float");

  gfloat *src_buf;
  gint    x;

  src_buf = g_new0 (gfloat, result->width * result->height * 4);

  format = babl_format ("RGBA float");

  gegl_buffer_get (input, 1.0, result, format, src_buf, GEGL_AUTO_ROWSTRIDE);

  for (x = 0; x < result->width * result->height; x++)
      color_rotate (src_buf, 4 * x, o);

  gegl_buffer_set (output, result, format, src_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  operation_class->categories  = "color";
  operation_class->name        = "gegl:color-rotate";
  operation_class->description = _("Rotate colors on the image.");
}

#endif
