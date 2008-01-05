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
 *           2006 Øyvind Kolås
 */

#include "config.h"

#include <stdio.h>
#include <string.h>

#include <glib-object.h>
#include "gegl-types.h"

#include "gegl-node-context.h"
#include "gegl-node.h"

static void     gegl_node_context_class_init   (GeglNodeContextClass  *klass);
static void     gegl_node_context_init         (GeglNodeContext *self);
static void     finalize                       (GObject         *gobject);
static GValue * gegl_node_context_get_value    (GeglNodeContext *self,
                                                const gchar     *property_name);
static GValue * gegl_node_context_add_value    (GeglNodeContext *self,
                                                const gchar     *property_name);

G_DEFINE_TYPE (GeglNodeContext, gegl_node_context, G_TYPE_OBJECT);

static void
gegl_node_context_class_init (GeglNodeContextClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;
}

static void
gegl_node_context_init (GeglNodeContext *self)
{
  self->refs = 0;
  self->cached = FALSE;
}

void
gegl_node_context_set_need_rect (GeglNodeContext *node,
                                 gint             x,
                                 gint             y,
                                 gint             width,
                                 gint             height)
{
  g_assert (node);
  node->need_rect.x      = x;
  node->need_rect.y      = y;
  node->need_rect.width  = width;
  node->need_rect.height = height;
}

GeglRectangle *
gegl_node_context_get_result_rect (GeglNodeContext *node)
{
  return &node->result_rect;
}

void
gegl_node_context_set_result_rect (GeglNodeContext *node,
                                   gint             x,
                                   gint             y,
                                   gint             width,
                                   gint             height)
{
  g_assert (node);
  node->result_rect.x      = x;
  node->result_rect.y      = y;
  node->result_rect.width  = width;
  node->result_rect.height = height;
}

GeglRectangle *
gegl_node_context_get_need_rect (GeglNodeContext *node)
{
  return &node->need_rect;
}

void
gegl_node_context_set_property (GeglNodeContext *context,
                                const gchar     *property_name,
                                const GValue    *value)
{
  GParamSpec *pspec;
  GValue     *storage;

  pspec = gegl_node_find_property (context->node, property_name);

  if (!pspec)
    {
      g_warning ("%s: node %s has no pad|property named '%s'",
                 G_STRFUNC,
                 gegl_node_get_debug_name (context->node),
                 property_name);
    }

  storage = gegl_node_context_add_value (context, property_name);
  /* storage needs to have the correct type */
  g_value_init (storage, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_value_copy (value, storage);
}

void
gegl_node_context_get_property (GeglNodeContext *context,
                                const gchar     *property_name,
                                GValue          *value)
{
  GParamSpec *pspec;
  GValue     *storage;

  pspec = gegl_node_find_property (context->node, property_name);

  if (!pspec)
    {
      g_warning ("%s: node %s has no pad|property named '%s'",
                 G_STRFUNC,
                 gegl_node_get_debug_name (context->node),
                 property_name);
    }

  storage = gegl_node_context_get_value (context, property_name);
  if (storage != NULL)
    {
      g_value_copy (storage, value);
    }
}

typedef struct Property
{
  gchar *name;
  GValue value;
} Property;

static Property *property_new (GeglNode    *node,
                               const gchar *property_name)
{
  Property *property = g_malloc0 (sizeof (Property));

  property->name = g_strdup (property_name);
  return property;
}

static void property_destroy (Property *property)
{
  if (property->name)
    g_free (property->name);
  g_value_unset (&property->value); /* does an unref */
  g_free (property);
}

static gint
lookup_property (gconstpointer a,
                 gconstpointer property_name)
{
  Property *property = (void *) a;

  return strcmp (property->name, property_name);
}

static GValue *
gegl_node_context_get_value (GeglNodeContext *self,
                             const gchar     *property_name)
{
  Property *property = NULL;

  {
    GSList *found;
    found = g_slist_find_custom (self->property, property_name, lookup_property);
    if (found)
      property = found->data;
  }
  if (!property)
    {
      return NULL;
    }
  return &property->value;
}

void
gegl_node_context_remove_property (GeglNodeContext *self,
                                   const gchar     *property_name)
{
  Property *property = NULL;

  {
    GSList *found;
    found = g_slist_find_custom (self->property, property_name, lookup_property);
    if (found)
      property = found->data;
  }
  if (!property)
    {
      g_warning ("didn't find context %p for %s", property_name, gegl_node_get_debug_name (self->node));
      return;
    }
  self->property = g_slist_remove (self->property, property);
  property_destroy (property);
}

static GValue *
gegl_node_context_add_value (GeglNodeContext *self,
                             const gchar     *property_name)
{
  Property *property = NULL;
  GSList   *found    = g_slist_find_custom (self->property, property_name, lookup_property);

  if (found)
    property = found->data;

  if (property)
    {
      return &property->value;
    }

  property = property_new (self->node, property_name);

  self->property = g_slist_prepend (self->property, property);

  return &property->value;
}

static void
finalize (GObject *gobject)
{
  GeglNodeContext *self = GEGL_NODE_CONTEXT (gobject);

  while (self->property)
    {
      Property *property = self->property->data;
      self->property = g_slist_remove (self->property, property);
      property_destroy (property);
    }

  G_OBJECT_CLASS (gegl_node_context_parent_class)->finalize (gobject);
}

