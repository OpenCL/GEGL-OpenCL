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
 * Copyright 2010 Barak Itkin <lightningismyname@gmail.org>
 *
 * Based on "Whirl and Pinch" GIMP plugin
 * Copyright (C) 1997 Federico Mena Quintero
 * Copyright (C) 1997 Scott Goehring
 *
 * The workshop/mirrors.c operation by Alexia Death and Øyvind Kolås
 * was used as a template for this op file.
 */

/* TODO: Find some better algorithm to calculate the roi for each dest
 *       rectangle. Right now it simply asks for the entire image...
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (whirl,  _("Whirl"), -G_MAXDOUBLE, G_MAXDOUBLE, 90, _("Whirl angle (degrees)"))

gegl_chant_double (pinch,  _("Pinch"), -1, 1, 0, _("Pinch amount"))

gegl_chant_double (radius,  _("Radius"), 0, 2, 1, _("Radius (1.0 is the largest circle that fits in the image, and 2.0 goes all the way to the corners)"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "whirl-pinch.c"

#include "gegl-chant.h"
#include <math.h>

/* This function is a slightly modified version from the one in the original plugin */
static gboolean
calc_undistorted_coords (gdouble  wx,      gdouble  wy,
                         gdouble  cen_x,   gdouble  cen_y,
                         gdouble  scale_x, gdouble  scale_y,
                         gdouble  whirl,   gdouble  pinch,   gdouble  wpradius,
                         gdouble *x,       gdouble *y)
{
  gdouble  dx, dy;
  gdouble  d, factor;
  gdouble  dist;
  gdouble  ang, sina, cosa;
  gboolean inside;

  gdouble  radius = MAX(cen_x, cen_y);
  gdouble  radius2 = radius * radius * wpradius;
  /* Distances to center, scaled */

  dx = (wx - cen_x) * scale_x;
  dy = (wy - cen_y) * scale_y;

  /* Distance^2 to center of *circle* (scaled ellipse) */

  d = dx * dx + dy * dy;

  /*  If we are inside circle, then distort.
   *  Else, just return the same position
   */

  inside = (d < radius2);

  /* If d is 0, then we are exactly at the center, so the transform has
   * no effect. The original version of the gimp plugin simply created
   * an offset of the original points to avoid this issue, but that is
   * less accurate...
   */
  if (inside && d > 0)
    {
      dist = sqrt(d / wpradius) / radius;

      /* Pinch */

      factor = pow (sin (G_PI_2 * dist), -pinch);

      dx *= factor;
      dy *= factor;

      /* Whirl */

      factor = 1.0 - dist;

      ang = whirl * factor * factor;

      sina = sin (ang);
      cosa = cos (ang);

      *x = (cosa * dx - sina * dy) / scale_x + cen_x;
      *y = (sina * dx + cosa * dy) / scale_y + cen_y;
    }
  else
    {
      *x = wx;
      *y = wy;
    }

  return inside;
}

/* Apply the actual transform */

static void
apply_whirl_pinch (gdouble whirl, gdouble pinch, gdouble radius,
                   gdouble cen_x, gdouble cen_y,
                   const Babl    *format,
                   GeglBuffer *src,
                   GeglRectangle *in_boundary,
                   GeglBuffer *dst,
                   GeglRectangle *boundary,
                   const GeglRectangle *roi)
{
  gfloat *dst_buf;
  gint row, col;
  gdouble scale_x, scale_y;
  gdouble cx, cy;
  GeglSampler *sampler;

  /* Get buffer in which to place dst pixels. */
  dst_buf = g_new0 (gfloat, roi->width * roi->height * 4);

  whirl = whirl * G_PI / 180;

  scale_x = 1.0;
  scale_y = roi->width / (gdouble) roi->height;
  sampler = gegl_buffer_sampler_new (src, babl_format ("RaGaBaA float"),
                                     GEGL_SAMPLER_LOHALO);

  for (row = 0; row < roi->height; row++) {
    for (col = 0; col < roi->width; col++) {
        GeglMatrix2 scale;
#define gegl_unmap(u,v,du,dv) \
        { \
          calc_undistorted_coords (u, v,\
                                   cen_x, cen_y,\
                                   scale_x, scale_y,\
                                   whirl, pinch, radius,\
                                   &cx, &cy);\
          du=cx;dv=cy;\
        }
        gegl_sampler_compute_scale (scale, roi->x + col, roi->y + row);
        gegl_unmap (roi->x + col, roi->y + row, cx, cy);

        gegl_sampler_get (sampler, cx, cy, &scale, &dst_buf[(row * roi->width + col) * 4]);
    } /* for */
  } /* for */

  /* Store dst pixels. */
  gegl_buffer_set (dst, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  gegl_buffer_flush(dst);

  g_free (dst_buf);
  g_object_unref (sampler);
}

/*****************************************************************************/

/* Compute the region for which this operation is defined.
 */
static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (!in_rect){
    return result;
  }
  else
    return *in_rect;
}

/* Compute the input rectangle required to compute the specified region of interest (roi).
 */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle *result = gegl_operation_source_get_bounding_box (operation, "input");

  return *result;
}

/* Specify the input and output buffer formats.
 */
static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input", babl_format ("RaGaBaA float"));
  gegl_operation_set_format (operation, "output", babl_format ("RaGaBaA float"));
}

/* Perform the specified operation.
 */
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle boundary = gegl_operation_get_bounding_box (operation);
  const Babl *format = babl_format ("RaGaBaA float");

  apply_whirl_pinch (o->whirl,
                     o->pinch,
                     o->radius,
                     boundary.width / 2.0,
                     boundary.height / 2.0,
                     format,
                     input,
                    &boundary,
                     output,
                    &boundary,
                     result);
  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:whirl-pinch",
  "categories"  , "distort",
  "description" ,
        _("Applies whirling and pinching on the image"),
        NULL);
}

#endif
