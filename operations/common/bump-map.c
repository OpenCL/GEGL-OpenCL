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
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_bump_map_type)
  enum_value (GEGL_BUMP_MAP_TYPE_LINEAR,     "linear",     N_("Linear"))
  enum_value (GEGL_BUMP_MAP_TYPE_SPHERICAL,  "spherical",  N_("Spherical"))
  enum_value (GEGL_BUMP_MAP_TYPE_SINUSOIDAL, "sinusodial", N_("Sinusoidal"))
enum_end (GeglBumpMapType)

property_enum (type, _("Type"), GeglBumpMapType, gegl_bump_map_type,
               GEGL_BUMP_MAP_TYPE_LINEAR)
  description (_("Type of map"))

property_boolean (compensate, _("Compensate"), TRUE)
  description (_("Compensate for darkening"))

property_boolean (invert, _("Invert"), FALSE)
  description (_("Invert bumpmap"))

property_boolean (tiled, _("Tiled"), FALSE)
  description (_("Tiled bumpmap"))

property_double  (azimuth, _("Azimuth"), 135.0)
  value_range (0.0, 360.0)
  ui_meta     ("unit", "degree")

property_double  (elevation, _("Elevation"), 45.0)
  value_range (0.5, 90.0)

property_int  (depth, _("Depth"), 3)
  value_range (1, 65)

property_int  (offset_x, _("Offset X"), 0)
  value_range (-20000, 20000)
  ui_range (-1000, 1000)
  ui_meta  ("axis", "x")
  ui_meta  ("unit", "pixel-coordinate")

property_int  (offset_y, _("Offset Y"), 0)
  value_range (-20000, 20000)
  ui_range (-1000, 1000)
  ui_meta  ("axis", "y")
  ui_meta  ("unit", "pixel-coordinate")

property_double  (waterlevel, _("Waterlevel"), 0.0)
  description(_("Level that full transparency should represent"))
  value_range (0.0, 1.0)

property_double  (ambient, _("Ambient lighting factor"), 0.0)
  value_range (0.0, 1.0)

#else

#define GEGL_OP_COMPOSER
#define GEGL_OP_C_SOURCE bump-map.c

#include "gegl-op.h"
#include <math.h>

#define LUT_SIZE 2048

typedef struct
{
  gdouble lx, ly;           /* X and Y components of light vector */
  gdouble nz2, nzlz;        /* nz^2, nz*lz */
  gdouble background;       /* Shade for vertical normals */
  gdouble compensation;     /* Background compensation */
  gdouble lut[LUT_SIZE];    /* Look-up table for modes - should be made interpolated*/

  gboolean in_has_alpha;    /* babl formats info */
  gboolean bm_has_alpha;
  gint     in_components;
  gint     bm_components;
} bumpmap_params_t;

static void
bumpmap_init_params (GeglProperties *o,
                     const Babl     *in_format,
                     const Babl     *bm_format)
{
  bumpmap_params_t *params = (bumpmap_params_t *) o->user_data;
  gdouble lz, nz;
  gint    i;

  gint scale = LUT_SIZE - 1;

  /* Convert to radians */
  const gdouble azimuth   = G_PI * o->azimuth / 180.0;
  const gdouble elevation = G_PI * o->elevation / 180.0;

  /* Calculate the light vector */
  params->lx = cos (azimuth) * cos (elevation);
  params->ly = sin (azimuth) * cos (elevation);
  lz         = sin (elevation);

  /* Calculate constant Z component of surface normal */
  nz           = 6.0 / o->depth;
  params->nz2  = nz * nz;
  params->nzlz = nz * lz;

  /* Optimize for vertical normals */
  params->background = lz;

  /* Calculate darkness compensation factor */
  params->compensation = sin (elevation);

  /* Create look-up table for map type */
  for (i = 0; i < LUT_SIZE; i++)
    {
      gdouble n;

      switch (o->type)
        {
        case GEGL_BUMP_MAP_TYPE_SPHERICAL:
          n = (gdouble) i / scale - 1.0;
          params->lut[i] = sqrt (1.0 - n * n) + 0.5;
          break;

        case GEGL_BUMP_MAP_TYPE_SINUSOIDAL:
          n = (gdouble) i / scale;
          params->lut[i] = (sin ((-G_PI / 2.0) + G_PI * n) + 1.0) / 2.0 + 0.5;
          break;

        case GEGL_BUMP_MAP_TYPE_LINEAR:
        default:
          params->lut[i] = (gdouble) i / scale;
        }

      if (o->invert)
        params->lut[i] = 1.0 - params->lut[i];
    }

  /* babl format stuff */
  params->in_has_alpha  = babl_format_has_alpha (in_format);
  params->bm_has_alpha  = babl_format_has_alpha (bm_format);
  params->in_components = babl_format_get_n_components (in_format);
  params->bm_components = babl_format_get_n_components (bm_format);
}

