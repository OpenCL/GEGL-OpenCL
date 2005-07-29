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

#include "config.h"

#include <glib-object.h>

#include "gegl-types.h"
#include "gegl-operation.h"
#include "gegl-node.h"
#include "gegl-pad.h"


enum
{
  PROP_0,
};

static void      gegl_operation_class_init (GeglOperationClass    *klass);
static void      gegl_operation_init       (GeglOperation         *self);
static GObject * constructor               (GType                  type,
                                            guint                  n_props,
                                            GObjectConstructParam *props);
static void      finalize                  (GObject               *gobject);
static void      set_property              (GObject               *gobject,
                                            guint                  prop_id,
                                            const GValue          *value,
                                            GParamSpec            *pspec);
static void      get_property              (GObject               *gobject,
                                            guint                  prop_id,
                                            GValue                *value,
                                            GParamSpec            *pspec);
static void      associate                 (GeglOperation         *self);

G_DEFINE_TYPE (GeglOperation, gegl_operation, GEGL_TYPE_OBJECT)


static void
gegl_operation_class_init (GeglOperationClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor  = constructor;
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  
  klass->associate = associate;
}

static void
gegl_operation_init (GeglOperation *self)
{
  self->name = NULL;
}

static void
finalize (GObject *gobject)
{
  GeglOperation * self = GEGL_OPERATION (gobject);

  if (self->name)
    g_free (self->name);

  G_OBJECT_CLASS (gegl_operation_parent_class)->finalize (gobject);
}

static GObject*
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject    *object;
  GeglOperation *self;

  object = G_OBJECT_CLASS (gegl_operation_parent_class)->constructor (type,
                                                                   n_props,
                                                                   props);

  self = GEGL_OPERATION (object);

  self->constructed = TRUE;

  return object;
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglOperation *object = GEGL_OPERATION (gobject);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglOperation *object = GEGL_OPERATION (gobject);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
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
      g_warning ("gegl_operation_create_pad aborting, no associated node, this"
                 " should be called after the operation is associated with a specific node.");
      return;
    }

  pad = g_object_new (GEGL_TYPE_PAD, NULL);
  gegl_pad_set_param_spec (pad, param_spec);
  gegl_pad_set_node (pad, self->node);
  gegl_node_add_pad (self->node, pad);
} 

gboolean
gegl_operation_evaluate (GeglOperation *self,
                         const gchar   *output_pad)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (self);

  return klass->evaluate (self, output_pad);
}

#include <stdio.h>

static void
associate (GeglOperation *self)
{
  fprintf (stderr, "kilroy was here (%p)\n", self);
  return;
}

gboolean
gegl_operation_associate (GeglOperation *self,
                          GeglNode      *node)
{
  GeglOperationClass *klass;

  g_return_val_if_fail (GEGL_IS_OPERATION (self), FALSE);
  g_return_val_if_fail (GEGL_IS_NODE (node), FALSE);

  klass = GEGL_OPERATION_GET_CLASS (self);

  g_assert (klass->associate);
  gegl_operation_set_node (self, node);
  klass->associate (self);
}

GeglNode *
gegl_operation_get_node (GeglOperation *self)
{
  return self->node;
}
void
gegl_operation_set_node (GeglOperation *self,
                         GeglNode      *node)
{
  if (self->node)
    {
      g_object_unref (node);
      self->node = NULL;
    }
  if (node)
    {
      g_object_ref (node);
      self->node = node;
    }
}
