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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2010 Alexia Death
 *
 * Based on "Kaleidoscope" GIMP plugin
 * Copyright (C) 1999, 2002 Kelly Martin, updated 2005 by Matthew Plough
 * kelly@gimp.org
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_double (m_angle, _("Mirror rotation"), 0.0)
    description (_("Rotation applied to the mirrors"))
    value_range (0.0, 180.0)
    ui_meta     ("unit", "degree")

property_double (r_angle, _("Result rotation"), 0.0)
    description (_("Rotation applied to the result"))
    value_range (0.0, 360.0)
    ui_meta     ("unit", "degree")

property_int    (n_segs, _("Mirrors"), 6)
    description (_("Number of mirrors to use"))
    value_range (2, 24)

property_double (c_x, _("Offset X"), 0.5)
    description (_("position of symmetry center in output"))
    value_range (0.0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "x")

property_double (c_y, _("Offset Y"), 0.5)
    description (_("position of symmetry center in output"))
    value_range (0.0, 1.0)
    ui_meta     ("unit", "relative-coordinate")
    ui_meta     ("axis", "y")

property_double (o_x, _("Center X"), 0.0)
    description (_("X axis ratio for the center of mirroring"))
    value_range (-1.0, 1.0)
    ui_meta     ("unit", "relative-coordinate")

property_double (o_y, _("Center Y"), 0.0)
    description (_("Y axis ratio for the center of mirroring"))
    value_range (-1.0, 1.0)
    ui_meta     ("unit", "relative-coordinate")

property_double (trim_x, _("Trim X"), 0.0)
    description (_("X axis ratio for trimming mirror expanse"))
    value_range (0.0, 0.5)

property_double (trim_y, _("Trim Y"), 0.0)
    description (_("Y axis ratio for trimming mirror expanse"))
    value_range (0.0, 0.5)

property_double (input_scale, _("Zoom"), 100.0)
    description (_("Scale factor to make rendering size bigger"))
    value_range (0.1, 100.0)

property_double (output_scale, _("Expand"), 1.0)
    description (_("Scale factor to make rendering size bigger"))
    value_range (0.0, 100.0)

property_boolean (clip, _("Clip result to input size"), TRUE)

property_boolean (warp, _("Wrap input"), TRUE)
    description (_("Fill full output area"))


#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     mirrors
#define GEGL_OP_C_SOURCE mirrors.c

#include "gegl-op.h"
#include <math.h>

#if 0
#define TRACE       /* Define this to see basic tracing info. */
#endif

#if 0
#define DO_NOT_USE_BUFFER_SAMPLE       /* Define this to disable buffer sample.*/
#endif

static int
calc_undistorted_coords(double wx, double wy,
                        double angle1, double angle2, int nsegs,
                        double cen_x, double cen_y,
                        double off_x, double off_y,
                        double *x, double *y)
{
  double dx, dy;
  double r, ang;

  double awidth = G_PI/nsegs;
  double mult;

  dx = wx - cen_x;
  dy = wy - cen_y;

  r = sqrt(dx*dx+dy*dy);
  if (r == 0.0) {
    *x = wx + off_x;
    *y = wy + off_y;
    return TRUE;
  }

  ang = atan2(dy,dx) - angle1 - angle2;
  if (ang<0.0) ang = 2*G_PI - fmod (fabs (ang), 2*G_PI);

  mult = ceil(ang/awidth) - 1;
  ang = ang - mult*awidth;
  if (((int) mult) % 2 == 1) ang = awidth - ang;
  ang = ang + angle1;

  *x = r*cos(ang) + off_x;
  *y = r*sin(ang) + off_y;

  return TRUE;
} /* calc_undistorted_coords */

/* Apply the actual transform */

