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
 * Copyright 2009 Martin Nordholts
 */

#include "config.h"
#include <string.h>
#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-dot.h"
#include "gegl-dot-visitor.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"


struct _GeglDotVisitorPriv
{
  GString *string_to_append;
};


static void gegl_dot_visitor_class_init (GeglDotVisitorClass *klass);
static void gegl_dot_visitor_visit_node (GeglVisitor         *self,
                                         GeglNode            *node);
static void gegl_dot_visitor_visit_pad  (GeglVisitor         *self,
                                         GeglPad             *pad);


G_DEFINE_TYPE (GeglDotVisitor, gegl_dot_visitor, GEGL_TYPE_VISITOR)


static void
gegl_dot_visitor_class_init (GeglDotVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);

  visitor_class->visit_node = gegl_dot_visitor_visit_node;
  visitor_class->visit_pad = gegl_dot_visitor_visit_pad;

  g_type_class_add_private (klass, sizeof (GeglDotVisitorPriv));
}

static void
gegl_dot_visitor_init (GeglDotVisitor *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            GEGL_TYPE_DOT_VISITOR,
                                            GeglDotVisitorPriv);
}

void
gegl_dot_visitor_set_string_to_append (GeglDotVisitor *self,
                                       GString        *string_to_append)
{
  self->priv->string_to_append = string_to_append;
}

static void
gegl_dot_visitor_visit_node (GeglVisitor *visitor,
                             GeglNode    *node)
{
  GeglDotVisitor *self = GEGL_DOT_VISITOR (visitor);

  g_return_if_fail (self->priv->string_to_append != NULL);

  GEGL_VISITOR_CLASS (gegl_dot_visitor_parent_class)->visit_node (visitor, node);

  gegl_dot_util_add_node (self->priv->string_to_append, node);
}

static void
gegl_dot_visitor_visit_pad (GeglVisitor *visitor,
                            GeglPad     *pad)
{
  GeglDotVisitor *self = GEGL_DOT_VISITOR (visitor);
  GSList         *pads = gegl_pad_get_depends_on (pad);
  GSList         *iter;
  GSList         *pad_iter;

  g_return_if_fail (self->priv->string_to_append != NULL);

  GEGL_VISITOR_CLASS (gegl_dot_visitor_parent_class)->visit_pad (visitor, pad);

  for (pad_iter = pads; pad_iter; pad_iter = g_slist_next (pad_iter))
    {
      GeglPad *pad = GEGL_PAD (pad_iter->data);

      /* Only use connections of input pads, otherwise we get
       * duplicate edges in the graphviz file
       */
      if (!gegl_pad_is_input (pad))
        continue;

      for (iter = pad->connections; iter; iter = g_slist_next (iter))
        {
          GeglConnection *connection = iter->data;
          gegl_dot_util_add_connection (self->priv->string_to_append, connection);
        }
    }

  g_slist_free (pads);
}
