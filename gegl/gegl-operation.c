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
 *           2005, 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-connection.h"
#include "gegl-pad.h"
#include "gegl-utils.h"
#include "gegl-graph.h"

static void          gegl_operation_class_init (GeglOperationClass    *klass);
static void          gegl_operation_init       (GeglOperation         *self);
static void          attach                    (GeglOperation         *self);

static GeglRectangle get_defined_region        (GeglOperation *self);
static GeglRectangle get_affected_region       (GeglOperation *self,
                                                const gchar   *input_pad,
                                                GeglRectangle  region);
static gboolean calc_source_regions            (GeglOperation *self,
                                                gpointer       context_id);

G_DEFINE_TYPE (GeglOperation, gegl_operation, G_TYPE_OBJECT)

static void
gegl_operation_class_init (GeglOperationClass * klass)
{
  klass->name = NULL;  /* an operation class with name == NULL is not included
                          when doing operation lookup by name */
  klass->description = NULL;
  klass->categories = NULL;
  klass->attach = attach;
  klass->prepare = NULL;
  klass->get_defined_region = get_defined_region;
  klass->get_affected_region = get_affected_region;
  klass->calc_source_regions = calc_source_regions;
}

static void
gegl_operation_init (GeglOperation *self)
{
}

/**
 * gegl_operation_create_pad:
 * @self: a #GeglOperation.
 * @param_spec:
 *
 * Create a property.
 **/
void
gegl_operation_create_pad (GeglOperation *self,
                           GParamSpec    *param_spec)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (param_spec);

  if (!self->node)
    {
      g_warning ("gegl_operation_create_pad aborting, no associated node. "
                 "This method should only be called after the operation is "
                 "associated with a node.");
      return;
    }

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self->node);
  gegl_node_add_pad (self->node, pad);
}

gboolean
gegl_operation_process (GeglOperation *self,
                        gpointer       context_id,
                        const gchar   *output_pad)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (self);

  return klass->process (self, context_id, output_pad);
}

GeglRectangle
gegl_operation_get_defined_region (GeglOperation *self)
{
  GeglRectangle rect = {0,0,0,0};
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);
  if (klass->get_defined_region)
    return klass->get_defined_region (self);
  return rect;
}

GeglRectangle
gegl_operation_get_affected_region (GeglOperation *self,
                                    const gchar   *input_pad,
                                    GeglRectangle  region)
{
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);
  if (region.width   == 0 ||
      region.height  == 0)
    return region;
  if (klass->get_affected_region)
    return klass->get_affected_region (self, input_pad, region);
  return region;
}


gboolean
gegl_operation_calc_source_regions (GeglOperation *self,
                                    gpointer       context_id)
{
  GeglOperationClass *klass;

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->calc_source_regions)
    return klass->calc_source_regions (self, context_id);
  return FALSE;
}

static void
attach (GeglOperation *self)
{
  g_warning ("kilroy was at What The Hack (%p, %s)\n", (void*)self,
                         G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return;
}

void
gegl_operation_attach (GeglOperation *self,
                       GeglNode      *node)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (GEGL_IS_NODE (node));

  klass = GEGL_OPERATION_GET_CLASS (self);

  g_assert (klass->attach);
  self->node = node;
  klass->attach (self);
}

void
gegl_operation_prepare (GeglOperation *self,
                        gpointer       context_id)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self, context_id);
}

GeglNode * gegl_operation_get_source_node (GeglOperation *operation,
                                           const gchar   *input_pad_name)
{
  GeglPad *pad;
  g_assert (operation &&
            operation->node &&
            input_pad_name);
  pad = gegl_node_get_pad (operation->node, input_pad_name);

  if (!pad)
    return NULL;

  pad = gegl_pad_get_internal_connected_to (pad);

  if (!pad)
    return NULL;

  g_assert (gegl_pad_get_node (pad));
  return gegl_pad_get_node (pad);
}

