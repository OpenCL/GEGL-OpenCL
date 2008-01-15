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
#include "gegl-pad.h"
#include "operation/gegl-operation.h"

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

void
gegl_node_context_set_object (GeglNodeContext *context,
                              const gchar     *padname,
                              GObject         *data)
{
  GeglNode      *node;
  GeglOperation *operation;
  GParamSpec    *pspec;
  GValue         value = {0,};

  /* FIXME: check that there isn't already an existing 
   *        output object/value set?
   */

  node = context->node;
  operation = node->operation;
  pspec = gegl_node_find_property (node, padname);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_value_set_object (&value, data);
  gegl_node_context_set_property (context, padname, &value);
  g_value_unset (&value);
  g_object_unref (data); /* are we stealing the initial reference? */
}

GObject *
gegl_node_context_get_object (GeglNodeContext *context,
                              const gchar     *padname)
{
  GeglNode      *node;
  GeglOperation *operation;
  GObject       *ret;
  GParamSpec    *pspec;
  GValue         value = { 0, };

  node = context->node;
  operation = node->operation;

  pspec = gegl_node_find_property (node, padname);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  gegl_node_context_get_property (context, padname, &value);
  /* FIXME: handle other things than gobjects as well? */
  ret = g_value_get_object (&value);

  if (!ret)
    {/*
        g_warning ("some important data was not found on %s.%s",
        gegl_node_get_debug_name (node), property_name);
      */
    }
  g_value_unset (&value);
  return ret;
}

GeglBuffer *
gegl_node_context_get_target (GeglNodeContext *context,
                              const gchar     *padname)
{
  GeglBuffer          *output;
  GeglPad             *pad;
  const GeglRectangle *result;
  const Babl          *format;
  GeglNode            *node;
  GeglOperation       *operation;

  g_return_val_if_fail (GEGL_IS_NODE_CONTEXT (context), NULL);

  node = context->node;
  operation = node->operation;

  pad = gegl_node_get_pad (node, padname);
  format = gegl_pad_get_format (pad);

  if (format == NULL)
    {
      g_warning ("no format for %s presuming RGBA float\n", gegl_node_get_debug_name (node));
      format = babl_format ("RGBA float");
    }
  g_assert (format != NULL);
  g_assert (!strcmp (padname, "output"));

  result = &context->result_rect;

#if 1 /* change to 0 to disable per node caches */
  if (GEGL_OPERATION_CLASS (G_OBJECT_GET_CLASS (operation))->no_cache)
    {
      output = gegl_buffer_new (result, format);
    }
  else
    {
      GeglBuffer    *cache;
      cache = GEGL_BUFFER (gegl_node_get_cache (node));
      output = gegl_buffer_create_sub_buffer (cache, result);
    }
#else
  output = gegl_buffer_new (result, format);
#endif

  gegl_node_context_set_object (context, padname, G_OBJECT (output));
  return output;
}


GeglBuffer *
gegl_node_context_get_source (GeglNodeContext *context,
                              const gchar     *padname)
{
  GeglNode            *node;
  GeglOperation       *operation;
  GeglBuffer      *real_input;
  GeglBuffer      *input;
  GeglRectangle    input_request;
 
  node = context->node;
  operation = node->operation;
  input_request  = gegl_operation_compute_input_request (operation,
                                                         padname,
                                                         &context->need_rect);

  real_input = GEGL_BUFFER (gegl_node_context_get_object (context, padname));
  if (!real_input)
    return NULL;
  input = gegl_buffer_create_sub_buffer (real_input, &input_request);
  return input;
}
