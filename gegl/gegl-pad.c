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
#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-node.h"
#include "gegl-pad.h"
#include "gegl-visitor.h"
#include "gegl-utils.h"
#include "gegl-connection.h"
#include "gegl-visitable.h"


static void       gegl_pad_class_init      (GeglPadClass  *klass);
static void       gegl_pad_init            (GeglPad       *self);
static void       finalize                 (GObject       *gobject);
static void       visitable_init           (gpointer       ginterface,
                                            gpointer       interface_data);
static void       visitable_accept         (GeglVisitable *visitable,
                                            GeglVisitor   *visitor);
static GList    * visitable_depends_on     (GeglVisitable *visitable);
static gboolean   visitable_needs_visiting (GeglVisitable *visitable);


G_DEFINE_TYPE_WITH_CODE (GeglPad, gegl_pad, GEGL_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GEGL_TYPE_VISITABLE,
                                                visitable_init))

static void
gegl_pad_class_init (GeglPadClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
}

static void
gegl_pad_init (GeglPad *self)
{
  self->param_spec  = NULL;
  self->node        = NULL;
  self->connections = NULL;
}

static void
visitable_init (gpointer ginterface,
                gpointer interface_data)
{
  GeglVisitableClass *visitable_class = ginterface;

  visitable_class->accept         = visitable_accept;
  visitable_class->depends_on     = visitable_depends_on;
  visitable_class->needs_visiting = visitable_needs_visiting;
}

static void
finalize (GObject *gobject)
{
  GeglPad *self = GEGL_PAD (gobject);

  g_assert (self->connections == NULL);

  G_OBJECT_CLASS (gegl_pad_parent_class)->finalize (gobject);
}

GParamSpec *
gegl_pad_get_param_spec (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), NULL);

  return self->param_spec;
}

void
gegl_pad_set_param_spec (GeglPad    *self,
                         GParamSpec *param_spec)
{
  g_return_if_fail (GEGL_IS_PAD (self));

  self->param_spec = param_spec;
  gegl_object_set_name (GEGL_OBJECT (self), g_param_spec_get_name (param_spec));
}

GeglConnection *
gegl_pad_connect (GeglPad *sink,
                  GeglPad *source)
{
  GeglConnection *connection;

  g_return_val_if_fail (GEGL_IS_PAD (sink), NULL);
  g_return_val_if_fail (GEGL_IS_PAD (source), NULL);

  connection = gegl_connection_new (NULL, sink, NULL, source);

  sink->connections   = g_list_append (sink->connections, connection);
  source->connections = g_list_append (source->connections, connection);

  return connection;
}

void
gegl_pad_disconnect (GeglPad        *sink,
                     GeglPad        *source,
                     GeglConnection *connection)
{
  g_return_if_fail (GEGL_IS_PAD (sink));
  g_return_if_fail (GEGL_IS_PAD (source));

  g_assert (sink == gegl_connection_get_sink_prop (connection));
  g_assert (source == gegl_connection_get_source_prop (connection));

  sink->connections   = g_list_remove (sink->connections, connection);
  source->connections = g_list_remove (source->connections, connection);
}

GList *
gegl_pad_get_connections (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), NULL);

  return self->connections;
}

gint
gegl_pad_get_num_connections (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), -1);

  return g_list_length (self->connections);
}

GeglNode *
gegl_pad_get_node (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), NULL);

  return self->node;
}

/* List should be freed */
GList *
gegl_pad_get_depends_on (GeglPad *self)
{
  GList *depends_on = NULL;

  if (gegl_pad_is_input (self))
    {
      gint i;
      for (i=0; i<g_list_length (self->connections); i++)
       {
          GeglConnection *connection = g_list_nth_data (self->connections, i);
          if (connection)
            {
              depends_on = g_list_append (depends_on,
                                          gegl_connection_get_source_prop (connection));
            }
          else
            {
              g_warning ("hmm,. or perhaps just a non connected pad");
            }
       }
      if (!strcmp (gegl_object_get_name (GEGL_OBJECT (self->node)), "proxynop-input"))
        {
          GeglNode *graph = GEGL_NODE (g_object_get_data (G_OBJECT (self->node), "graph"));
          GList *llink = graph->sources;

          for (llink = graph->sources; llink; llink = g_list_next (llink))
            {
              GeglConnection *connection = llink->data;
              depends_on = g_list_append (depends_on,
                                          gegl_connection_get_source_prop (connection));
            }
        }

    }
  else if (gegl_pad_is_output (self))
    {
      GList *input_pads = gegl_node_get_input_pads (self->node);
          gint i;

      depends_on = g_list_copy (input_pads);
    }

  return depends_on;
}

const gchar *
gegl_pad_get_name (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), NULL);
  return gegl_object_get_name (GEGL_OBJECT (self));
}

GeglPad *
gegl_pad_get_connected_to (GeglPad *self)
{
  g_return_val_if_fail (GEGL_IS_PAD (self), NULL);

  if (gegl_pad_is_input (self) &&
      gegl_pad_get_num_connections (self) == 1)
    {
      GeglConnection *connection = g_list_nth_data (self->connections, 0);

      return gegl_connection_get_source_prop (connection);
    }

  return NULL;
}

void
gegl_pad_set_node (GeglPad  *self,
                   GeglNode *node)
{
  g_return_if_fail (GEGL_IS_PAD (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  self->node = node;
}

gboolean
gegl_pad_is_output (GeglPad *self)
{
  return GEGL_PAD_OUTPUT & self->param_spec->flags;
}

gboolean
gegl_pad_is_input (GeglPad *self)
{
  return GEGL_PAD_INPUT & self->param_spec->flags;
}

static void
visitable_accept (GeglVisitable *visitable,
                  GeglVisitor   *visitor)
{
  gegl_visitor_visit_pad (visitor, GEGL_PAD (visitable));
}

static GList *
visitable_depends_on (GeglVisitable *visitable)
{
  GeglPad *self = GEGL_PAD (visitable);

  return gegl_pad_get_depends_on (self);
}

static gboolean
visitable_needs_visiting (GeglVisitable *visitable)
{
  return TRUE;
}
