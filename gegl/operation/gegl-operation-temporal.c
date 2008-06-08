/* This file is part of GEGL
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
 * Copyright 2008 Øyvind Kolås
 */

#define GEGL_INTERNAL

#include "config.h"

#include <glib-object.h>
#include <string.h>
#include "gegl-types.h"
#include "gegl-operation-temporal.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"

struct _GeglOperationTemporalPrivate
{
  gint                count;
  gint                history_length;

  gint                width;
  gint                height;
  gint                next_to_write;
  GeglBuffer         *frame_store;
};

static void gegl_operation_temporal_prepare (GeglOperation *operation);

G_DEFINE_TYPE (GeglOperationTemporal,
               gegl_operation_temporal,
               GEGL_TYPE_OPERATION_FILTER)

#define GEGL_OPERATION_TEMPORAL_GET_PRIVATE(obj) \
  G_TYPE_INSTANCE_GET_PRIVATE (obj, GEGL_TYPE_OPERATION_TEMPORAL, GeglOperationTemporalPrivate)



GeglBuffer *
gegl_operation_temporal_get_frame (GeglOperation *op,
                                          gint           frame)
{
  GeglOperationTemporal *temporal= GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = temporal->priv;
  GeglRectangle rect = {0, 0, priv->width, priv->height};
  GeglBuffer   *buffer;

   if (frame * -1 > priv->count)
     {
       g_print ("%i > priv->count(%i), using frame 0", frame*-1, priv->count);
       frame = 0;
     }
   else
     {
       frame = (priv->next_to_write - 1 + priv->history_length + frame) % priv->history_length;
     }

  rect.y = frame * priv->height;
  buffer = gegl_buffer_create_sub_buffer (priv->frame_store, &rect);
  return buffer; 
}

static gboolean gegl_operation_temporal_process (GeglOperation       *self,
                                                 GeglBuffer          *input,
                                                 GeglBuffer          *output,
                                                 const GeglRectangle *result)
{
  GeglOperationTemporal *temporal = GEGL_OPERATION_TEMPORAL (self);
  GeglOperationTemporalPrivate *priv = temporal->priv;
  GeglOperationTemporalClass *temporal_class;
  GeglBuffer *temp_in;
  gint        pixels = result->width * result->height;
  gfloat     *buf    = g_new (gfloat, pixels * 4 * 4);

  temporal_class = GEGL_OPERATION_TEMPORAL_GET_CLASS (self);

  priv->width  = result->width;
  priv->height = result->height;

  temp_in = gegl_buffer_create_sub_buffer (input, result);
  gegl_buffer_get (temp_in, 1.0, result,
                  babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
  g_object_unref (temp_in);

  {
   GeglRectangle write_rect = *result;
   write_rect.y = priv->next_to_write * priv->height;


   gegl_buffer_set (priv->frame_store, &write_rect,
                    babl_format ("RGBA float"), buf, GEGL_AUTO_ROWSTRIDE);
   priv->count++;
   priv->next_to_write++;
   if (priv->next_to_write >= priv->history_length)
     priv->next_to_write=0;
  }
  g_free (buf);

 if (temporal_class->process)
   return temporal_class->process (self, input, output, result);
 return FALSE;
}

static void gegl_operation_temporal_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
  gegl_operation_set_format (operation, "input", babl_format ("RGBA float"));
}

static void
gegl_operation_temporal_class_init (GeglOperationTemporalClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *operation_filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = gegl_operation_temporal_prepare;
  operation_filter_class->process = gegl_operation_temporal_process;
}

static void
gegl_operation_temporal_init (GeglOperationTemporal *self)
{
  GeglOperationTemporalPrivate *priv;
  self->priv = GEGL_OPERATION_TEMPORAL_GET_PRIVATE(self);
  priv=self->priv;
  priv->count          = 0;
  priv->history_length = 500;
  priv->width          = 1024;
  priv->height         = 1024;
  priv->next_to_write  = 0;

  /* FIXME: the format used for the frame_store should be autodetected from
   * input
   */
  priv->frame_store    =
      gegl_buffer_new (&((GeglRectangle){0,0,4096,4096*600}), babl_format ("RGBA u8"));
;
}

void gegl_operation_temporal_set_history_length (GeglOperation *op,
                                                 gint           history_length)
{
  GeglOperationTemporal *self = GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = self->priv;
  priv->history_length = history_length;
}

guint gegl_operation_temporal_get_history_length (GeglOperation *op)
{
  GeglOperationTemporal *self = GEGL_OPERATION_TEMPORAL (op);
  GeglOperationTemporalPrivate *priv = self->priv;
  return priv->history_length;
}
