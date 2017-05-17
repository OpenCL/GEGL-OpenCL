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
#include <math.h>

#ifdef GEGL_PROPERTIES

property_int (subdivisions, _("Subdivisions"), 1)
  description(_("Number of subdivisions"))
  value_range (0, 15)

property_int (x_scale, _("X Scale"), 1)
  description(_("Horizontal pattern scale"))
  value_range (1, G_MAXINT)
  ui_range    (1, 128)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "x")

property_int (y_scale, _("Y Scale"), 1)
  description(_("Vertical pattern scale"))
  value_range (1, G_MAXINT)
  ui_range    (1, 128)
  ui_meta     ("unit", "pixel-distance")
  ui_meta     ("axis", "y")

enum_start (gegl_bayer_matrix_rotation)
  enum_value (GEGL_BAYER_MATRIX_ROTATION_0,   "0",   N_("0°"))
  enum_value (GEGL_BAYER_MATRIX_ROTATION_90,  "90",  N_("90°"))
  enum_value (GEGL_BAYER_MATRIX_ROTATION_180, "180", N_("180°"))
  enum_value (GEGL_BAYER_MATRIX_ROTATION_270, "270", N_("270°"))
enum_end (GeglBayerMatrixRotation)

property_enum (rotation, _("Rotation"),
               GeglBayerMatrixRotation, gegl_bayer_matrix_rotation,
               GEGL_BAYER_MATRIX_ROTATION_0)
  description (_("Pattern rotation angle"))

property_boolean (reflect, _("Reflect"), FALSE)
  description(_("Reflect the pattern horizontally"))