GeglRectangle *
gegl_operation_source_get_defined_region (GeglOperation *operation,
                                          const gchar   *input_pad_name)
{
  GeglNode *node = gegl_operation_get_source_node (operation, input_pad_name);
  if (node)
    return &node->have_rect;
  return NULL;
}

void
gegl_operation_set_source_region (GeglOperation *operation,
                                  gpointer       context_id,
                                  const gchar   *input_pad_name,
                                  GeglRectangle *region)
{
  GeglNode      *child;
  GeglRectangle  child_need;

  g_assert (operation);
  g_assert (operation->node);
  g_assert (input_pad_name);

  {
    GeglPad *pad = gegl_node_get_pad (operation->node, input_pad_name);
    if (!pad)
      return;
    pad = gegl_pad_get_internal_connected_to (pad);
    if (!pad)
      return;
    child = gegl_pad_get_node (pad);
    if (!child)
      return;
  }

  {
    GeglNodeDynamic *dynamic = gegl_node_get_dynamic (operation->node, context_id);
    gegl_rectangle_bounding_box (&child_need, &dynamic->need_rect, region);
  }

  /* expand the need rect of the node, to include what the calling
   * operation needs as well
   */
  gegl_node_set_need_rect (child, context_id,
                           child_need.x, child_need.y,
                           child_need.width, child_need.height);
}

