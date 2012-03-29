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
 * Copyright 2003      Calvin Williamson
 *           2005-2008 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation.h"
#include "gegl-utils.h"
#include "graph/gegl-node.h"
#include "graph/gegl-connection.h"
#include "graph/gegl-pad.h"
#include "buffer/gegl-region.h"
#include "buffer/gegl-buffer.h"
#include "gegl-operations.h"

static void         attach                    (GeglOperation       *self);

static GeglRectangle get_bounding_box          (GeglOperation       *self);
static GeglRectangle get_invalidated_by_change (GeglOperation       *self,
                                                const gchar         *input_pad,
                                                const GeglRectangle *input_region);
static GeglRectangle get_required_for_output   (GeglOperation       *self,
                                                const gchar         *input_pad,
                                                const GeglRectangle *region);

static void gegl_operation_class_init     (GeglOperationClass *klass);
static void gegl_operation_base_init      (GeglOperationClass *klass);
static void gegl_operation_init           (GeglOperation      *self);

GType
gegl_operation_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GeglOperationClass),
        (GBaseInitFunc)      gegl_operation_base_init,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc)     gegl_operation_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (GeglOperation),
        0,              /* n_preallocs */
        (GInstanceInitFunc) gegl_operation_init,
      };

      type = g_type_register_static (G_TYPE_OBJECT,
                                     "GeglOperation",
                                     &info, 0);
    }
  return type;
}


static void
gegl_operation_class_init (GeglOperationClass *klass)
{
  klass->name                      = NULL;  /* an operation class with
                                             * name == NULL is not
                                             * included when doing
                                             * operation lookup by
                                             * name
                                             */
  klass->compat_name               = NULL;
  klass->attach                    = attach;
  klass->prepare                   = NULL;
  klass->no_cache                  = FALSE;
  klass->opencl_support            = FALSE;
  klass->get_bounding_box          = get_bounding_box;
  klass->get_invalidated_by_change = get_invalidated_by_change;
  klass->get_required_for_output   = get_required_for_output;
}

static void
gegl_operation_base_init  (GeglOperationClass *klass)
{
  /* XXX: leaked for now, should replace G_DEFINE_TYPE with the expanded one */
  klass->keys = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
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
  g_return_if_fail (param_spec != NULL);

  if (!self->node)
    {
      g_warning ("%s: aborting, no associated node. "
                 "This method should only be called after the operation is "
                 "associated with a node.", G_STRFUNC);
      return;
    }

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self->node);
  gegl_node_add_pad (self->node, pad);
}

void gegl_node_emit_computed (GeglNode *node, const GeglRectangle *rect);


gboolean
gegl_operation_process (GeglOperation        *operation,
                        GeglOperationContext *context,
                        const gchar          *output_pad,
                        const GeglRectangle  *result,
                        gint                  level)
{
  GeglOperationClass  *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (operation);

  if (!strcmp (output_pad, "output") &&
      (result->width == 0 || result->height == 0))
    {
      GeglBuffer *output = gegl_buffer_new (NULL, NULL);
      g_warning ("%s Eeek: processing 0px rectangle", G_STRLOC);
      /* when this case is hit.. we've done something bad.. */
      gegl_operation_context_take_object (context, "output", G_OBJECT (output));
      return TRUE;
    }

  return klass->process (operation, context, output_pad, result, level);
}

/* Calls an extending class' get_bound_box method if defined otherwise
 * just returns a zero-initiliased bouding box
 */
GeglRectangle
gegl_operation_get_bounding_box (GeglOperation *self)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (self);
  GeglRectangle       rect  = { 0, 0, 0, 0 };

  if (klass->get_bounding_box)
    return klass->get_bounding_box (self);

  return rect;
}

GeglRectangle
gegl_operation_get_invalidated_by_change (GeglOperation        *self,
                                          const gchar         *input_pad,
                                          const GeglRectangle *input_region)
{
  GeglOperationClass *klass;
  GeglRectangle       retval = { 0, };

  g_return_val_if_fail (GEGL_IS_OPERATION (self), retval);
  g_return_val_if_fail (input_pad != NULL, retval);
  g_return_val_if_fail (input_region != NULL, retval);

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (input_region->width  == 0 ||
      input_region->height == 0)
    return *input_region;

  if (klass->get_invalidated_by_change)
    return klass->get_invalidated_by_change (self, input_pad, input_region);

  return *input_region;
}