static void
apply_mirror (double               mirror_angle,
              double               result_angle,
              int                  nsegs,
              double               cen_x,
              double               cen_y,
              double               off_x,
              double               off_y,
              double               input_scale,
              gboolean             clip,
              gboolean             warp,
              const Babl          *format,
              GeglBuffer          *src,
              GeglRectangle       *in_boundary,
              GeglBuffer          *dst,
              GeglRectangle       *boundary,
              const GeglRectangle *roi,
              gint                 level)
{
  gfloat *dst_buf;
  gint    row, col;
  gdouble cx, cy;

  /* Get src pixels. */

  #ifdef TRACE
    g_warning ("> mirror marker1, boundary x:%d, y:%d, w:%d, h:%d, center: (%f, %f) offset: (%f, %f)", boundary->x, boundary->y, boundary->width, boundary->height, cen_x, cen_y, off_x,off_y );
  #endif

  #ifdef DO_NOT_USE_BUFFER_SAMPLE
    src_buf = g_new0 (gfloat, boundary->width * boundary->height * 4);
    gegl_buffer_get (src, 1.0, boundary, format, src_buf, GEGL_AUTO_ROWSTRIDE);
  #endif
  /* Get buffer in which to place dst pixels. */
  dst_buf = g_new0 (gfloat, roi->width * roi->height * 4);

  mirror_angle   = mirror_angle * G_PI / 180;
  result_angle   = result_angle * G_PI / 180;

  for (row = 0; row < roi->height; row++) {
    for (col = 0; col < roi->width; col++) {
        calc_undistorted_coords(roi->x + col + 0.01, roi->y + row - 0.01, mirror_angle, result_angle,
                                  nsegs,
                                  cen_x, cen_y,
                                  off_x * input_scale, off_y * input_scale,
                                  &cx, &cy);


  /* apply scale*/
  cx = in_boundary->x + (cx - in_boundary->x) / input_scale;
  cy = in_boundary->y + (cy - in_boundary->y) / input_scale;

        /*Warping*/
        if (warp)
          {
            double dx = cx - in_boundary->x;
            double dy = cy - in_boundary->y;

            double width_overrun = ceil ((dx) / (in_boundary->width)) ;
            double height_overrun = ceil ((dy) / (in_boundary->height));

            if (cx <= (in_boundary->x))
              {
                if ( fabs (fmod (width_overrun, 2)) < 1.0)
                  cx = in_boundary->x - fmod (dx, in_boundary->width);
                else
                  cx = in_boundary->x + in_boundary->width + fmod (dx, in_boundary->width);
              }

            if (cy <= (in_boundary->y))
              {
                if ( fabs (fmod (height_overrun, 2)) < 1.0)
                  cy = in_boundary->y + fmod (dy, in_boundary->height);
                else
                  cy = in_boundary->y + in_boundary->height - fmod (dy, in_boundary->height);
              }

            if (cx >= (in_boundary->x + in_boundary->width))
              {
                if ( fabs (fmod (width_overrun, 2)) < 1.0)
                  cx = in_boundary->x + in_boundary->width - fmod (dx, in_boundary->width);
                else
                  cx = in_boundary->x + fmod (dx, in_boundary->width);
              }

            if (cy >= (in_boundary->y + in_boundary->height))
              {
                if ( fabs (fmod (height_overrun, 2)) < 1.0)
                  cy = in_boundary->y + in_boundary->height - fmod (dy, in_boundary->height);
                else
                  cy = in_boundary->y + fmod (dy, in_boundary->height);
              }
          }
        else /* cliping */
          {
            if (cx < boundary->x)
              cx = 0;
            if (cy < boundary->x)
              cy = 0;

            if (cx >= boundary->width)
              cx = boundary->width - 1;
            if (cy >= boundary->height)
              cy = boundary->height -1;
        }


        /* Top */
#ifdef DO_NOT_USE_BUFFER_SAMPLE

        if (cx >= 0.0)
          ix = (int) cx;
        else
          ix = -((int) -cx + 1);

        if (cy >= 0.0)
          iy = (int) cy;
        else
          iy = -((int) -cy + 1);

        spx_pos = (iy * boundary->width + ix) * 4;
#endif



#ifndef DO_NOT_USE_BUFFER_SAMPLE
        gegl_buffer_sample_at_level (src, cx, cy, NULL, &dst_buf[(row * roi->width + col) * 4], format, level, GEGL_SAMPLER_LINEAR, GEGL_ABYSS_NONE);
#endif

#ifdef DO_NOT_USE_BUFFER_SAMPLE
         dst_buf[dpx_pos]     = src_buf[spx_pos];
         dst_buf[dpx_pos + 1] = src_buf[spx_pos + 1];
         dst_buf[dpx_pos + 2] = src_buf[spx_pos + 2];
         dst_buf[dpx_pos + 3] = src_buf[spx_pos + 3];
#endif

    } /* for */
  } /* for */

    gegl_buffer_sample_cleanup(src);

  /* Store dst pixels. */
  gegl_buffer_set (dst, roi, 0, format, dst_buf, GEGL_AUTO_ROWSTRIDE);

  /* Free acquired storage. */
#ifdef DO_NOT_USE_BUFFER_SAMPLE
  g_free (src_buf);
#endif
  g_free (dst_buf);

}

