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
 * This plug-in uses the algorithm described by John Schlag, "Fast
 * Embossing Effects on Raster Image Data" in Graphics Gems IV (ISBN
 * 0-12-336155-9).  It takes a grayscale image to be applied as a
 * bump-map to another image, producing a nice embossing effect.
 *
 * Ported to gegl by Dov Grobgeld <dov.grobgeld@gmail.com>
 *
 * ToDo:
 *
 *  - Make lut table size configurable?
 *  - Support and y-offset tiling of the bump map.
 *  - Make an opencl version.
 *  - Don't "upgrade" Y,YA and RGB images to RGBA as it is a waste of time.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_register_enum (gegl_bump_map_type)
  enum_value (GEGL_BUMP_MAP_TYPE_LINEAR,     "Linear")
  enum_value (GEGL_BUMP_MAP_TYPE_SPHERICAL,  "Cosinus")
  enum_value (GEGL_BUMP_MAP_TYPE_SINUSOIDAL, "Sinusoidal")
gegl_chant_register_enum_end (GeglBumpMapType)

gegl_chant_double  (azimuth, _("Azimuth"),
                    0.0, 360.0, 135.0,
                    _("Azimuth"))

gegl_chant_double  (elevation, _("Elevation"),
                    0.0, 360.0, 45.0,
                    _("Elevation"))

gegl_chant_double  (depth, _("Depth"),
                    0.0005, 100, 0.005,
                    _("Depth"))

gegl_chant_int     (xofs, _("xofs"),
                    -1000, 1000,0,
                    _("X offset"))

gegl_chant_int     (yofs, _("yofs"),
                    -1000, 1000,0,
                    _("Y offset"))

gegl_chant_double  (waterlevel, _("waterlevel"),
                    0.0,1.0,0.0,
                    _("Level that full transparency should represent"))

gegl_chant_double  (ambient, _("ambient"),
                    0.0, 1.0, 0.0,
                    _("Ambient lighting factor"))

gegl_chant_boolean (compensate, _("Compensate"),
                    TRUE,
                    _("Compensate for darkening"))

gegl_chant_boolean (invert, _("Invert"),
                    FALSE,
                    _("Invert bumpmap"))

gegl_chant_enum    (type, _("Type"),
                    GeglBumpMapType, gegl_bump_map_type,
                    GEGL_BUMP_MAP_TYPE_LINEAR,
                    _("Type of map"))

gegl_chant_boolean (tiled, _("Tiled"),
                    FALSE,
                    _("Tiled"))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE "bump-map.c"

#include "gegl-chant.h"
#include <math.h>

/***** Macros *****/

// Should the LUT table be a configurable property? Should it
// be interpolated instead of by a plain lookup?
#define LUT_TABLE_SIZE 8192

#define MOD(x, y) \
  ((x) < 0 ? ((y) - 1 - ((y) - 1 - (x)) % (y)) : (x) % (y))

typedef struct
{
  gdouble lx, ly;       /* X and Y components of light vector */
  gdouble nz2, nzlz;    /* nz^2, nz*lz */
  gdouble background;   /* Shade for vertical normals */
  gdouble compensation; /* Background compensation */
  gfloat lut[LUT_TABLE_SIZE];    /* Look-up table for modes - should be made interpolated*/
} bumpmap_params_t;

static void
bumpmap_setup_calc (GeglChantO     *o,
                    bumpmap_params_t *params)
{
  /* Convert to radians */
  const gdouble azimuth   = G_PI * o->azimuth / 180.0;
  const gdouble elevation = G_PI * o->elevation / 180.0;

  gdouble lz, nz;
  gint i;

  /* Calculate the light vector */
  params->lx = cos (azimuth) * cos (elevation);
  params->ly = sin (azimuth) * cos (elevation);
  lz         = sin (elevation);

  /* Calculate constant Z component of surface normal */
  /*              (depth may be 0 if non-interactive) */
  nz           = 6.0 / MAX(o->depth,0.001);
  params->nz2  = nz * nz;
  params->nzlz = nz * lz;

  /* Optimize for vertical normals */
  params->background = lz;

  /* Calculate darkness compensation factor */
  params->compensation = sin(elevation);

  /* Create look-up table for map type */
  for (i = 0; i < LUT_TABLE_SIZE; i++)
    {
      gdouble n;

      switch (o->type)
        {
        case GEGL_BUMP_MAP_TYPE_SPHERICAL:
          n = 1.0 - 1.0*i / LUT_TABLE_SIZE;
          params->lut[i] = sqrt(1.0 - n * n) + 0.5;
          break;

        case GEGL_BUMP_MAP_TYPE_SINUSOIDAL:
          n = 1.0 * i / (LUT_TABLE_SIZE-1);
          params->lut[i] = (sin((-G_PI / 2.0) + G_PI * n) + 1.0) / 2.0 + 0.5;
          break;

        case GEGL_BUMP_MAP_TYPE_LINEAR:
        default:
          params->lut[i] = 1.0*i/(LUT_TABLE_SIZE-1);
        }

      if (o->invert)
        params->lut[i] = 1.0 - params->lut[i];
    }
}

