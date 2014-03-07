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
 * Copyright 2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-meta.h"

static void       finalize     (GObject       *self_object);

static GeglNode * detect       (GeglOperation *operation,
                                gint           x,
                                gint           y);

static void       constructed  (GObject       *object);

G_DEFINE_TYPE (GeglOperationMeta, gegl_operation_meta, GEGL_TYPE_OPERATION)


static void
gegl_operation_meta_class_init (GeglOperationMetaClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize               = finalize;
  object_class->constructed            = constructed;
  GEGL_OPERATION_CLASS (klass)->detect = detect;
}

static void
gegl_operation_meta_init (GeglOperationMeta *self)
{
  self->redirects = NULL;
}

static void
constructed (GObject *object)
{
  G_OBJECT_CLASS (gegl_operation_meta_parent_class)->constructed (object);

  g_signal_connect (object, "notify", G_CALLBACK (gegl_operation_meta_property_changed), NULL);
}

static GeglNode *
detect (GeglOperation *operation,
        gint           x,
        gint           y)
{
  return NULL; /* hands it over request to the internal nodes */
}

typedef struct Redirect
{
  gchar    *name;
  GeglNode *internal;
  gchar    *internal_name;
} Redirect;

static gchar *
canonicalize_identifier (const gchar *identifier)
{
  gchar *canonicalized = NULL;

  if (identifier)
    {
      gchar *p;

      canonicalized = g_strdup (identifier);

      for (p = canonicalized; *p != 0; p++)
        {
          gchar c = *p;

          if (c != '-' &&
              (c < '0' || c > '9') &&
              (c < 'A' || c > 'Z') &&
              (c < 'a' || c > 'z'))
            *p = '-';
        }
    }

  return canonicalized;
}

static Redirect *
redirect_new (const gchar *name,
              GeglNode    *internal,
              const gchar *internal_name)
{
  Redirect *self = g_slice_new (Redirect);

  self->name          = canonicalize_identifier (name);
  self->internal      = internal;
  self->internal_name = canonicalize_identifier (internal_name);

  return self;
}

static void
redirect_destroy (Redirect *self)
{
  if (!self)
    return;
  if (self->name)
    g_free (self->name);
  if (self->internal_name)
    g_free (self->internal_name);
  g_slice_free (Redirect, self);
}

static void
gegl_node_copy_property_property (GeglOperation *source,
                                  const gchar   *source_property,
                                  GeglOperation *destination,
                                  const gchar   *destination_property)
{
  GValue      value = { 0 };
  GParamSpec *spec  = g_object_class_find_property (G_OBJECT_GET_CLASS (source),
                                                    source_property);

  g_assert (spec);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (spec));
  gegl_node_get_property (source->node, source_property, &value);
  gegl_node_set_property (destination->node, destination_property, &value);
  g_value_unset (&value);
}

void
gegl_operation_meta_redirect (GeglOperation *operation,
                              const gchar   *name,
                              GeglNode      *internal,
                              const gchar   *internal_name)
{
  GeglOperationMeta *self     = GEGL_OPERATION_META (operation);
  Redirect          *redirect = redirect_new (name, internal, internal_name);

  self->redirects = g_slist_prepend (self->redirects, redirect);

  /* set default value */
  gegl_node_copy_property_property (operation,
                                    redirect->name,
                                    gegl_node_get_gegl_operation (internal),
                                    redirect->internal_name);
}

typedef struct
{
  GeglNode *parent;
  GeglNode *child;
} GeglOperationMetaNodeCleanupContext;

static void
_gegl_operation_meta_node_cleanup (gpointer data,
                                   GObject *where_the_object_was)
{
  GeglOperationMetaNodeCleanupContext *ctx = data;

  if (ctx->child)
    {
      g_object_remove_weak_pointer (G_OBJECT (ctx->child), (void**)&ctx->child);
      gegl_node_remove_child (ctx->parent, ctx->child);
    }

  g_free (data);
}

void
gegl_operation_meta_watch_node (GeglOperation     *operation,
                                GeglNode          *node)
{
  GeglOperationMetaNodeCleanupContext *ctx;
  ctx = g_new0 (GeglOperationMetaNodeCleanupContext, 1);

  ctx->parent = operation->node;
  ctx->child  = node;

  g_object_add_weak_pointer (G_OBJECT (ctx->child), (void**)&ctx->child);
  g_object_weak_ref (G_OBJECT (operation), _gegl_operation_meta_node_cleanup, ctx);
}

void
gegl_operation_meta_watch_nodes (GeglOperation     *operation,
                                 GeglNode          *node,
                                 ...)
{
  va_list var_args;

  va_start (var_args, node);
  while (node)
    {
      gegl_operation_meta_watch_node (operation, node);
      node = va_arg (var_args, GeglNode *);
    }
  va_end (var_args);
}

void
gegl_operation_meta_property_changed (GeglOperationMeta *self,
                                      GParamSpec        *pspec,
                                      gpointer           user_data)
{
  GSList *iter;

  g_return_if_fail (GEGL_IS_OPERATION_META (self));
  g_return_if_fail (pspec);

  for (iter = self->redirects; iter; iter = iter->next)
    {
      Redirect *redirect = iter->data;

      if (!strcmp (redirect->name, pspec->name))
        {
          gegl_node_copy_property_property (GEGL_OPERATION (self), pspec->name,
                                            gegl_node_get_gegl_operation (redirect->internal),
                                            redirect->internal_name);
        }
    }
}

static void
finalize (GObject *gobject)
{
  GeglOperationMeta *self = GEGL_OPERATION_META (gobject);
  GSList            *iter;

  for (iter = self->redirects; iter; iter = iter->next)
    redirect_destroy (iter->data);

  g_slist_free (self->redirects);

  G_OBJECT_CLASS (gegl_operation_meta_parent_class)->finalize (gobject);
}