static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  if (operation->node->is_graph)
    {
      return gegl_operation_get_required_for_output (operation, input_pad, roi);
    }

  return *roi;
}

GeglRectangle
gegl_operation_get_required_for_output (GeglOperation        *operation,
                                        const gchar         *input_pad,
                                        const GeglRectangle *roi)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);

  if (roi->width == 0 ||
      roi->height == 0)
    return *roi;

  g_assert (klass->get_required_for_output);

  return klass->get_required_for_output (operation, input_pad, roi);
}



GeglRectangle
gegl_operation_get_cached_region (GeglOperation        *operation,
                                  const GeglRectangle *roi)
{
  GeglOperationClass *klass = GEGL_OPERATION_GET_CLASS (operation);

  if (!klass->get_cached_region)
    {
      return *roi;
    }

  return klass->get_cached_region (operation, roi);
}

static void
attach (GeglOperation *self)
{
  g_warning ("kilroy was at What The Hack (%p, %s)\n", (void *) self,
             G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
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

/* Calls the prepare function on the operation that extends this base class */
void
gegl_operation_prepare (GeglOperation *self)
{
  GeglOperationClass *klass;

  g_return_if_fail (GEGL_IS_OPERATION (self));

  klass = GEGL_OPERATION_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self);
}

GeglNode *
gegl_operation_get_source_node (GeglOperation *operation,
                                const gchar   *input_pad_name)
{
  GeglPad *pad;

  g_assert (operation &&
            operation->node &&
            input_pad_name);
  pad = gegl_node_get_pad (operation->node, input_pad_name);

  if (!pad)
    return NULL;

  pad = gegl_pad_get_connected_to (pad);

  if (!pad)
    return NULL;

  g_assert (gegl_pad_get_node (pad));

  return gegl_pad_get_node (pad);
}

GeglRectangle *
gegl_operation_source_get_bounding_box (GeglOperation  *operation,
                                        const gchar   *input_pad_name)
{
  GeglNode *node = gegl_operation_get_source_node (operation, input_pad_name);

  if (node)
    {
      GeglRectangle *ret;
      g_mutex_lock (node->mutex);
      ret = &node->have_rect;
      g_mutex_unlock (node->mutex);
      return ret;
    }

  return NULL;
}

static GeglRectangle
get_bounding_box (GeglOperation *self)
{
  GeglRectangle rect = { 0, 0, 0, 0 };

  if (self->node->is_graph)
    {
      GeglOperation *operation;

      operation = gegl_node_get_output_proxy (self->node, "output")->operation;
      rect      = gegl_operation_get_bounding_box (operation);
    }
  else
    {
      g_warning ("Operation '%s' has no get_bounding_box() method",
                 G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS (self)));
    }

  return rect;
}

static GeglRectangle
get_invalidated_by_change (GeglOperation        *self,
                           const gchar         *input_pad,
                           const GeglRectangle *input_region)
{
#if 0
  /* FIXME: this seems to sometimes go into an infinite loop, the
   * current workaround of passing the rectangle straight through
   * isn't even true for unsharp-mask/drop-shadow/difference of gaussians,
   * but it stops a crasher bug.
   *
   * This needs to be revisited as part of the core processing revisit
   * (perhaps together with a meta-op framework revwrite).
   */
  if (self->node->is_graph)
    {
      return gegl_operation_get_invalidated_by_change (
               gegl_node_get_output_proxy (self->node, "output")->operation,
               input_pad,
               input_region);
    }
#endif
  return *input_region;
}

/* returns a freshly allocated list of the properties of the object, does not list
 * the regular gobject properties of GeglNode ('name' and 'operation') */
GParamSpec **
gegl_operation_list_properties (const gchar *operation_type,
                                guint       *n_properties_p)
{
  GParamSpec  **pspecs;
  GType         type;
  GObjectClass *klass;

  type = gegl_operation_gtype_from_name (operation_type);
  if (!type)
    {
      if (n_properties_p)
        *n_properties_p = 0;
      return NULL;
    }
  klass  = g_type_class_ref (type);
  pspecs = g_object_class_list_properties (klass, n_properties_p);
  g_type_class_unref (klass);
  return pspecs;
}

GeglNode *
gegl_operation_detect (GeglOperation *operation,
                       gint           x,
                       gint           y)
{
  GeglNode           *node = NULL;
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

  if (x >= node->have_rect.x &&
      x < node->have_rect.x + node->have_rect.width &&
      y >= node->have_rect.y &&
      y < node->have_rect.y + node->have_rect.height)
    {
      return node;
    }
  return NULL;
}

