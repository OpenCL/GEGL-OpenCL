/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
      case PROP_CHUNK_SIZE:
        self->chunk_size = g_value_get_int (value);
        break;
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Øyvind Kolås
 */
#include "gegl-operation-point-composer.h"
#include "gegl-utils.h"
#include <string.h>

static gboolean process_inner (GeglOperation *operation,
                               gpointer       context_id);

G_DEFINE_TYPE (GeglOperationPointComposer, gegl_operation_point_composer, GEGL_TYPE_OPERATION_COMPOSER)


static void
gegl_operation_point_composer_class_init (GeglOperationPointComposerClass * klass)
{
  /*GObjectClass       *object_class    = G_OBJECT_CLASS (klass);*/
  GeglOperationComposerClass *composer_class = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = process_inner;
}

static void
gegl_operation_point_composer_init (GeglOperationPointComposer *self)
{
  self->format = babl_format ("RGBA float"); /* default to RGBA float for
                                                processing */
  self->aux_format = babl_format ("RGBA float"); /* default to RGBA float for
                                                    processing */
}

static gboolean
process_inner (GeglOperation *operation,
               gpointer       context_id)
{
  GeglOperationPointComposer *point_composer = GEGL_OPERATION_POINT_COMPOSER (operation);

  GeglBuffer * input = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "input"));
  GeglBuffer * aux = GEGL_BUFFER (gegl_operation_get_data (operation, context_id, "aux"));

  GeglRectangle *result = gegl_operation_result_rect (operation, context_id);

  if (!input && aux)
    {
        g_object_ref (aux);
        gegl_operation_set_data (operation, context_id, "output", G_OBJECT (aux));
        return TRUE;
    }

  {
    gfloat *buf = NULL, *aux_buf = NULL;

    g_assert (gegl_buffer_get_format (input));


    if ( (result->w>0) && (result->h>0))
      {
        GeglBuffer *output;

	const gchar *op = gegl_node_get_operation (operation->node);
	if (!strcmp (op, "over"))
{
#define SKIP_EMPTY_IN
#define SKIP_EMPTY_AUX
#ifdef SKIP_EMPTY_IN
       {
	GeglRectangle in_abyss;

	in_abyss = gegl_buffer_get_abyss (input);

	if ((!input ||
            !gegl_rect_intersect (NULL, &in_abyss, result)) &&
            aux)
          {
	    GeglRectangle aux_abyss;
	    aux_abyss = gegl_buffer_get_abyss (aux);

            if(!gegl_rect_intersect (NULL, &aux_abyss, result))
              {
                GeglBuffer *output = g_object_new (GEGL_TYPE_BUFFER,
                                     "format", point_composer->format,
                                     "x",      0,
                                     "y",      0,
                                     "width",  0,
                                     "height", 0,
                                     NULL);
                gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
                return TRUE;
              }
            g_object_ref (aux);
            gegl_operation_set_data (operation, context_id, "output", G_OBJECT (aux));
            return TRUE;
          }
        }
#endif

#ifdef SKIP_EMPTY_AUX
       {
	GeglRectangle aux_abyss;

	if (aux)
	  aux_abyss = gegl_buffer_get_abyss (aux);

	if (!aux ||
            !gegl_rect_intersect (NULL, &aux_abyss, result))
          {
            g_object_ref (input);
            gegl_operation_set_data (operation, context_id, "output", G_OBJECT (input));
            return TRUE;
          }
        }
#endif
}
        output = g_object_new (GEGL_TYPE_BUFFER,
                               "format", point_composer->format,
                               "x",      result->x,
                               "y",      result->y,
                               "width",  result->w,
                               "height", result->h,
                               NULL);

        buf = g_malloc (4 * sizeof (gfloat) * gegl_buffer_pixels (output));

        if (aux)
          aux_buf = g_malloc (4 * sizeof (gfloat) * gegl_buffer_pixels (output));

        gegl_buffer_get (input, result, buf, point_composer->format, 1.0);
          
        if (aux)
          {
            gegl_buffer_get (aux, result, aux_buf, point_composer->aux_format, 1.0);
          }
          {
            GEGL_OPERATION_POINT_COMPOSER_GET_CLASS (operation)->process (
               operation,
               buf,
               aux_buf,
               buf,
               gegl_buffer_pixels (output));
          }

        gegl_buffer_set (output, result, buf, point_composer->format);

        g_free (buf);
        if (aux)
          g_free (aux_buf);

          gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
        }
      else
        {
          GeglBuffer *output = g_object_new (GEGL_TYPE_BUFFER,
                               "format", point_composer->format,
                               "x",      0,
                               "y",      0,
                               "width",  0,
                               "height", 0,
                               NULL);
          gegl_operation_set_data (operation, context_id, "output", G_OBJECT (output));
        }
      }
  return  TRUE;
}
