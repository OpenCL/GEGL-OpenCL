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

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-operation-meta.h"
#include "graph/gegl-node.h"
#include <string.h>


enum
{
  PROP_0,
  PROP_LAST
};

static void       get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec);
static void       set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec);
static void       finalize (GObject *self_object);
static GeglNode * detect (GeglOperation *operation,
                          gint            x,
                          gint            y);


G_DEFINE_TYPE (GeglOperationMeta, gegl_operation_meta, GEGL_TYPE_OPERATION)

static void
gegl_operation_meta_class_init (GeglOperationMetaClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property           = set_property;
  object_class->get_property           = get_property;
  object_class->finalize               = finalize;
  GEGL_OPERATION_CLASS (klass)->detect = detect;
}

static void
gegl_operation_meta_init (GeglOperationMeta *self)
{
  self->redirects = NULL;
}


static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
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

static Redirect *redirect_new (const gchar *name,
                               GeglNode    *internal,
                               const gchar *internal_name)
{
  Redirect *self = g_slice_new (Redirect);

  self->name          = g_strdup (name);
  self->internal      = internal;
  self->internal_name = g_strdup (internal_name);
  return self;
}

static void redirect_destroy (Redirect *self)
{
  if (!self)
    return;
  if (self->name)
    g_free (self->name);
  if (self->internal_name)
    g_free (self->internal_name);
  g_slice_free (Redirect, self);
}

/* FIXME: take GeglNode's as parameters, since we need
 * extra behavior provided by GeglNode on top of GObject.
 */
static void
gegl_node_copy_property_property (GObject     *source,
                                  const gchar *source_property,
                                  GObject     *destination,
                                  const gchar *destination_property)
{
  GValue      value = { 0 };
  GParamSpec *spec  = g_object_class_find_property (
    G_OBJECT_GET_CLASS (source), source_property);

  g_assert (spec);
  g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (spec));
  gegl_node_get_property (GEGL_OPERATION (source)->node, source_property, &value);
  gegl_node_set_property (GEGL_OPERATION (destination)->node, destination_property, &value);
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
  gegl_node_copy_property_property (G_OBJECT (operation), name, G_OBJECT (internal->operation), internal_name);
}

void gegl_operation_meta_property_changed (GeglOperationMeta *self,
                                           GParamSpec        *arg1,
                                           gpointer           user_data)
{
  g_assert (GEGL_IS_OPERATION_META (self));
  if (arg1)
    {
      GSList *iter = self->redirects;
      while (iter)
        {
          Redirect *redirect = iter->data;
          if (!strcmp (redirect->name, arg1->name))
            {
              gegl_node_copy_property_property (G_OBJECT (self), arg1->name,
                                                G_OBJECT (redirect->internal->operation),
                                                redirect->internal_name);
            }
          iter = iter->next;
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