void
gegl_operation_set_format (GeglOperation *self,
                           const gchar   *pad_name,
                           const Babl    *format)
{
  GeglPad *pad;

  g_return_if_fail (GEGL_IS_OPERATION (self));
  g_return_if_fail (pad_name != NULL);

  pad = gegl_node_get_pad (self->node, pad_name);

  g_return_if_fail (pad != NULL);

  pad->format = format;
}

const Babl *
gegl_operation_get_format (GeglOperation *self,
                           const gchar   *pad_name)
{
  GeglPad *pad;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), NULL);
  g_return_val_if_fail (pad_name != NULL, NULL);

  pad = gegl_node_get_pad (self->node, pad_name);

  if (pad == NULL || pad->format == NULL)
    {
      g_warning ("%s: returns NULL", G_STRFUNC);
    }
  g_return_val_if_fail (pad != NULL, NULL);

  return pad->format;
}

const gchar *
gegl_operation_get_name (GeglOperation *operation)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (operation), NULL);

  klass = GEGL_OPERATION_GET_CLASS (operation);

  return klass->name;
}

void
gegl_operation_invalidate (GeglOperation       *operation,
                           const GeglRectangle *roi,
                           gboolean             clear_cache)
{
  GeglNode *node = NULL;

  if (!operation)
    return;

  g_return_if_fail (GEGL_IS_OPERATION (operation));
  node = operation->node;

  gegl_node_invalidated (node, roi, TRUE);
}


gchar **
gegl_operation_list_keys (const gchar *operation_name,
                          guint       *n_keys)
{
  GType         type;
  GObjectClass *klass;
  GList        *list, *l;
  gchar       **ret;
  int count;
  int i;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      if (n_keys)
        *n_keys = 0;
      return NULL;
    }
  klass  = g_type_class_ref (type);
  count = g_hash_table_size (GEGL_OPERATION_CLASS (klass)->keys);
  ret = g_malloc0 (sizeof (gpointer) * (count + 1));
  list = g_hash_table_get_keys (GEGL_OPERATION_CLASS (klass)->keys);
  for (i = 0, l = list; l; l = l->next)
    {
      ret[i] = l->data;
    }
  g_list_free (list);
  if (n_keys)
    *n_keys = count;
  g_type_class_unref (klass);
  return ret;
}

void
gegl_operation_class_set_key (GeglOperationClass *klass,
                              const gchar        *key_name,
                              const gchar        *key_value)
{
  if (!key_value)
    {
      g_hash_table_remove (klass->keys, key_name);
      return;
    }
  else
    {
      key_value = g_strdup (key_value);
      g_hash_table_insert (klass->keys, g_strdup (key_name),
                           (void*)key_value);
    }
  if (!strcmp (key_name, "name"))
    {
      klass->name = key_value;
    }
}

void
gegl_operation_class_set_keys (GeglOperationClass *klass,
                               const gchar        *key_name,
                                ...)
{
  va_list var_args;

  va_start (var_args, key_name);
  while (key_name)
    {
      const char *value = va_arg (var_args, char *);

      gegl_operation_class_set_key (klass, key_name, value);

      key_name = va_arg (var_args, char *);
    }
  va_end (var_args);
}

void
gegl_operation_set_key (const gchar *operation_name,
                        const gchar *key_name,
                        const gchar *key_value)
{
  GType         type;
  GObjectClass *klass;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    return;
  klass  = g_type_class_ref (type);
  gegl_operation_class_set_key (GEGL_OPERATION_CLASS (klass), key_name, key_value);
  g_type_class_unref (klass);
}

const gchar *
gegl_operation_class_get_key (GeglOperationClass *operation_class,
                              const gchar        *key_name)
{
  const gchar  *ret = NULL;
  ret = g_hash_table_lookup (GEGL_OPERATION_CLASS (operation_class)->keys, key_name);
  return ret;
}

const gchar *
gegl_operation_get_key (const gchar *operation_name,
                        const gchar *key_name)
{
  GType         type;
  GObjectClass *klass;
  const gchar  *ret = NULL;
  type = gegl_operation_gtype_from_name (operation_name);
  if (!type)
    {
      return NULL;
    }
  klass  = g_type_class_ref (type);
  ret = gegl_operation_class_get_key (GEGL_OPERATION_CLASS (klass), key_name);
  g_type_class_unref (klass);
  return ret;
}
