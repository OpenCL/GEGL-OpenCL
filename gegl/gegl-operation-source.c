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
 * Copyright 2006 Øyvind Kolås
 */
#include "gegl-operation-source.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LAST
};

static void     finalize     (GObject      *gobject);
static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean evaluate     (GeglOperation   *operation,
                              const gchar  *output_prop);
static void     associate    (GeglOperation *operation);
static void     clean_pads   (GeglOperation *operation);



G_DEFINE_TYPE (GeglOperationSource, gegl_operation_source, GEGL_TYPE_OPERATION)

static gboolean calc_have_rect (GeglOperation *self);
static gboolean calc_need_rect (GeglOperation *self);

static void
gegl_operation_source_class_init (GeglOperationSourceClass * klass)
{
  GObjectClass       *gobject_class   = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->finalize     = finalize;

  operation_class->evaluate = evaluate;
  operation_class->associate = associate;
  operation_class->clean_pads = clean_pads;

  operation_class->calc_have_rect = calc_have_rect;
  operation_class->calc_need_rect = calc_need_rect;

  g_object_class_install_property (gobject_class, PROP_OUTPUT,
    g_param_spec_object ("output",
                         "Output",
                         "An output property",
                         GEGL_TYPE_BUFFER,
                         G_PARAM_READABLE |
                         GEGL_PAD_OUTPUT));

}

static void
gegl_operation_source_init (GeglOperationSource *self)
{
  self->output = NULL;
}

static void
associate (GeglOperation *self)
{
  GeglOperation   *operation = GEGL_OPERATION (self);
  GObjectClass *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                             g_object_class_find_property (object_class,
                                                           "output"));
}

static void
clean_pads (GeglOperation *operation)
{
  GeglOperationSource *source = GEGL_OPERATION_SOURCE (operation);
  if (source->output)
    {
      g_object_unref (source->output);
      source->output = NULL;
    }
}

static void
finalize (GObject *object)
{
  clean_pads (GEGL_OPERATION (object));

  G_OBJECT_CLASS (gegl_operation_source_parent_class)->finalize (object);
}

static void
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglOperationSource *self = GEGL_OPERATION_SOURCE (object);

  switch (prop_id)
  {
    case PROP_OUTPUT:
      g_value_set_object(value, self->output);
      break;
    default:
      break;
  }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglOperationSource *self = GEGL_OPERATION_SOURCE (object);
  self = NULL;

  switch (prop_id)
  {
    default:
      break;
  }
}

static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationSourceClass *klass = GEGL_OPERATION_SOURCE_GET_CLASS (operation);
  gboolean success;

  success = klass->evaluate (operation, output_prop);

  return success;
}

static gboolean
calc_have_rect (GeglOperation *self)
{
  g_warning ("Gegl Source '%s' has no proper have_rect function",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return FALSE;
}

static gboolean
calc_need_rect (GeglOperation *self)
{
  return TRUE;
}
