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
 * Copyright 2013 Daniel Sabo
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-debug.h"
#include "gegl-types-internal.h"
#include "gegl-list-visitor.h"
#include "graph/gegl-node-private.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "gegl-instrument.h"
#include "operation/gegl-operation.h"

static void gegl_list_visitor_class_init (GeglListVisitorClass *klass);
static void gegl_list_visitor_visit_node (GeglVisitor          *self,
                                          GeglNode             *node);
static void gegl_list_visitor_finalize   (GObject              *visitor);

G_DEFINE_TYPE (GeglListVisitor, gegl_list_visitor, GEGL_TYPE_VISITOR)


static void
gegl_list_visitor_class_init (GeglListVisitorClass *klass)
{
  GeglVisitorClass *visitor_class = GEGL_VISITOR_CLASS (klass);
  GObjectClass     *gobject_class = G_OBJECT_CLASS (klass);

  visitor_class->visit_node = gegl_list_visitor_visit_node;
  gobject_class->finalize   = gegl_list_visitor_finalize;
}

static void
gegl_list_visitor_init (GeglListVisitor *visitor)
{
  GeglListVisitor *list_visitor = GEGL_LIST_VISITOR(visitor);

  list_visitor->visit_path = NULL;
}

static void
gegl_list_visitor_finalize (GObject *object)
{
  GeglListVisitor *list_visitor = GEGL_LIST_VISITOR(object);

  if (list_visitor->visit_path)
    {
      g_list_free (list_visitor->visit_path);
      list_visitor->visit_path = NULL;
    }

  G_OBJECT_CLASS (gegl_list_visitor_parent_class)->finalize (object);
}

void
gegl_list_visitor_reset (GeglListVisitor *self)
{
  gegl_visitor_reset (GEGL_VISITOR (self));
  if (self->visit_path)
    {
      g_list_free (self->visit_path);
      self->visit_path = NULL;
    }
}

GList *
gegl_list_visitor_get_dfs_path (GeglListVisitor *self,
                                GeglVisitable   *visitable)
{
  GList *result;

  gegl_list_visitor_reset (self);
  gegl_visitor_dfs_traverse (GEGL_VISITOR (self), visitable);

  result = self->visit_path;
  self->visit_path = NULL;
  return g_list_reverse (result);
}

GList *
gegl_list_visitor_get_bfs_path (GeglListVisitor *self,
                                GeglVisitable   *visitable)
{
  GList *result;

  gegl_list_visitor_reset (self);
  gegl_visitor_bfs_traverse (GEGL_VISITOR (self), visitable);

  result = self->visit_path;
  self->visit_path = NULL;
  return g_list_reverse (result);
}

static void
gegl_list_visitor_visit_node (GeglVisitor *visitor,
                              GeglNode    *node)
{
  GeglListVisitor *list_visitor = GEGL_LIST_VISITOR(visitor);

  list_visitor->visit_path = g_list_prepend (list_visitor->visit_path, node);
}
