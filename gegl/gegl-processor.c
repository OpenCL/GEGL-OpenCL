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
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2007 Øyvind Kolås
 */

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-node.h"
#include "gegl-processor.h"
#include "gegl-operation-sink.h"

struct _GeglProcessor {
  GeglNode        *node;
  GeglRectangle    rectangle;
  GeglNode        *input;
  GeglNodeDynamic *dynamic;
};

void
gegl_processor_set_rectangle (GeglProcessor *processor,
                              GeglRectangle *rectangle)
{
  processor->rectangle = *rectangle;
}

GeglProcessor *
gegl_node_new_processor (GeglNode      *node,
                         GeglRectangle *rectangle)
{
  GeglProcessor *processor;
  GeglCache     *cache;

  g_assert (GEGL_IS_NODE (node));

  processor = g_malloc0 (sizeof (GeglProcessor));

  processor->node = node;

  if(node->operation &&
     g_type_is_a(G_OBJECT_TYPE(node->operation),
                 GEGL_TYPE_OPERATION_SINK))
    {
      processor->input = gegl_node_get_producer (node, "input", NULL);
    }
  else
    {
      processor->input = node;
    }

  if (rectangle)
    gegl_processor_set_rectangle (processor, rectangle);
  else
    {
      GeglRectangle tmp = gegl_node_get_bounding_box (processor->input);
      gegl_processor_set_rectangle (processor, &tmp);
    }

  cache = gegl_node_get_cache (processor->input);
  if(node->operation &&
     g_type_is_a(G_OBJECT_TYPE(node->operation),
                 GEGL_TYPE_OPERATION_SINK))
    {
      processor->dynamic = gegl_node_add_dynamic (node, cache);
        {
          GValue value = {0,};
          g_value_init (&value, GEGL_TYPE_BUFFER);
          g_value_set_object (&value, cache);
          gegl_node_dynamic_set_property (processor->dynamic, "input", &value);
          g_value_unset (&value);
        }

      gegl_node_dynamic_set_result_rect (processor->dynamic,
                   processor->rectangle.x, processor->rectangle.y,
                   processor->rectangle.w, processor->rectangle.h);
    }
  else
    {
      processor->dynamic = NULL;
    }

  return processor;
}

gboolean
gegl_processor_work (GeglProcessor *processor,
                     gdouble       *progress)
{
  gboolean more_work=FALSE;
  GeglCache *cache = gegl_node_get_cache (processor->input);

  more_work = gegl_cache_render (cache, &processor->rectangle, progress);
  if (more_work)
    {
      return TRUE;
    }

  if (processor->dynamic)
    {
      gegl_operation_process (processor->node->operation, cache, "foo");
      gegl_node_remove_dynamic (processor->node, cache);
      processor->dynamic=NULL;
      if (progress)
        *progress=1.0;
      return TRUE;
    }

  if (progress)
    *progress= 1.0;

  return FALSE;
}

void
gegl_processor_destroy (GeglProcessor *processor)
{
  gegl_node_disable_cache (processor->input); /* FIXME: it's a bit rude to kill of the cache */
  g_free (processor);
}