static void
bumpmap_row (gfloat         *row,
             gint            width,
             const gfloat   *bm_row1,
             const gfloat   *bm_row2,
             const gfloat   *bm_row3,
             gint            bm_width,
             gboolean        row_in_bumpmap,
             gint            roix,
             GeglProperties *o)
{
  bumpmap_params_t *params = (bumpmap_params_t *) o->user_data;
  gint xofs1, xofs2, xofs3;
  gint x, b;
  gfloat *buf = row;

  for (x = 0; x < width; x++)
    {
      gdouble shade;
      gdouble nx, ny;

      /* Calculate surface normal from bump map */

      if (o->tiled || (row_in_bumpmap &&
            roix + x >= - o->offset_x && roix + x < - o->offset_x + bm_width))
        {
          xofs2 = (x + 1) * params->bm_components;
          xofs1 = CLAMP (x * params->bm_components, 0, (width + 1) * params->bm_components);
          xofs3 = CLAMP ((x + 2) * params->bm_components, 0, (width + 1) * params->bm_components);

          nx = (bm_row1[xofs1] + bm_row2[xofs1] + bm_row3[xofs1] -
                bm_row1[xofs3] - bm_row2[xofs3] - bm_row3[xofs3]);

          ny = (bm_row3[xofs1] + bm_row3[xofs2] + bm_row3[xofs3] -
                bm_row1[xofs1] - bm_row1[xofs2] - bm_row1[xofs3]);
        }
      else
        {
          nx = ny = 0.0;
        }

      if ((nx == 0.0) && (ny == 0.0))
        {
          shade = params->background;
        }
      else
        {
          gdouble ndotl = nx * params->lx + ny * params->ly + params->nzlz;

          if (ndotl < 0.0)
            {
              shade = params->compensation * o->ambient;
            }
          else
            {
              shade = ndotl / sqrt (nx * nx + ny * ny + params->nz2);
              shade += MAX(0, (params->compensation - shade)) * o->ambient;
            }
        }

      /* Paint */
      if (o->compensate)
        {
          for (b = 0 ; b < 3; b++)
            buf[b] = (buf[b] * shade) / params->compensation;
        }
      else
        {
          for (b = 0 ; b < 3; b++)
            buf[b] = buf[b] * shade;
        }

      if (params->in_has_alpha)
        buf[3] = buf[3];

      buf += params->in_components;
    }
}

