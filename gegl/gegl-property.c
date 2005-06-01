/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */
#include "gegl-property.h"
#include "gegl-visitor.h"
#include "gegl-utils.h"
#include "gegl-connection.h"
#include "gegl-visitable.h"

enum
{
  PROP_0,
  PROP_PARAM_SPEC,
  PROP_LAST
};

static void class_init (GeglPropertyClass * klass);
static void init (GeglProperty *self, GeglPropertyClass * klass);
static void finalize(GObject * gobject);
static void set_property (GObject *gobject, guint prop_id, const GValue *value, GParamSpec *pspec);
static void get_property (GObject *gobject, guint prop_id, GValue *value, GParamSpec *pspec);

static void visitable_init (gpointer ginterface, gpointer interface_data);
static void visitable_accept (GeglVisitable *visitable, GeglVisitor *visitor);
static GList* visitable_depends_on(GeglVisitable *visitable);
static gboolean visitable_needs_visiting (GeglVisitable *visitable);

static gpointer parent_class = NULL;

GType
gegl_property_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo typeInfo =
      {
        sizeof (GeglPropertyClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) class_init,
        (GClassFinalizeFunc) NULL,
        NULL,
        sizeof (GeglProperty),
        0,
        (GInstanceInitFunc) init,
        NULL,             /* value_table */
      };

      static const GInterfaceInfo visitable_info =
      {
         (GInterfaceInitFunc) visitable_init,
         NULL,
         NULL
      };

      type = g_type_register_static (GEGL_TYPE_OBJECT,
                                     "GeglProperty",
                                     &typeInfo,
                                     0);

      g_type_add_interface_static (type,
                                   GEGL_TYPE_VISITABLE,
                                   &visitable_info);

    }

    return type;
}

static void
class_init (GeglPropertyClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent(klass);

  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  /*
  g_object_class_install_property (gobject_class, PROP_PARAM_SPEC,
                   g_param_spec_param ("param_spec",
                                       "ParamSpec",
                                       "Param spec for the property",
                                       ????,
                                       G_PARAM_READWRITE));
                                       */
}

static void
init (GeglProperty * self,
      GeglPropertyClass * klass)
{
  self->param_spec = NULL;
  self->filter = NULL;
  self->connections = NULL;
  self->dirty = TRUE;
}

static void
visitable_init (gpointer ginterface,
                gpointer interface_data)
{
  GeglVisitableClass *visitable_class = ginterface;

  g_assert (G_TYPE_FROM_INTERFACE (ginterface) == GEGL_TYPE_VISITABLE);

  visitable_class->accept = visitable_accept;
  visitable_class->depends_on = visitable_depends_on;
  visitable_class->needs_visiting = visitable_needs_visiting;
}

static void
finalize(GObject *gobject)
{
  GeglProperty *self = GEGL_PROPERTY(gobject);

  g_assert(self->connections == NULL);

  G_OBJECT_CLASS(parent_class)->finalize(gobject);
}

static void
set_property (GObject      *gobject,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  /*GeglProperty * property = GEGL_PROPERTY(gobject);*/
  switch (prop_id)
  {
    default:
      break;
  }
}

static void
get_property (GObject      *gobject,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  /*GeglProperty * property = GEGL_PROPERTY(gobject);*/
  switch (prop_id)
  {
    default:
      break;
  }
}

GParamSpec *
gegl_property_get_param_spec (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), NULL);

  return self->param_spec;
}

void
gegl_property_set_param_spec (GeglProperty * self,
                              GParamSpec *param_spec)
{
  g_return_if_fail (GEGL_IS_PROPERTY (self));

  self->param_spec = param_spec;
}

GeglConnection *
gegl_property_connect(GeglProperty *sink,
                      GeglProperty *source)
{
  GeglConnection *connection;
  g_return_val_if_fail (GEGL_IS_PROPERTY (sink), NULL);
  g_return_val_if_fail (GEGL_IS_PROPERTY (source), NULL);

  connection = gegl_connection_new(NULL, sink, NULL, source);

  sink->connections = g_list_append(sink->connections, connection);
  source->connections = g_list_append(source->connections, connection);

  return connection;
}

void
gegl_property_disconnect (GeglProperty *sink,
                          GeglProperty *source,
                          GeglConnection *connection)
{
  g_return_if_fail (GEGL_IS_PROPERTY (sink));
  g_return_if_fail (GEGL_IS_PROPERTY (source));

  g_assert(sink == gegl_connection_get_sink_prop(connection));
  g_assert(source == gegl_connection_get_source_prop(connection));

  sink->connections = g_list_remove(sink->connections, connection);
  source->connections = g_list_remove(source->connections, connection);
}

GList*
gegl_property_get_connections (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), NULL);
  return self->connections;
}

gint
gegl_property_num_connections (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), -1);

  return g_list_length(self->connections);
}

GeglFilter*
gegl_property_get_filter (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), NULL);

  return self->filter;
}

/* List should be freed */
GList *
gegl_property_get_depends_on(GeglProperty *self)
{
  GList *depends_on = NULL;

  if(gegl_property_is_input(self) &&
     gegl_property_num_connections(self) == 1)
    {
      GeglConnection *connection = g_list_nth_data(self->connections, 0);

      if(connection)
        depends_on = g_list_append(depends_on, gegl_connection_get_source_prop(connection));
    }
  else if(gegl_property_is_output(self))
    {
      GList *input_properties = gegl_node_get_input_properties(GEGL_NODE(self->filter));
      depends_on = g_list_copy(input_properties);
    }

  return depends_on;
}

const gchar *
gegl_property_get_name (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), NULL);

  return g_param_spec_get_name(self->param_spec);
}

GeglProperty *
gegl_property_get_connected_to (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), NULL);

  if(gegl_property_is_input(self) &&
     gegl_property_num_connections(self) == 1)
    {
      GeglConnection *connection = g_list_nth_data(self->connections, 0);
      return gegl_connection_get_source_prop(connection);
    }

  return NULL;
}

void
gegl_property_set_filter (GeglProperty * self,
                          GeglFilter *filter)
{
  g_return_if_fail (GEGL_IS_PROPERTY (self));
  g_return_if_fail (GEGL_IS_FILTER (filter));

  self->filter = filter;
}

gboolean
gegl_property_is_dirty (GeglProperty * self)
{
  g_return_val_if_fail (GEGL_IS_PROPERTY (self), TRUE);
  return self->dirty;
}

void
gegl_property_set_dirty (GeglProperty * self,
                         gboolean flag)
{
  g_return_if_fail (GEGL_IS_PROPERTY (self));
  self->dirty = flag;
}

gboolean
gegl_property_is_output (GeglProperty * self)
{
  return GEGL_PROPERTY_OUTPUT & self->param_spec->flags;
}

gboolean
gegl_property_is_input (GeglProperty * self)
{
  return GEGL_PROPERTY_INPUT & self->param_spec->flags;
}

static void
visitable_accept (GeglVisitable * visitable,
                  GeglVisitor * visitor)
{
  gegl_visitor_visit_property(visitor, GEGL_PROPERTY(visitable));
}

static GList *
visitable_depends_on(GeglVisitable *visitable)
{
  GeglProperty * property = GEGL_PROPERTY(visitable);
  return gegl_property_get_depends_on(property);
}

static gboolean
visitable_needs_visiting(GeglVisitable *visitable)
{
  GeglProperty * property = GEGL_PROPERTY(visitable);
  return gegl_property_is_dirty(property);
}
