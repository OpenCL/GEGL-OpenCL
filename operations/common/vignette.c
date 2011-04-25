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
 * Copyright 2011 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_int    (shape,    _("shape"),  0, 2, 0, _("shape to use 0=circle 1=diamond 2=square"))
gegl_chant_color (color,     _("Color"), "black", _("defaults to 'black', you can use transparency here to erase portions of an image"))
gegl_chant_double (radius,   _("radius"),  0.0, 3.0, 1.5, _("how far out vignetting goes as portion of half image diagonal"))
gegl_chant_double (softness,  _("softness"),  0.0, 1.0, 0.8, _("softness"))
gegl_chant_double (gamma,    _("gamma"),  1.0, 20.0, 1.4545, _("falloff linearity"))
gegl_chant_double (proportion, _("proportion"), 0.0, 1.0, 1.0,  _("how close we are to image proportions"))
gegl_chant_double (squeeze,   _("squeeze"), -1.0, 1.0, 0.0,  _("Aspect ratio to use, -0.5 = 1:2, 0.0 = 1:1, 0.5 = 2:1, -1.0 = 1:inf 1.0 = inf:1, this is applied after proportion is taken into account, to directly use squeeze factor as proportions, set proportion to 0.0."))

gegl_chant_double (x,        _("x"),  -1.0, 2.0, 0.5, _("Horizontal center of vignetting"))
gegl_chant_double (y,        _("y"),  -1.0, 2.0, 0.5, _("Vertical center of vignetting"))
gegl_chant_double (rotation, _("rotation"),  0.0, 360.0, 0.0, _("Rotation angle"))

#else

#define GEGL_CHANT_TYPE_POINT_FILTER
#define GEGL_CHANT_C_FILE               "vignette.c"

#include "gegl-chant.h"

#include <math.h>

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle result = {-10000000, -10000000, 20000000, 20000000};
  return result;
}

/* conversion function mapping between scale and aspect
 *
 * -1.0 = 0.0
 * -0.5 = 0.5
 *  0.0 = 1.0
 *  0.5 = 2.0
 *  1.0 = infinity
 */

static float aspect_to_scale (float aspect)
{
  if (aspect == 0.0)
    return 1.0;
  else if (aspect > 0.0)
    return tan(aspect * (G_PI/2)) + 1;
  else /* (aspect < 0.0) */
    return 1.0/(tan((-aspect) * (G_PI/2)) + 1);
}

#if 0
static float scale_to_aspect (float scale)
{
  if (scale == 1.0)
    return 0.0;
  else if (scale > 1.0)
    return atan (scale-1) / (G_PI/2);
  else /* scale < 1.0 */
    return -atan(1.0/scale- 1) / (G_PI/2);
}
#endif

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat     *in_pixel =  in_buf;
  gfloat     *out_pixel = out_buf;
  gfloat      scale;
  gfloat      radius0, radius1;
  gint        x, y;
  gint        midx, midy;
  GeglRectangle *bounds = gegl_operation_source_get_bounding_box (
                            operation, "input");
  gfloat length = hypot (bounds->width, bounds->height)/2;
  gfloat rdiff;
  gfloat cost, sint;


  gfloat      color[4];

  scale = bounds->width / (1.0 * bounds->height);
  scale = scale * (o->proportion) + 1.0 * (1.0-o->proportion);

  scale *= aspect_to_scale (o->squeeze);

  length = (bounds->width/2.0);

  if (scale > 1.0)
    length /= scale;

  gegl_color_get_rgba4f (o->color, color);

  for (x=0; x<3; x++)   /* premultiply */
    color[x] *= color[3];

  radius0 = o->radius * (1.0-o->softness);
  radius1 = o->radius;
  rdiff = radius1-radius0;
  if (fabs (rdiff) < 0.0001)
    rdiff = 0.0001;

  midx = bounds->x + bounds->width * o->x;
  midy = bounds->y + bounds->height * o->y;

  /* rotate coordinates around midx/midy,.
   */

  cost = cos(-o->rotation * (G_PI*2/360.0));/* caching constants outside loop */
  sint = sin(-o->rotation * (G_PI*2/360.0));/* caching constants outside loop */

  x = roi->x;
  y = roi->y;

  while (n_pixels--)
    {
      gfloat strength;
      gfloat u, v;
      gint c;

      /* this can be partially optimized out of the loop, ending up being
         additions..
       */
      u = cost * (x-midx) - sint * (y-midy) + midx;
      v = sint * (x-midx) + cost * (y-midy) + midy;

      if (length == 0.0)
        strength = 0.5;
      else
        {
          switch (o->shape)
          {
            case 0:  /* circle */
#define POW2(a) ((a)*(a))
              strength = sqrt( POW2((u-midx) / scale) +
                               POW2(v-midy)) /length;
#undef POW2
              break;
             case 1: /* square */
              strength =
                  MAX(ABS(u-midx) / scale, ABS(v-midy) )/length;
              break;
             case 2: /* diamond */
              strength = (ABS(u-midx) / scale + ABS(v-midy) )/length;
              break;
             default:
              strength = 1.0;
          }
          strength = (strength-radius0) /rdiff;
        }

      if (strength<0.0) strength = 0.0;
      if (strength>1.0) strength = 1.0;

      if (o->gamma != 1.0)
        strength = powf(strength, o->gamma);

      for (c=0;c<4;c++)
        out_pixel[c]=in_pixel[c] * (1.0-strength) + color[c] * strength;

      out_pixel += 4;
      in_pixel += 4;

      /* update x and y coordinates */
      if (++x>=roi->x + roi->width)
        {
          x=roi->x;
          y++;
        }
    }

  return  TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare = prepare;
  operation_class->no_cache = TRUE;

  operation_class->name        = "gegl:vignette";
  operation_class->categories  = "render";
  operation_class->description = _("A vignetting op, applies a vignette to an image. Simulates the luminance fall off at edge of exposed film, and some other fuzzier border effects that can naturally occur with analoge photograpy.");
}

#endif