static void
bumpmap_convert (gfloat         *buffer,
                 glong           n_pixels,
                 GeglProperties *o)
{
  bumpmap_params_t *params   = (bumpmap_params_t *) o->user_data;
  gint    idx;
  gint    scale = LUT_SIZE - 1;
  gfloat  *p = buffer;

  while (n_pixels--)
    {
      gfloat value = CLAMP (p[0], 0.0, 1.0);

      if (params->bm_has_alpha)
        {
          gfloat alpha = CLAMP (p[1], 0.0, 1.0);
          idx = (gint) ((o->waterlevel + (value - o->waterlevel) * alpha) * scale);
        }
      else
        idx = (gint) (value * scale);

      p[0] = params->lut[idx];
      p += params->bm_components;
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const Babl *in_format = gegl_operation_get_source_format (operation, "input");
  const Babl *bm_format = gegl_operation_get_source_format (operation, "aux");

  if (!o->user_data)
    o->user_data = g_slice_new0 (bumpmap_params_t);

  if (in_format)
    {
      if (babl_format_has_alpha (in_format))
        in_format = babl_format ("R'G'B'A float");
      else
        in_format = babl_format ("R'G'B' float");
    }
  else
    in_format = babl_format ("R'G'B' float");

  if (bm_format)
    {
      if (babl_format_has_alpha (bm_format))
        bm_format = babl_format ("Y'A float");
      else
        bm_format = babl_format ("Y' float");
    }
  else
    bm_format = babl_format ("Y' float");

  bumpmap_init_params (o, in_format, bm_format);

  gegl_operation_set_format (operation, "input",  in_format);
  gegl_operation_set_format (operation, "aux",    bm_format);
  gegl_operation_set_format (operation, "output", in_format);
}

static void
finalize (GObject *object)
{
  GeglOperation *op = (void*) object;
  GeglProperties *o = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_slice_free (bumpmap_params_t, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties   *o        = GEGL_PROPERTIES (operation);
  bumpmap_params_t *params   = (bumpmap_params_t *) o->user_data;

  const Babl *in_format = gegl_operation_get_format (operation, "input");
  const Babl *bm_format = gegl_operation_get_format (operation, "aux");

  GeglAbyssPolicy repeat_mode = o->tiled ? GEGL_ABYSS_LOOP : GEGL_ABYSS_CLAMP;

  GeglRectangle bm_rect, bm_extent;

  gfloat  *src_row, *src_buffer, *bm_buffer;
  gfloat  *bm_row1, *bm_row2, *bm_row3;
  gint     y;
  gboolean row_in_bumpmap;

  src_buffer = g_new (gfloat, roi->width * roi->height * params->in_components);

  gegl_buffer_get (input, roi, 1.0, in_format, src_buffer,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  if (aux)
    {
      bm_extent = *gegl_buffer_get_extent (aux);

      bm_rect = *roi;
      bm_rect.x += o->offset_x - 1;
      bm_rect.y += o->offset_y - 1;
      bm_rect.width += 2;
      bm_rect.height += 2;

      bm_buffer = g_new (gfloat, bm_rect.width * bm_rect.height * params->bm_components);

      gegl_buffer_get (aux, &bm_rect, 1.0, bm_format,
                       bm_buffer, GEGL_AUTO_ROWSTRIDE, repeat_mode);

      bumpmap_convert (bm_buffer, bm_rect.width * bm_rect.height, o);

      bm_row1 = bm_buffer;
      bm_row2 = bm_buffer + (bm_rect.width * params->bm_components);
      bm_row3 = bm_buffer + (2 * bm_rect.width * params->bm_components);

      for (y = roi->y; y < roi->y + roi->height; y++)
        {
          row_in_bumpmap = (y >= - o->offset_y && y < - o->offset_y + bm_extent.height);

          src_row = src_buffer + ((y - roi->y) * roi->width * params->in_components);

          bumpmap_row (src_row, roi->width,
                       bm_row1, bm_row2, bm_row3, bm_extent.width,
                       row_in_bumpmap, roi->x, o);

          bm_row1 = bm_row2;
          bm_row2 = bm_row3;
          bm_row3 = bm_row2 + (bm_rect.width * params->bm_components);
       }

       g_free (bm_buffer);
    }

  gegl_buffer_set (output, roi, level, in_format,
                   src_buffer, GEGL_AUTO_ROWSTRIDE);

  g_free (src_buffer);

  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result  = { 0, 0, 0, 0 };
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");

  if (! in_rect)
    return result;

  return *in_rect;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  if (!strcmp (input_pad, "aux"))
    {
      GeglRectangle bm_rect = *gegl_operation_source_get_bounding_box (operation,
                                                                       "aux");
      if (gegl_rectangle_is_empty (&bm_rect))
        return *roi;

      return bm_rect;
    }

  return *roi;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass               *object_class;
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  object_class    = G_OBJECT_CLASS (klass);
  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  object_class->finalize                   = finalize;

  operation_class->prepare                 = prepare;
  operation_class->get_bounding_box        = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->opencl_support          = FALSE;

  composer_class->process                  = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:bump-map",
    "categories",  "light",
    "title",       _("Bump Map"),
    "license",     "GPL3+",
    "description", _("This plug-in uses the algorithm described by John "
                     "Schlag, \"Fast Embossing Effects on Raster Image "
                     "Data\" in Graphics GEMS IV (ISBN 0-12-336155-9). "
                     "It takes a buffer to be applied as a bump "
                     "map to another buffer and produces a nice embossing "
                     "effect."),
    NULL);
}

#endif
