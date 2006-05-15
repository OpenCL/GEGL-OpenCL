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
 */

#include "config.h"
#include <glib-object.h>
#include "gegl-types.h"
#include "gegl-object.h"


enum
{
  PROP_0,
  PROP_NAME
};

static void      gegl_object_class_init (GeglObjectClass       *klass);
static void      gegl_object_init       (GeglObject            *self);
static GObject * constructor            (GType                  type,
                                         guint                  n_props,
                                         GObjectConstructParam *props);
static void      finalize               (GObject               *gobject);
static void      set_property           (GObject               *gobject,
                                         guint                  prop_id,
                                         const GValue          *value,
                                         GParamSpec            *pspec);
static void      get_property           (GObject               *gobject,
                                         guint                  prop_id,
                                         GValue                *value,
                                         GParamSpec            *pspec);


G_DEFINE_TYPE (GeglObject, gegl_object, G_TYPE_OBJECT)


static void
gegl_object_class_init (GeglObjectClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor  = constructor;
  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_NAME,
                                   g_param_spec_string ("name",
                                                        "Name",
                                                        "The GeglObject's name",
                                                        "",
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE));
}

static void
gegl_object_init (GeglObject *self)
{
  self->name = NULL;
}

static void
finalize (GObject *gobject)
{
  GeglObject * self = GEGL_OBJECT (gobject);

  if (self->name)
    g_free (self->name);

  G_OBJECT_CLASS (gegl_object_parent_class)->finalize (gobject);
}

static GObject*
constructor (GType                  type,
             guint                  n_props,
             GObjectConstructParam *props)
{
  GObject    *gobject;
  GeglObject *self;

  gobject = G_OBJECT_CLASS (gegl_object_parent_class)->constructor (type,
                                                                   n_props,
                                                                   props);
  self = GEGL_OBJECT (gobject);
  self->constructed = TRUE;

  return gobject;
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglObject *self = GEGL_OBJECT (gobject);

  switch (property_id)
    {
    case PROP_NAME:
      gegl_object_set_name (self, g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

static void
get_property (GObject      *gobject,
              guint         property_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglObject *self = GEGL_OBJECT (gobject);

  switch (property_id)
    {
    case PROP_NAME:
      g_value_set_string (value, gegl_object_get_name (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
      break;
    }
}

/**
 * gegl_object_set_name:
 * @object: a #GeglObject.
 * @name: a string
 *
 * Sets the name for this object.
 **/
void
gegl_object_set_name (GeglObject  *self,
                      const gchar *name)
{
  g_return_if_fail (GEGL_IS_OBJECT (self));

  if (self->name)
    g_free (self->name);

  self->name = g_strdup (name);

  g_object_notify (G_OBJECT (self), "name");
}

/**
 * gegl_object_get_name:
 * @object: a #GeglObject.
 *
 * Gets the name for this object.
 *
 * Returns: a string for the name of this object.
 **/
const gchar*
gegl_object_get_name (GeglObject *self)
{
  g_return_val_if_fail (GEGL_IS_OBJECT (self), NULL);

  return self->name;
}
