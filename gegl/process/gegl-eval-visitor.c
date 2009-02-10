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
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"

#include "gegl-eval-visitor.h"
#include "graph/gegl-node.h"
#include "operation/gegl-operation.h"
#include "operation/gegl-operation-context.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "gegl-instrument.h"
#include "operation/gegl-operation-sink.h"
#include "buffer/gegl-region.h"


static void gegl_eval_visitor_class_init (GeglEvalVisitorClass *klass);
static void gegl_eval_visitor_visit_pad  (GeglVisitor *self,
                                          GeglPad     *pad);


G_DEFINE_TYPE (GeglEvalVisitor, gegl_eval_visitor, GEGL_TYPE_VISITOR)


static void
gegl_eval_visitor_class_init (GeglEvalVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_pad = gegl_eval_visitor_visit_pad;
}

static void
gegl_eval_visitor_init (GeglEvalVisitor *self)
{
}

extern long babl_total_usecs;

/* this is the visitor that does the real computations for GEGL */
static void
gegl_eval_visitor_visit_pad (GeglVisitor *self,
                             GeglPad     *pad)
{
  GeglNode        *node       = gegl_pad_get_node (pad);
  gpointer         context_id = self->context_id;
  GeglOperationContext *context    = gegl_node_get_context (node, context_id);
  GeglOperation   *operation  = node->operation;

  GEGL_VISITOR_CLASS (gegl_eval_visitor_parent_class)->visit_pad (self, pad);

  if (gegl_pad_is_output (pad))
    {
      /* processing only really happens for output pads */
      if (context->cached)
        { /* unless we've got a cache valid for the requested region 
           * that has not been invalidated, (the validity of the cache
           * is determined by other visitors)
           */
          gegl_operation_context_get_target (context, pad->name);
          /* XXX: why is the _get_target call needed anyways? */
        }
      else
        {
          glong time      = gegl_ticks ();
          glong babl_time = babl_total_usecs;

          /* Make the operation do it's actual processing */
          gegl_operation_process (operation, context, gegl_pad_get_name (pad),
                                  &context->result_rect);
          babl_time = babl_total_usecs - babl_time;
          time      = gegl_ticks () - time;

          gegl_instrument ("process", gegl_node_get_operation (node), time);
          gegl_instrument (gegl_node_get_operation (node), "babl", babl_time);

          if (node->cache)
            {
              /* if we've got a cache, notify the cache that parts
               * of it has been computed
               */
              gegl_cache_computed (node->cache, &context->result_rect);
            }
        }
    }
  else if (gegl_pad_is_input (pad))
    {
      GeglPad *source_pad = gegl_pad_get_internal_connected_to (pad);

      /* the work needed to be done on input pads is to set the
       * data from the corresponding output pad it is connected to
       */
      if (source_pad)
        {
          GValue           value          = { 0 };
          GParamSpec      *prop_spec      = gegl_pad_get_param_spec (pad);
          GeglNode        *source_node    = gegl_pad_get_node (source_pad);
          GeglOperationContext *source_context = gegl_node_get_context (source_node, context_id);

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));

          gegl_operation_context_get_property (source_context,
                                          gegl_pad_get_name (source_pad),
                                          &value);

          if (!g_value_get_object (&value) &&
              !g_object_get_data (G_OBJECT (source_node), "graph"))
            g_warning ("eval-visitor encountered a NULL buffer passed from: %s.%s-[%p]",
                       gegl_node_get_debug_name (source_node),
                       gegl_pad_get_name (source_pad),
                       g_value_get_object (&value));

          gegl_operation_context_set_property (context,
                                          gegl_pad_get_name (pad),
                                          &value);
          /* reference counting for this source dropped to zero, freeing up */
          if (-- gegl_node_get_context (
                     gegl_pad_get_node (source_pad), context_id)->refs == 0 &&
              g_value_get_object (&value))
            {
              gegl_operation_context_remove_property (
                 gegl_node_get_context (
                    gegl_pad_get_node (source_pad), context_id),
                    gegl_pad_get_name (source_pad));
            }

          g_value_unset (&value);

	  /* processing for sink operations that accepts partial consumption
             and thus probably are being processed by the processor from the
             this very operation.
           */
          if (GEGL_IS_OPERATION_SINK (operation) &&
              !gegl_operation_sink_needs_full (operation))
            {
              gegl_operation_process (operation, context, "output",
                &context->result_rect);
            }
        }
    }
}