/*****************************************************************************/

static GeglRectangle
get_effective_area (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);
  gdouble xt = o->trim_x * in_rect->width;
  gdouble yt = o->trim_y * in_rect->height;

  gegl_rectangle_copy(&result, in_rect);

  /*Applying trims*/

  result.x = result.x + xt;
  result.y = result.y + yt;
  result.width = result.width - xt;
  result.height = result.height - yt;

  return result;
}

/* Compute the region for which this operation is defined.
 */
static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (!in_rect){
        return result;
  }

  if (o->clip) {
    gegl_rectangle_copy(&result, in_rect);
  }
  else {
    result.x = in_rect->x;
    result.y = in_rect->y;
    result.width = result.height = sqrt (in_rect->width * in_rect->width + in_rect->height * in_rect->height) * MAX ((o->o_x + 1),  (o->o_y + 1)) * 2;
  }

  result.width = result.width * o->output_scale;
  result.height = result.height * o->output_scale;

  #ifdef TRACE
    g_warning ("< get_bounding_box result = %dx%d+%d+%d", result.width, result.height, result.x, result.y);
  #endif
  return result;
}

/* Compute the input rectangle required to compute the specified region of interest (roi).
 */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglRectangle  result = get_effective_area (operation);

  #ifdef TRACE
    g_warning ("> get_required_for_output src=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
    if (roi)
      g_warning ("  ROI == %dx%d+%d+%d", roi->width, roi->height, roi->x, roi->y);
  #endif

  return result;
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
  GeglProperties *o            = GEGL_PROPERTIES (operation);
  GeglRectangle   boundary     = gegl_operation_get_bounding_box (operation);
  GeglRectangle   eff_boundary = get_effective_area (operation);
  const Babl     *format       = babl_format ("RaGaBaA float");

#ifdef DO_NOT_USE_BUFFER_SAMPLE
 g_warning ("NOT USING BUFFER SAMPLE!");
#endif
  apply_mirror (o->m_angle,
                o->r_angle,
                o->n_segs,
                o->c_x * boundary.width,
                o->c_y * boundary.height,
                o->o_x * (eff_boundary.width  - eff_boundary.x) + eff_boundary.x,
                o->o_y * (eff_boundary.height - eff_boundary.y) + eff_boundary.y,
                o->input_scale / 100,
                o->clip,
                o->warp,
                format,
                input,
                &eff_boundary,
                output,
                &boundary,
                result,
                level);
  return TRUE;
}


static void
gegl_op_class_init (GeglOpClass *klass)
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
    "name",               "gegl:mirrors",
    "title",              _("Kaleidoscopic Mirroring"),
    "position-dependent", "true",
    "reference-hash",     "34196267740ed972fd1e4d49f9560959",
    "categories",         "blur",
    "description",        _("Create a kaleidoscope like effect."),
    NULL);
}

#endif