static void
bumpmap_row (const gfloat     *src,
             gfloat           *dest,
             gint              width,
             gint              nchannels,
             gint              has_alpha,
             const gfloat     *bm_row1,
             const gfloat     *bm_row2,
             const gfloat     *bm_row3,
             gint              bm_width,
             gint              bm_xofs,
             gboolean          row_in_bumpmap,
             GeglChantO       *o,
             bumpmap_params_t *params)
{
  gint xofs1, xofs2, xofs3;
  gint x, k;
  gdouble result;

  xofs2 = MOD (bm_xofs,bm_width);

  for (x = 0; x < width; x++)
    {
      double shade;
      double nx, ny;

      /* Calculate surface normal from bump map */

      if (o->tiled || (row_in_bumpmap &&
                       x >= - bm_xofs && x < - bm_xofs + bm_width))
        {
          if (o->tiled)
            {
              xofs1 = MOD (xofs2 - 1, bm_width);
              xofs3 = MOD (xofs2 + 1, bm_width);
            }
          else
            {
              xofs1 = CLAMP (xofs2 - 1, 0, bm_width - 1);
              xofs3 = CLAMP (xofs2 + 1, 0, bm_width - 1);
            }

          nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
                bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);
          ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
                bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);
        }
      else
        {
          nx = ny = 0;
        }

      if ((nx == 0) && (ny == 0))
        {
          shade = params->background;
        }
      else
        {
          double ndotl = nx * params->lx + ny * params->ly + params->nzlz;

          if (ndotl < 0)
            {
              shade = params->compensation * o->ambient;
            }
          else
            {
              double pre_shade = ndotl / sqrt (nx * nx + ny * ny + params->nz2);

              shade = pre_shade + MAX(0, (params->compensation - pre_shade)) *
                o->ambient;
            }
        }

      /* Paint */
      if (o->compensate)
        {
          for (k = nchannels-has_alpha; k; k--)
            {
              result  = (*src++ * shade) / params->compensation;
              *dest++ = result;
            }
        }
      else
        {
          for (k = nchannels-has_alpha; k; k--)
            *dest++ = *src++ * shade;
        }

      if (has_alpha)
        *dest++ = *src++;

      /* Next pixel */
      if (++xofs2 == bm_width)
        xofs2 = 0;
    }
}

// Map the FIFO bumpmap row according to the lookup table.
static void
bumpmap_convert_row (gfloat       *row,
                     gint          width,
                     const gfloat  *lut,
                     double        waterlevel)
{
  gfloat *p = row;

  for (; width; width--)
    {
      int lut_idx = CLAMP((int)(waterlevel * (LUT_TABLE_SIZE-1)
                                + *p *(LUT_TABLE_SIZE-1)*(1.0-waterlevel)),0,(LUT_TABLE_SIZE-1));
      *p++ = lut[lut_idx];
    }
}

