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
 * Copyright 1997 Eric L. Hernes (erich@rrnet.com)
 * Copyright 2011 Robert Sasu (sasu.robert@gmail.com)
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (azimuth, _("Azimuth"), 0.0, 360.0, 30.0,
                   _("The value of azimuth"))
gegl_chant_double (elevation, _("Elevation"), 0.0, 180.0, 45.0,
                   _("The value of elevation"))
gegl_chant_int    (depth, _("Depth"), 1, 100, 20,
                   _("Pixel depth"))
gegl_chant_string (filter, _("Filter"), "emboss",
                   _("Optional parameter to override automatic selection "
                     "of emboss filter. Choices are emboss, blur-map"))

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE        "emboss.c"

#include "gegl-chant.h"
#include <math.h>
#include <stdio.h>

#define DEG_TO_RAD(d) (((d) * G_PI) / 180.0)

/*
 * ANSI C code from the article
 * "Fast Embossing Effects on Raster Image Data"
 * by John Schlag, jfs@kerner.com
 * in "Graphics Gems IV", Academic Press, 1994
 *
 * Emboss - shade 24-bit pixels using a single distant light source.
 * Normals are obtained by differentiating a monochrome 'bump' image.
 * The unary case ('texture' == NULL) uses the shading result as output.
 * The binary case multiples the optional 'texture' image by the shade.
 * Images are in row major order with interleaved color components
 * (rgbrgb...).  E.g., component c of pixel x,y of 'dst' is
 * dst[3*(y*width + x) + c].
 */

static void
emboss (gfloat              *src_buf,
        const GeglRectangle *src_rect,
        gfloat              *dst_buf,
        const GeglRectangle *dst_rect,
        gint                 x,
        gchar               *text,
        gint                 floats_per_pixel,
        gint                 alpha,
        gdouble              azimuth,
        gdouble              elevation,
        gint                 width45)
{
  gint y;
  gint offset, verify;
  gint bytes;

  gdouble Lx, Ly, Lz;
  gdouble Nz, Nz2, NzLz;

  Lx = cos (azimuth) * cos (elevation);
  Ly = sin (azimuth) * cos (elevation);
  Lz = sin (elevation);
  Nz = 1.0 / width45;
  Nz2  = Nz * Nz;
  NzLz = Nz * Lz;

  bytes = (alpha) ? floats_per_pixel - 1 : floats_per_pixel;

  verify = src_rect->width * src_rect->height * floats_per_pixel;
  offset = x * dst_rect->width * floats_per_pixel;

  for (y = 0; y < dst_rect->width; y++)
    {
      gint   i, j, b, count;
      gfloat Nx, Ny, NdotL;
      gfloat shade;
      gfloat M[3][3];
      gfloat a;

      for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
          M[i][j] = 0.0;

      for (b = 0; b < bytes; b++)
        for (i = 0; i < 3; i++)
          for (j = 0; j < 3; j++)
            {
              count = ((x+i-1)*src_rect->width + (y+j-1))*floats_per_pixel + bytes;

              /*verify each time that we are in the source image*/
              if (alpha && count >= 0 && count < verify)
                a = src_buf[count];
              else
                a = 1.0;

              /*calculate recalculate the sorrounding pixels by multiplication*/
              /*after we have that we can calculate new value of the pixel*/
              if ((count - bytes + b) >= 0 && (count - bytes + b) < verify)
                M[i][j] += a * src_buf[count - bytes + b];
            }

      Nx = M[0][0] + M[1][0] + M[2][0] - M[0][2] - M[1][2] - M[2][2];
      Ny = M[2][0] + M[2][1] + M[2][2] - M[0][0] - M[0][1] - M[0][2];

      /*calculating the shading result (same as in gimp)*/
      if ( Nx == 0 && Ny == 0 )
        shade = Lz;
      else if ( (NdotL = Nx * Lx + Ny * Ly + NzLz) < 0 )
        shade = 0;
      else
        shade = NdotL / sqrt(Nx*Nx + Ny*Ny + Nz2);

      count = (x*src_rect->width + y)*floats_per_pixel;

      /*setting the value of the destination buffer*/
      if (bytes == 1)
        dst_buf[offset++] = shade;
      else
        {
          /*recalculating every byte of a pixel*/
          /*by multiplying with the shading result*/

          for (b = 0; b < bytes; b++)
            if ((count + b) >= 0 && (count + b) < verify)
              dst_buf[offset++] = (src_buf[count+b] * shade) ;
            else
              dst_buf[offset++] = 1.0;

          /*preserving alpha*/
          if (alpha && (count + bytes) >= 0 && (count + bytes) < verify)
            dst_buf[offset++] = src_buf[count + bytes];
          else
            dst_buf[offset++] = 1.0 ;
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left = op_area->right = op_area->top = op_area->bottom = 3;

  if (o->filter && !strcmp(o->filter, "blur-map"))
    gegl_operation_set_format (operation, "output",
                               babl_format ("RGBA float"));
  else
    gegl_operation_set_format (operation, "output",
                               babl_format ("Y float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO              *o       = GEGL_CHANT_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglRectangle  rect;
  gfloat        *src_buf;
  gfloat        *dst_buf;

  gchar         *type;
  gint           alpha;
  gint           x;
  gint           floats_per_pixel;

  /*blur-map or emboss*/
  if (o->filter && !strcmp (o->filter, "blur-map"))
    {
      type = "RGBA float";
      floats_per_pixel = 4;
      alpha = 1;
    }
  else
    {
      type = "Y float";
      floats_per_pixel = 1;
      alpha = 0;
    }

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_new0 (gfloat, rect.width * rect.height * floats_per_pixel);
  dst_buf = g_new0 (gfloat, rect.width * rect.height * floats_per_pixel);

  gegl_buffer_get (input, &rect, 1.0, babl_format (type), src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*do for every row*/
  for (x = 0; x < rect.height; x++)
    emboss (src_buf, &rect, dst_buf, &rect, x, type, floats_per_pixel, alpha,
            DEG_TO_RAD (o->azimuth), DEG_TO_RAD (o->elevation), o->depth);

  gegl_buffer_set (output, &rect, 0, babl_format (type),
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
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

  gegl_operation_class_set_keys (operation_class,
    "categories" , "distort",
    "name"       , "gegl:emboss",
    "description", _("Simulates an image created by embossing"),
    NULL);
}

#endif