property_double (amplitude, _("Amplitude"), 0.0)
  description(_("Pattern amplitude (logarithmic scale)"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-2.0, 2.0)

property_double (offset, _("Offset"), 0.0)
  description(_("Value offset"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-1.0, 1.0)

property_double (exponent, _("Exponent"), 0.0)
  description(_("Value exponent (logarithmic scale)"))
  value_range (-G_MAXDOUBLE, G_MAXDOUBLE)
  ui_range    (-2.0, 2.0)

property_int (x_offset, _("X Offset"), 0)
  description (_("Offset for X axis"))
  value_range (G_MININT, G_MAXINT)
  ui_range    (-512, 512)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "x")

property_int (y_offset, _("Y Offset"), 0)
  description (_("Offset for Y axis"))
  value_range (G_MININT, G_MAXINT)
  ui_range    (-512, 512)
  ui_meta     ("unit", "pixel-coordinate")
  ui_meta     ("axis", "y")

#else

#define GEGL_OP_POINT_RENDER
#define GEGL_OP_NAME     bayer_matrix
#define GEGL_OP_C_SOURCE bayer-matrix.c

#include "gegl-op.h"
#include <math.h>

#define GEGL_BAYER_MATRIX_MAX_LUT_SUBDIVISIONS 8

static inline gfloat
odd_powf (gfloat base,
          gfloat exponent)
{
  if (base >= 0.0f)
    return  powf ( base, exponent);
  else
    return -powf (-base, exponent);
}

static inline gboolean
is_power_of_two (guint x)
{
  return (x & (x - 1)) == 0;
}

static inline gint
log2i (guint x)
{
  gint result = 0;
  gint shift  = 8 * sizeof (guint);

  while (shift >>= 1)
    {
      if (x >> shift)
        {
          result  += shift;
          x      >>= shift;
        }
    }

  return result;
}

static inline gint
div_floor (gint a,
           gint b)
{
  /* we assume b is positive */
  if (a < 0) a -= b - 1;
  return a / b;
}

static void
finalize (GObject *object)
{
  GeglOperation  *op = (void*) object;
  GeglProperties *o  = GEGL_PROPERTIES (op);

  if (o->user_data)
    {
      g_free (o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gfloat
value_at (GeglProperties *o,
          gint            x,
          gint            y)
{
  gint  i;
  guint value = 0;

  static const gint subdivision_value_luts[2 /* reflection */]
                                          [4 /* rotation   */]
                                          [2 /* row        */]
                                          [2 /* column     */] =
    {
      {
        {{0, 2},
         {3, 1}},

        {{2, 1},
         {0, 3}},

         {{1, 3},
          {2, 0}},

         {{3, 0},
          {1, 2}}
      },

      {
        {{2, 0},
         {1, 3}},

        {{1, 2},
         {3, 0}},

         {{3, 1},
          {0, 2}},

         {{0, 3},
          {2, 1}}
      }
    };
  const gint (* subdivision_values)[2];

  subdivision_values = subdivision_value_luts[o->reflect][o->rotation];

  for (i = 0; i < o->subdivisions; i++)
    {
      value <<= 2;
      value  |= subdivision_values[y & 1][x & 1];
      x     >>= 1;
      y     >>= 1;
    }

  return odd_powf (o->offset            +
                   exp2f (o->amplitude) *
                   (value + .5f) / (1u << (2 * o->subdivisions)),
                   exp2f (o->exponent));
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  if (o->subdivisions <= GEGL_BAYER_MATRIX_MAX_LUT_SUBDIVISIONS)
    {
      gint    size;
      gint    x, y;
      gfloat *lut;

      size = 1 << o->subdivisions;

      o->user_data = lut = g_renew (gfloat, o->user_data, size * size);

      for (y = 0; y < size; y++)
        {
          for (x = 0; x < size; x++)
            {
              *lut++ = value_at (o, x, y);
            }
        }
    }

  gegl_operation_set_format (operation, "output", babl_format ("Y' float"));
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  return gegl_rectangle_infinite_plane ();
}

static gboolean
process (GeglOperation       *operation,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o            = GEGL_PROPERTIES (operation);
  gint            i, j;
  gint            last_i, last_j;
  gint            x, y;
  gfloat         *result       = out_buf;
  const gfloat   *lut          = NULL;
  const gfloat   *lut_row      = NULL;
  gint            size;
  gint            coord_mask;
  gint            log2_x_scale = -1;
  gboolean        log2_y_scale = -1;

  if (o->subdivisions <= GEGL_BAYER_MATRIX_MAX_LUT_SUBDIVISIONS)
    lut = o->user_data;

  size       = 1 << o->subdivisions;
  coord_mask = size - 1;

  if (is_power_of_two (o->x_scale))
    log2_x_scale = log2i (o->x_scale);
  if (is_power_of_two (o->y_scale))
    log2_y_scale = log2i (o->y_scale);

  for (j = roi->y - o->y_offset, last_j = j + roi->height; j != last_j; j++)
    {
      if (log2_y_scale >= 0) y = j >> log2_y_scale;
      else                   y = div_floor (j, o->y_scale);

      y &= coord_mask;

      if (lut)
        lut_row = lut + size * y;

      for (i = roi->x - o->x_offset, last_i = i + roi->width; i != last_i; i++)
        {
          if (log2_x_scale >= 0) x = i >> log2_x_scale;
          else                   x = div_floor (i, o->x_scale);

          x &= coord_mask;

          if (lut_row)
            *result = lut_row[x];
          else
            *result = value_at (o, x, y);

          result++;
        }
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                  *object_class;
  GeglOperationClass            *operation_class;
  GeglOperationPointRenderClass *point_render_class;

  object_class       = G_OBJECT_CLASS (klass);
  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_render_class = GEGL_OPERATION_POINT_RENDER_CLASS (klass);

  object_class->finalize = finalize;

  point_render_class->process = process;

  operation_class->get_bounding_box = get_bounding_box;
  operation_class->prepare          = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",               "gegl:bayer-matrix",
    "title",              _("Bayer Matrix"),
    "categories",         "render",
    "position-dependent", "true",
    "license",            "GPL3+",
    "description",        _("Generate a Bayer matrix pattern"),
    NULL);
}

#endif