static GeglRectangle
get_defined_region (GeglOperation *self)
{
  GeglRectangle rect = {0,0,0,0};
  if (self->node->is_graph)
    {
      return gegl_operation_get_defined_region (
                   gegl_node_get_output_proxy (self->node, "output")->operation);
    }
  g_warning ("Op '%s' has no defined_region method",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return rect;
}

static GeglRectangle
get_affected_region (GeglOperation *self,
                     const gchar   *input_pad,
                     GeglRectangle  region)
{
  if (self->node->is_graph)
    {
      return gegl_operation_get_affected_region (
                   gegl_node_get_output_proxy (self->node, "output")->operation,
                   input_pad,
                   region);
    }
  return region;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       context_id)
{
  if (self->node->is_graph)
    {
      return gegl_operation_calc_source_regions (
                         gegl_node_get_output_proxy (self->node, "output")->operation,
                         context_id);
    }

  g_warning ("Op '%s' has no calc_source_regions method",
                         G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return FALSE;
}

GeglRectangle *
gegl_operation_get_requested_region     (GeglOperation *operation,
                                         gpointer       context_id)
{
  GeglNodeDynamic *dynamic = gegl_node_get_dynamic (operation->node, context_id);
  g_assert (operation);
  g_assert (operation->node);
  return &dynamic->need_rect;
}

GeglRectangle *
gegl_operation_result_rect (GeglOperation *operation,
                            gpointer       context_id)
{
  GeglNodeDynamic *dynamic = gegl_node_get_dynamic (operation->node, context_id);
  g_assert (operation);
  g_assert (operation->node);
  return &dynamic->result_rect;
}

void
gegl_operation_class_set_name (GeglOperationClass *klass,
                               const gchar        *new_name)
{
  gchar *name_copy;

  name_copy = g_strdup (new_name);
  g_strdelimit (name_copy, "_", '-');
  klass->name = name_copy;
}

static void
add_operations (GHashTable *hash,
                GType       parent)
{
  GType       *types;
  guint        count;
  gint         no;

  types = g_type_children (parent, &count);
  if (!types)
    return;

  for (no=0; no < count; no++)
    {
      GeglOperationClass *operation_class = g_type_class_ref (types[no]);
      if (operation_class->name)
        {
          g_hash_table_insert (hash, g_strdup (operation_class->name), (gpointer)types[no]);
        }
      add_operations (hash, types[no]);
    }
  g_free (types);
}

static GHashTable *gtype_hash = NULL;
GType
gegl_operation_gtype_from_name (const gchar *name)
{
  if (!gtype_hash)
    {
      gtype_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);

      add_operations (gtype_hash, GEGL_TYPE_OPERATION);
    }
  return (GType)g_hash_table_lookup (gtype_hash, name);
}

static GSList *operations_list = NULL;
static void addop(gpointer key,
                  gpointer value,
                  gpointer user_data)
{
  operations_list = g_slist_prepend (operations_list, key);
}

gchar **gegl_list_operations (guint *n_operations_p)
{
  gchar **pasp = NULL;
  gint    n_operations;
  gint    i;
  GSList *iter;
  gint    pasp_size=0;
  gint    pasp_pos;

  if (!operations_list)
    {
      gegl_operation_gtype_from_name ("");
      g_hash_table_foreach (gtype_hash, addop, NULL);
      operations_list = g_slist_sort (operations_list, (GCompareFunc) strcmp);
    }

  n_operations = g_slist_length (operations_list);
  pasp_size += (n_operations+1) * sizeof (gchar*);
  for (iter = operations_list; iter!=NULL; iter= g_slist_next (iter))
    {
      const gchar *name = iter->data;
      pasp_size += strlen (name) + 1;
    }
  pasp = g_malloc (pasp_size);
  pasp_pos = (n_operations+1) * sizeof (gchar*);
  for (iter = operations_list, i=0; iter!=NULL; iter= g_slist_next (iter), i++)
    {
      const gchar *name = iter->data;
      pasp[i] = ((gchar*)pasp) + pasp_pos;
      strcpy (pasp[i], name);
      pasp_pos += strlen (name) + 1;
    }
  pasp[i] = NULL;
  if (n_operations_p)
    *n_operations_p = n_operations;
  return pasp;
}

/* returns a freshly allocated list of the properties of the object, does not list
 * the regular gobject properties of GeglNode ('name' and 'operation') */
GParamSpec**
gegl_list_properties (const gchar *operation_type,
                      guint       *n_properties_p)
{
  GParamSpec  **pspecs;
  GType         type; 
  GObjectClass *klass;

  type = gegl_operation_gtype_from_name (operation_type);
  if (!type)
    {
      if (n_properties_p)
        *n_properties_p=0;
      return NULL;
    }
  klass = g_type_class_ref (type);
  pspecs = g_object_class_list_properties (klass, n_properties_p);
  g_type_class_unref (klass);
  return pspecs;
}

GObject *
gegl_operation_get_data (GeglOperation *operation,
                         gpointer       context_id,
                         const gchar   *property_name)
{
  GObject    *ret;
  GeglNode   *node = operation->node;
  GParamSpec *pspec;
  GValue      value = {0,};
  GeglNodeDynamic *dynamic = gegl_node_get_dynamic (node, context_id);

  pspec = gegl_node_find_property (node, property_name);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  gegl_node_dynamic_get_property (dynamic, property_name, &value);
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

GeglNode *
gegl_operation_detect (GeglOperation *operation,
                       gint           x,
                       gint           y)
{
  GeglNode *node = NULL;
  GeglOperationClass *klass;

  if (!operation)
    return NULL;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), NULL);
  node = operation->node;

  klass = GEGL_OPERATION_GET_CLASS (operation);

  if (klass->detect)
    {
      return klass->detect (operation, x, y);
    }

  if (x>=node->have_rect.x &&
      x<node->have_rect.x+node->have_rect.width   &&
      y>=node->have_rect.y &&
      y<node->have_rect.y+node->have_rect.height)
    {
      return node;
    }
  return NULL;
}

void
gegl_operation_set_data (GeglOperation *operation,
                         gpointer       context_id,
                         const gchar   *property_name,
                         GObject       *data)
{
  GeglNode   *node = operation->node;
  GParamSpec *pspec;
  GValue      value = {0,};
  GeglNodeDynamic *dynamic = gegl_node_get_dynamic (node, context_id);

  pspec = gegl_node_find_property (node, property_name);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
  g_value_set_object (&value, data);
  gegl_node_dynamic_set_property (dynamic, property_name, &value);
  g_value_unset (&value);
  g_object_unref (data);  /* stealing the initial reference? */
}