static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "aux",    babl_format ("Y float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *rect,
         gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat  *src_buf, *dst_buf, *bm_row1, *bm_row2, *bm_row3, *src_row, *dest_row, *bm_tmprow;
  const Babl *format = gegl_operation_get_format (operation, "output");
  gint channels = babl_format_get_n_components (format);
  bumpmap_params_t params;
  gint yofs1, yofs2, yofs3;
  gint row_stride;
  gint row, y;
  gint bm_width, bm_height;
  gint slice_thickness = 32;
  gboolean first_time = TRUE;

  // This should be made more sophisticated
  int has_alpha = (channels == 4) || (channels == 2);

  if (! aux)
    return FALSE;

  bm_width = gegl_buffer_get_width (aux);
  bm_height = gegl_buffer_get_height (aux);

  src_buf    = g_new0 (gfloat, rect->width * slice_thickness * channels);
  dst_buf    = g_new0 (gfloat, rect->width * slice_thickness * channels);

  bumpmap_setup_calc (o, &params);

  /* Initialize offsets */
  if (o->tiled)
    {
      yofs2 = MOD (o->yofs, bm_height);
      yofs1 = MOD (yofs2 - 1, bm_height);
      yofs3 = MOD (yofs2 + 1, bm_height);
    }
  else
    {
      yofs2 = o->yofs;
      yofs1 = yofs2-1;
      yofs3 = yofs2+1;
    }

  /* Initialize three line fifo buffers */
  bm_row1 = g_new (gfloat, bm_width);
  bm_row2 = g_new (gfloat, bm_width);
  bm_row3 = g_new (gfloat, bm_width);

  // The source and destination row stride in floats
  row_stride = rect->width*channels;

  // Process the input buffer one slice at a time, but the bumpmap one row at a time.
  for (row=rect->y; row < rect->y+rect->height; row+= slice_thickness)
    {
      GeglRectangle rect_slice, bm_rect;
      rect_slice.x = rect->x;
      rect_slice.width = rect->width;
      rect_slice.y = rect->y+row;
      rect_slice.height = MIN (slice_thickness, rect->height-row);
      gegl_buffer_get (input, &rect_slice, 1.0,
                       babl_format ("RGBA float"), src_buf,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      // Get the bumpmap one row at a time. The following values are constant
      // in the bumpmap buffer access.
      bm_rect.x = 0;
      bm_rect.width = bm_width;
      bm_rect.height = 1;

      for (y = 0; y < rect_slice.height; y++)
        {
          gboolean row_in_bumpmap = (yofs2 > 0 && yofs2 < bm_height);

          // Fill in the three rows FIFO the first time we are inside the bumpmap.
          if (row_in_bumpmap && first_time)
            {
              first_time = FALSE;

              bm_rect.y = yofs1;
              gegl_buffer_get (aux, &bm_rect, 1.0,
                               babl_format ("Y float"), bm_row1,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
              bm_rect.y = yofs2;
              gegl_buffer_get (aux, &bm_rect, 1.0,
                               babl_format ("Y float"), bm_row2,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
              bm_rect.y = yofs3;
              gegl_buffer_get (aux, &bm_rect, 1.0,
                               babl_format ("Y float"), bm_row3,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

              bumpmap_convert_row (bm_row1, bm_width, params.lut, o->waterlevel);
              bumpmap_convert_row (bm_row2, bm_width, params.lut, o->waterlevel);
              bumpmap_convert_row (bm_row3, bm_width, params.lut, o->waterlevel);
            }

          src_row = src_buf + y * row_stride;
          dest_row = dst_buf + y * row_stride;

          bumpmap_row (src_row, dest_row,
                       rect->width,
                       channels,
                       has_alpha,
                       bm_row1, bm_row2, bm_row3,
                       bm_width,
                       o->xofs,
                       row_in_bumpmap,
                       o,
                       &params);

          if (!first_time)
            {
              /* Next line */
              bm_tmprow = bm_row1;
              bm_row1   = bm_row2;
              bm_row2   = bm_row3;
              bm_row3   = bm_tmprow;
            }

          if (++yofs2 == bm_height && o->tiled)
            yofs2 = 0;

          if (o->tiled)
            yofs3 = MOD (yofs2 + 1, bm_height);
          else
            yofs3 = CLAMP (yofs2 + 1, 0, bm_height);

          if (!first_time)
            {
              bm_rect.y = yofs3;
              gegl_buffer_get (aux, &bm_rect, 1.0, babl_format ("Y float"), bm_row3,
                               GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
              bumpmap_convert_row (bm_row3, bm_width, params.lut, o->waterlevel);
            }
        }

      gegl_buffer_set (output, &rect_slice, 0,
                       babl_format ("RGBA float"), dst_buf,
                       GEGL_AUTO_ROWSTRIDE);
    }

  g_free (bm_row1);
  g_free (bm_row2);
  g_free (bm_row3);

  g_free (dst_buf);
  g_free (src_buf);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle  result  = { 0, 0, 0, 0 };
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (self, "input");

  if (! in_rect)
    return result;

  return *in_rect;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  composer_class->process           = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:bump-map",
    "categories",  "map",
    "description", _("This plug-in uses the algorithm described by John "
                     "Schlag, \"Fast Embossing Effects on Raster Image "
                     "Data\" in Graphics GEMS IV (ISBN 0-12-336155-9). "
                     "It takes a drawable to be applied as a bump "
                     "map to another image and produces a nice embossing "
                     "effect."),
    NULL);
}

#endif
