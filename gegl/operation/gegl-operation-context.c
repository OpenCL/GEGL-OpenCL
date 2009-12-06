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
 *           2006-2008 Øyvind Kolås
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl.h"
#include "gegl/gegl-utils.h"
#include "gegl-types-internal.h"
#include "gegl-operation-context.h"
#include "gegl/graph/gegl-node.h"
#include "gegl-config.h"

#include "operation/gegl-operation.h"

static GValue *
gegl_operation_context_add_value (GeglOperationContext *self,
                                  const gchar          *property_name,
                                  GType                 proptype);


void
gegl_operation_context_set_need_rect (GeglOperationContext *self,
                                      const GeglRectangle  *rect)
{
  g_assert (self);
  self->need_rect = *rect;
}

GeglRectangle *
gegl_operation_context_get_result_rect (GeglOperationContext *self)
{
  return &self->result_rect;
}

void
gegl_operation_context_set_result_rect (GeglOperationContext *self,
                                        const GeglRectangle  *rect)
{
  g_assert (self);
  self->result_rect = *rect;
}

GeglRectangle *
gegl_operation_context_get_need_rect (GeglOperationContext *self)
{
  return &self->need_rect;
}

void
gegl_operation_context_set_property (GeglOperationContext *context,
                                     const gchar          *property_name,
                                     const GValue         *value)
{
  GParamSpec *pspec;
  GValue     *storage;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (context->operation)), property_name);

  if (!pspec)
    {
      g_warning ("%s: node %s has no pad|property named '%s'",
                 G_STRFUNC,
                 GEGL_OPERATION_GET_CLASS (context->operation)->name,
                 property_name);
    }

  /* if the value already exists in the context it will be reused */
  storage = gegl_operation_context_add_value (context, property_name, G_PARAM_SPEC_VALUE_TYPE(pspec));
  /* storage needs to have the correct type */
  g_value_copy (value, storage);
}

void
gegl_operation_context_get_property (GeglOperationContext *context,
                                     const gchar          *property_name,
                                     GValue               *value)
{
  GParamSpec *pspec;
  GValue     *storage;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (context->operation)), property_name);

  if (!pspec)
    {
      g_warning ("%s: node %s has no pad|property named '%s'",
                 G_STRFUNC,
                 GEGL_OPERATION_GET_CLASS (context->operation)->name,
                 property_name);
    }

  storage = gegl_operation_context_get_value (context, property_name);
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

static Property *
property_new (const gchar *property_name)
{
  Property *property = g_slice_new0 (Property);

  property->name = g_strdup (property_name);
  return property;
}

static void
property_destroy (Property *property)
{
  g_free (property->name);
  g_value_unset (&property->value); /* does an unref */
  g_slice_free (Property, property);
}

static gint
lookup_property (gconstpointer a,
                 gconstpointer property_name)
{
  Property *property = (void *) a;

  return strcmp (property->name, property_name);
}

GValue *
gegl_operation_context_get_value (GeglOperationContext *self,
                                  const gchar          *property_name)
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
gegl_operation_context_remove_property (GeglOperationContext *self,
                                        const gchar     *property_name)
{
  Property *property = NULL;

  GSList *found;
  found = g_slist_find_custom (self->property, property_name, lookup_property);
  if (found)
    property = found->data;

  if (!property)
    {
      g_warning ("didn't find property %s for %s", property_name,
                 GEGL_OPERATION_GET_CLASS (self->operation)->name);
      return;
    }
  self->property = g_slist_remove (self->property, property);
  property_destroy (property);
}

static GValue *
gegl_operation_context_add_value (GeglOperationContext *self,
                                  const gchar          *property_name,
                                  GType                 proptype)
{
  Property *property = NULL;
  GSList   *found;
  
  found = g_slist_find_custom (self->property, property_name, lookup_property);

  if (found)
    {
      property = found->data;
    }

  if (property)
    {
      /* XXX: check that the existing one was of the right type */
      g_value_reset (&property->value);
      return &property->value;
    }

  property = property_new (property_name);

  self->property = g_slist_prepend (self->property, property);
  g_value_init (&property->value, proptype);

  return &property->value;
}

