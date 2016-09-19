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

#ifdef GEGL_PROPERTIES

enum_start (gegl_emboss_type)
  enum_value (GEGL_EMBOSS_TYPE_EMBOSS,  "emboss",  N_("Emboss"))
  enum_value (GEGL_EMBOSS_TYPE_BUMPMAP, "bumpmap", N_("Bumpmap (preserve original colors)"))
enum_end (GeglEmbossType)

property_enum (type, _("Emboss Type"),
               GeglEmbossType, gegl_emboss_type, GEGL_EMBOSS_TYPE_EMBOSS)
    description(_("Rendering type"))

property_double (azimuth, _("Azimuth"), 30.0)
    description (_("Light angle (degrees)"))
    value_range (0, 360)
    ui_meta ("unit", "degree")

property_double (elevation, _("Elevation"), 45.0)
    description (_("Elevation angle (degrees)"))
    value_range (0, 180)
    ui_meta ("unit", "degree")

property_int (depth, _("Depth"), 20)
    description (_("Filter width"))
    value_range (1, 100)

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     emboss
#define GEGL_OP_C_SOURCE emboss.c

#include "gegl-op.h"
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
        GeglEmbossType       type,
        gint                 y,
        gint                 floats_per_pixel,
        gdouble              azimuth,
        gdouble              elevation,
        gint                 width45)
{
  gint x;
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

  bytes = floats_per_pixel - 1;

  verify = src_rect->width * src_rect->height * floats_per_pixel;
  offset = y * dst_rect->width * floats_per_pixel;

  for (x = 0; x < dst_rect->width; x++)
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
        {
          for (i = 0; i < 3; i++)
            {
              for (j = 0; j < 3; j++)
                {
                  count = ((y + i - 1) * src_rect->width + (x + j - 1)) * floats_per_pixel + bytes;

                  /* verify each time that we are in the source image */
                  if (count >= 0 && count < verify)
                    a = src_buf[count];
                  else
                    a = 1.0;

                  /* calculate recalculate the sorrounding pixels by
                   * multiplication after we have that we can
                   * calculate new value of the pixel
                   */
                  if ((count - bytes + b) >= 0 && (count - bytes + b) < verify)
                    M[i][j] += a * src_buf[count - bytes + b];
                }
            }
        }

      Nx = M[0][0] + M[1][0] + M[2][0] - M[0][2] - M[1][2] - M[2][2];
      Ny = M[2][0] + M[2][1] + M[2][2] - M[0][0] - M[0][1] - M[0][2];

      /* calculating the shading result (same as in gimp) */
      if (Nx == 0 && Ny == 0)
        shade = Lz;
      else if ((NdotL = Nx * Lx + Ny * Ly + NzLz) < 0)
        shade = 0;
      else
        shade = NdotL / sqrt (Nx * Nx + Ny * Ny + Nz2);

      count = (y * src_rect->width + x) * floats_per_pixel;

      /* setting the value of the destination buffer */
      if (type == GEGL_EMBOSS_TYPE_EMBOSS)
        {
          dst_buf[offset++] = shade;
        }
      else
        {
          /* recalculating every byte of a pixel by multiplying with
           * the shading result
           */

          for (b = 0; b < bytes; b++)
            {
              if ((count + b) >= 0 && (count + b) < verify)
                dst_buf[offset++] = (src_buf[count + b] * shade);
              else
                dst_buf[offset++] = 1.0;
            }
        }

      /* preserving alpha */
      if ((count + bytes) >= 0 && (count + bytes) < verify)
        dst_buf[offset++] = src_buf[count + bytes];
      else
        dst_buf[offset++] = 1.0;
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  op_area->left = op_area->right = op_area->top = op_area->bottom = 3;

  if (o->type == GEGL_EMBOSS_TYPE_BUMPMAP)
    gegl_operation_set_format (operation, "output",
                               babl_format ("RGBA float"));
  else
    gegl_operation_set_format (operation, "output",
                               babl_format ("YA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties          *o       = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *op_area = GEGL_OPERATION_AREA_FILTER (operation);

  GeglRectangle  rect;
  gfloat        *src_buf;
  gfloat        *dst_buf;
  const Babl    *format;
  gint           y;
  gint           floats_per_pixel;

  /*blur-map or emboss*/
  if (o->type == GEGL_EMBOSS_TYPE_BUMPMAP)
    {
      format = babl_format ("RGBA float");
      floats_per_pixel = 4;
    }
  else
    {
      format = babl_format ("YA float");
      floats_per_pixel = 2;
    }

  rect.x      = result->x - op_area->left;
  rect.width  = result->width + op_area->left + op_area->right;
  rect.y      = result->y - op_area->top;
  rect.height = result->height + op_area->top + op_area->bottom;

  src_buf = g_new0 (gfloat, rect.width * rect.height * floats_per_pixel);
  dst_buf = g_new0 (gfloat, rect.width * rect.height * floats_per_pixel);

  gegl_buffer_get (input, &rect, 1.0, format, src_buf,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  /*do for every row*/
  for (y = 0; y < rect.height; y++)
    emboss (src_buf, &rect, dst_buf, &rect, o->type, y, floats_per_pixel,
            DEG_TO_RAD (o->azimuth), DEG_TO_RAD (o->elevation), o->depth);

  gegl_buffer_set (output, &rect, 0, format,
                   dst_buf, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buf);
  g_free (dst_buf);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process    = process;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:emboss",
    "title",       _("Emboss"),
    "categories",  "light",
    "license",     "GPL3+",
    "description", _("Simulates an image created by embossing"),
    NULL);
}

#endif
