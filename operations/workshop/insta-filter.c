/* This file is an image processing operation for GEGL
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
 * Copyright 2015, 2016 Red Hat, Inc.
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

enum_start (gegl_insta_filter_preset)
  enum_value (GEGL_INSTA_FILTER_PRESET_NONE, "none", "None")
  enum_value (GEGL_INSTA_FILTER_PRESET_1977, "1977", "1977")
  enum_value (GEGL_INSTA_FILTER_PRESET_BRANNAN, "brannan", "Brannan")
  enum_value (GEGL_INSTA_FILTER_PRESET_GOTHAM, "gotham", "Gotham")
  enum_value (GEGL_INSTA_FILTER_PRESET_NASHVILLE, "nashville", "Nashville")
enum_end (GeglInstaFilterPreset)

property_enum (preset, _("Preset"),
               GeglInstaFilterPreset, gegl_insta_filter_preset,
               GEGL_INSTA_FILTER_PRESET_NONE)
  description (_("Which filter to apply"))

#else

#define GEGL_OP_C_SOURCE insta-filter.c

#include "gegl-plugin.h"

struct _GeglOp
{
  GeglOperationMeta parent_instance;
  gpointer          properties;

  GeglNode *input;
  GeglNode *output;
  GList *nodes;
};

typedef struct
{
  GeglOperationMetaClass parent_class;
} GeglOpClass;

#include "gegl-op.h"
GEGL_DEFINE_DYNAMIC_OPERATION(GEGL_TYPE_OPERATION_META)

static void
do_setup (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *node;
  GList *l;

  g_list_free_full (self->nodes, g_object_unref);
  self->nodes = NULL;

  switch (o->preset)
    {
    case GEGL_INSTA_FILTER_PRESET_NONE:
      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:nop",
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);
      break;

    case GEGL_INSTA_FILTER_PRESET_1977:
      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:insta-curve",
                                  "preset", o->preset,
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);
      break;

    case GEGL_INSTA_FILTER_PRESET_BRANNAN:
      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:insta-curve",
                                  "preset", o->preset,
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);
      break;

    case GEGL_INSTA_FILTER_PRESET_GOTHAM:
      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:insta-curve",
                                  "preset", o->preset,
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);

      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:gray",
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);
      break;

    case GEGL_INSTA_FILTER_PRESET_NASHVILLE:
      node = gegl_node_new_child (operation->node,
                                  "operation", "gegl:insta-curve",
                                  "preset", o->preset,
                                  NULL);
      self->nodes = g_list_prepend (self->nodes, node);
      break;

    default:
      break;
    }

  node = GEGL_NODE (self->nodes->data);
  gegl_node_link (self->input, node);

  for (l = self->nodes; l != NULL && l->next != NULL; l = l->next)
    {
      GeglNode *sink = GEGL_NODE (l->next->data);
      GeglNode *source = GEGL_NODE (l->data);

      gegl_node_link (source, sink);
      gegl_operation_meta_watch_node (operation, source);
    }

  node = GEGL_NODE (l->data);
  gegl_node_link (node, self->output);
  gegl_operation_meta_watch_node (operation, node);
}

static void
attach (GeglOperation *operation)
{
  GeglOp *self = GEGL_OP (operation);

  self->input = gegl_node_get_output_proxy (operation->node, "input");
  self->output = gegl_node_get_output_proxy (operation->node, "output");
  do_setup (operation);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  GeglOp *self = GEGL_OP (operation);
  GeglNode *output = self->output;
  GeglRectangle bounds;

  bounds = gegl_node_get_bounding_box (output);
  if (x >= bounds.x &&
      y >= bounds.y &&
      x  < bounds.x + bounds.width &&
      y  < bounds.y + bounds.height)
    return operation->node;

  return NULL;
}

static void
finalize (GObject *object)
{
  GeglOp *self = GEGL_OP (object);

  g_list_free (self->nodes);

  G_OBJECT_CLASS (gegl_op_parent_class)->finalize (object);
}

static void
my_set_property (GObject      *object,
                 guint         property_id,
                 const GValue *value,
                 GParamSpec   *pspec)
{
  GeglOperation  *operation = GEGL_OPERATION (object);
  GeglOp *self = GEGL_OP (operation);
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglInstaFilterPreset old_preset = o->preset;

  set_property (object, property_id, value, pspec);

  if (self->input != NULL && o->preset != old_preset)
    do_setup (operation);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->finalize = finalize;
  object_class->set_property = my_set_property;

  operation_class->attach = attach;
  operation_class->detect = detect;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:insta-filter",
    "title",       _("Insta Filter"),
    "categories",  "meta:color",
    "description", _("Apply a preset filter to an image"),
    NULL);

}

#endif
