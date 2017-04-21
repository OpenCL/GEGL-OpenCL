/* This file is an image processing operation for GEGL
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
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (s_amount, _("shadows amount"), 0.2)
    value_range (0.0, 1.0)

property_double (s_tonalwidth, _("shadows tonal width"), 0.1)
    value_range (0.001, 1.0)

property_double (h_amount, _("highlights amount"), 0.2)
    value_range (0.0, 1.0)

property_double (h_tonalwidth, _("highlights tonal width"), 0.1)
    value_range (0.001, 1.0)

#else

#define GEGL_OP_POINT_COMPOSER3
#define GEGL_OP_NAME     shadows_highlights_correction
#define GEGL_OP_C_SOURCE shadows-highlights-correction.c

#include "gegl-op.h"
#include <math.h>

#define LUT_SIZE 2048

typedef struct
{
  gdouble shadows[LUT_SIZE];
  gdouble highlights[LUT_SIZE];
} Luts;

static void
initialize_luts (GeglProperties *o)
{
  gint     i, scale = LUT_SIZE - 1;
  Luts    *luts = (Luts *) o->user_data;
  gdouble  s_scaled = o->s_tonalwidth * scale;
  gdouble  h_scaled = (1.0 - o->h_tonalwidth) * scale;

  for (i = 0; i < LUT_SIZE; i++)
    {
      /* shadows */
      if (i < s_scaled)
        {
          luts->shadows[i] = 1.0 - pow((i / s_scaled), 2.0);
        }
      else
        {
          luts->shadows[i] = 0.0;
        }

      /* highlights */
      if (i > h_scaled)
        {
          luts->highlights[i] = 1.0 - pow((scale - i) / (scale - h_scaled), 2.0);
        }
      else
        {
          luts->highlights[i] = 0.0;
        }
    }
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  const Babl *rgba_f = babl_format ("R'G'B'A float");
  const Babl *y_f  = babl_format ("Y' float");

  if (o->user_data == NULL)
    o->user_data = g_slice_new0 (Luts);

  initialize_luts (o);

  gegl_operation_set_format (operation, "input",  rgba_f);
  gegl_operation_set_format (operation, "aux",    y_f);
  gegl_operation_set_format (operation, "aux2",   y_f);
  gegl_operation_set_format (operation, "output", rgba_f);
}

static void
finalize (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->user_data)
    {
      g_slice_free (Luts, o->user_data);
      o->user_data = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *aux_buf,
         void                *aux2_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  Luts *luts = (Luts *) o->user_data;

  gfloat *src  = in_buf;
  gfloat *dst  = out_buf;
  gfloat *aux  = aux_buf;
  gfloat *aux2 = aux2_buf;

  if (!aux || !aux2)
    {
      memcpy (out_buf, in_buf, sizeof (gfloat) * 4 * n_pixels);
      return TRUE;
    }

  while (n_pixels--)
    {
      gfloat *src_pix               = src;
      gfloat  shadows_estimation    = *aux;
      gfloat  highlights_estimation = *aux2;

      gdouble correction;
      gdouble lut_value;
      gint    b;

      /* shadows correction */

      if (o->s_amount)
        {
          lut_value = luts->shadows[(gint) (shadows_estimation * (LUT_SIZE - 1))];

          if (lut_value > 0.0)
            {
              correction = o->s_amount * lut_value;

              for (b = 0; b < 3; b++)
                {
                  gfloat new_shadow = 1.0 - shadows_estimation;

                  if (src_pix[b] < 0.5)
                    new_shadow = 2.0 * src_pix[b] * new_shadow;
                  else
                    new_shadow = 1.0 - 2.0 * (1.0 - src_pix[b]) * (1.0 - new_shadow);

                  src_pix[b] = correction * new_shadow + (1.0 - correction) * src_pix[b];
                }
            }
        }

      /* highlights correction */

      if (o->h_amount)
        {
          lut_value = luts->highlights[(gint) (highlights_estimation * (LUT_SIZE - 1))];

          if (lut_value > 0.0)
            {
              correction = o->h_amount * lut_value;

              for (b = 0; b < 3; b++)
                {
                  gfloat new_highlight = 1.0 - highlights_estimation;

                  if (src_pix[b] < 0.5)
                    new_highlight = 2.0 * src_pix[b] * new_highlight;
                  else
                    new_highlight = 1.0 - 2.0 * (1.0 - src_pix[b]) * (1.0 - new_highlight);

                  src_pix[b] = correction * new_highlight + (1.0 - correction) * src_pix[b];
                }
            }
        }

      dst[0] = src_pix[0];
      dst[1] = src_pix[1];
      dst[2] = src_pix[2];
      dst[3] = src[3];

      src += 4;
      dst += 4;
      aux += 1;
      aux2 += 1;
    }

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
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass                     *object_class;
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_composer3_class;

  object_class          = G_OBJECT_CLASS (klass);
  operation_class       = GEGL_OPERATION_CLASS (klass);
  point_composer3_class = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  object_class->finalize            = finalize;

  operation_class->prepare          = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->opencl_support   = FALSE;

  point_composer3_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:shadows-highlights-correction",
    "categories",  "hidden",
    "description", _("Lighten shadows and darken highlights"),
    NULL);
}

#endif