GeglOperationContext *gegl_operation_context_new (void)
{
  GeglOperationContext *self = g_slice_new0 (GeglOperationContext);
  return self;
}

void gegl_operation_context_destroy (GeglOperationContext *self)
{

  while (self->property)
    {
      Property *property = self->property->data;
      self->property = g_slist_remove (self->property, property);
      property_destroy (property);
    }
  g_slice_free (GeglOperationContext, self);
}

void
gegl_operation_context_set_object (GeglOperationContext *context,
                                   const gchar          *padname,
                                   GObject              *data)
{
  /* Make it simple, just add an extra ref and then take the object */
  if (data)
    g_object_ref (data);
  gegl_operation_context_take_object (context, padname, data);
}

void
gegl_operation_context_take_object (GeglOperationContext *context,
                                    const gchar          *padname,
                                    GObject              *data)
{
  GParamSpec *pspec;

  /* FIXME: check that there isn't already an existing 
   *        output object/value set?
   */

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (context->operation)), padname);

  if (pspec)
    {
      GValue value = {0, };
      g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));

      g_value_take_object (&value, data);
      gegl_operation_context_set_property (context, padname, &value);

      g_value_unset (&value);
    }
  else
    {
      g_warning ("%s: No paramspec found for pad '%s' on \"%s\"\n",
                 G_STRFUNC,
                 padname,
                 gegl_operation_get_name (context->operation));
    }
}

GObject *
gegl_operation_context_get_object (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GeglOperation *operation;
  GObject       *ret;
  GParamSpec    *pspec;
  GValue         value = { 0, };

  operation = context->operation;

  pspec = g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (context->operation)), padname);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  gegl_operation_context_get_property (context, padname, &value);
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
gegl_operation_context_get_source (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GeglOperation  *operation;
  GeglBuffer     *real_input;
  GeglBuffer     *input;
 
  operation = context->operation;

  real_input = GEGL_BUFFER (gegl_operation_context_get_object (context, padname));
  if (!real_input)
    return NULL;
  input = g_object_ref (real_input);
  return input;
}

static GeglBuffer *emptybuf (void)
{
  static GeglBuffer *empty = NULL; /* we leak this single empty buffer,
                                      avoiding having to create it weighs
                                      up the penalty.
                                    */
  if (!empty)
    {
      GeglRectangle rect={0,0,0,0};
      empty = gegl_buffer_new (&rect, babl_format ("RGBA float"));
    }
  return empty;
}

GeglBuffer *
gegl_operation_context_get_target (GeglOperationContext *context,
                                   const gchar          *padname)
{
  GeglBuffer          *output;
  const GeglRectangle *result;
  const Babl          *format;
  GeglNode            *node;
  GeglOperation       *operation;

#if 0
  g_return_val_if_fail (GEGL_IS_OPERATION_CONTEXT (context), NULL);
#endif

  operation = context->operation;
  node = operation->node; /* <ick */
  format = gegl_operation_get_format (operation, padname);

  if (format == NULL)
    {
      g_warning ("no format for %s presuming RGBA float\n",
                 gegl_node_get_debug_name (node));
      format = babl_format ("RGBA float");
    }
  g_assert (format != NULL);
  g_assert (!strcmp (padname, "output"));

  result = &context->result_rect;

  if (result->width == 0 ||
      result->height == 0)
    {
      output = g_object_ref (emptybuf());
    }
  else if (node->dont_cache == FALSE &&
      ! GEGL_OPERATION_CLASS (G_OBJECT_GET_CLASS (operation))->no_cache)
    {
      GeglBuffer    *cache;
      cache = GEGL_BUFFER (gegl_node_get_cache (node));

      /* Only use the cache if the result is within the cache
       * extent. This is certainly not optimal. My gut feeling is that
       * the current caching mechanism needs to be redesigned
       */
      if (gegl_rectangle_contains (gegl_buffer_get_extent (cache), result))
        {
          output = g_object_ref (cache);
        }
      else
        {
          output = gegl_buffer_new_ram (result, format);
        }
    }
  else
    {
      output = gegl_buffer_new_ram (result, format);
    }

  gegl_operation_context_take_object (context, padname, G_OBJECT (output));
  return output;
}


