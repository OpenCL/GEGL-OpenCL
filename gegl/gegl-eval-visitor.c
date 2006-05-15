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
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-eval-visitor.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-pad.h"
#include "gegl-visitable.h"


static void gegl_eval_visitor_class_init (GeglEvalVisitorClass *klass);
static void visit_pad                    (GeglVisitor          *self,
                                          GeglPad              *pad);


G_DEFINE_TYPE(GeglEvalVisitor, gegl_eval_visitor, GEGL_TYPE_VISITOR)


static void
gegl_eval_visitor_class_init (GeglEvalVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_pad = visit_pad;
}

static void
gegl_eval_visitor_init (GeglEvalVisitor *self)
{
}

static void
visit_pad (GeglVisitor *self,
           GeglPad     *pad)
{
  GeglNode      *node      = gegl_pad_get_node (pad);
  GeglOperation *operation = node->operation;

  GEGL_VISITOR_CLASS (gegl_eval_visitor_parent_class)->visit_pad (self, pad);
#if 0
  g_print("Compute Visitor: Visiting pad %s from operation %s\n",
          gegl_pad_get_name(pad),
          gegl_object_get_name(GEGL_OBJECT(operation)));
#endif

  if (gegl_pad_is_output (pad))
    {
      const gchar *pad_name = gegl_pad_get_name (pad);
      gboolean     success;

      /* XXX: this is probably correct anyways */
      success = gegl_operation_evaluate (operation, pad_name);
    }
  else if (gegl_pad_is_input (pad))
    {
      GeglPad *source_pad = gegl_pad_get_connected_to (pad);

      if (source_pad)
        {
          GValue      value     = { 0 };
          GParamSpec *prop_spec = gegl_pad_get_param_spec (pad);
          GeglOperation *source = gegl_pad_get_node (source_pad)->operation;

          g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (prop_spec));

          g_object_get_property (G_OBJECT(source),
                                 gegl_pad_get_name (source_pad),
                                 &value);
          g_object_set_property (G_OBJECT (operation),
                                 gegl_pad_get_name (pad),
                                 &value);
        }
    }
}
