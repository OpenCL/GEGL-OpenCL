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
 * Original author:
 *     Tim Rowley <tor@cs.brown.edu>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE tile-seamless.c

#include "gegl-op.h"
#include <math.h>


static void
prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",
                             babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output",
                             babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  const GeglRectangle *whole_region;
  GeglRectangle        shift_region;
  GeglBufferIterator  *gi;
  gint                 half_width;
  gint                 half_height;
  gint                 index_iter;
  gint                 index_iter2;

  whole_region = gegl_operation_source_get_bounding_box (operation, "input");

  half_width  = whole_region->width / 2;
  half_height = whole_region->height / 2;

  shift_region.x = whole_region->x + half_width;
  shift_region.y = whole_region->y + half_height;
  shift_region.width  = whole_region->width;
  shift_region.height = whole_region->height;

  gi = gegl_buffer_iterator_new (output, whole_region,
                                 0, babl_format ("R'G'B'A float"),
                                 GEGL_ACCESS_WRITE, GEGL_ABYSS_NONE);

  index_iter = gegl_buffer_iterator_add (gi, input, whole_region,
                                         0, babl_format ("R'G'B'A float"),
                                         GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  index_iter2 = gegl_buffer_iterator_add (gi, input, &shift_region,
                                          0, babl_format ("R'G'B'A float"),
                                          GEGL_ACCESS_READ, GEGL_ABYSS_LOOP);

  while (gegl_buffer_iterator_next (gi))
    {
      guint   k;
      gfloat *data_out;
      gfloat *data_in1;
      gfloat *data_in2;

      data_out = (gfloat*) gi->data[0];
      data_in1 = (gfloat*) gi->data[index_iter];
      data_in2 = (gfloat*) gi->data[index_iter2];

      for (k = 0; k < gi->length; k++)
        {
          gint x, y, b;
          gfloat alpha;
          gfloat val_x, val_y;
          gfloat w, w1, w2;
          const gfloat eps = 1e-4;

          x = gi->roi[0].x + k % gi->roi[0].width;
          y = gi->roi[0].y + k / gi->roi[0].width;

          val_x = (half_width - x) / (gfloat) half_width;
          val_y = (half_height - y) / (gfloat) half_height;

          val_x = ABS (CLAMP (val_x, -1.0, 1.0));
          val_y = ABS (CLAMP (val_y, -1.0, 1.0));

          /* ambiguous position */
          if (ABS (val_x - val_y) >= 1.0 - eps)
            w = 0.0;
          else
            w = val_x * val_y / (val_x * val_y + (1.0 - val_x) * (1.0 - val_y));

          alpha = data_in1[3] * (1.0 - w) + data_in2[3] * w;

          w1 = (1.0 - w) * data_in1[3] / alpha;
          w2 = w * data_in2[3] / alpha;

          for (b = 0; b < 3; b++)
            data_out[b] = data_in1[b] * w1 + data_in2[b] * w2;

          data_out[3] = alpha;

          data_out += 4;
          data_in1 += 4;
          data_in2 += 4;
        }
    }

  return TRUE;
}

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process                    = process;
  operation_class->prepare                 = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region       = get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name",              "gegl:tile-seamless",
    "title",             _("Make Seamlessly tileable"),
    "categories",        "tile",
    "position-dependent", "true",
    "description", _("Make the input buffer seamlessly tileable."
                     " The algorithm is not content-aware,"
                     " so the result may need post-processing."),
    NULL);
}

#endif
