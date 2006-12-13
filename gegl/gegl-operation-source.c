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
#include <string.h>

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_LAST
};

static void     get_property (GObject      *gobject,
                              guint         prop_id,
                              GValue       *value,
                              GParamSpec   *pspec);
static void     set_property (GObject      *gobject,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec);
static gboolean process      (GeglOperation *operation,
                              gpointer       dynamic_id,
                              const gchar   *output_prop);
static void     associate    (GeglOperation *operation);



G_DEFINE_TYPE (GeglOperationSource, gegl_operation_source, GEGL_TYPE_OPERATION)

static GeglRectangle get_defined_region (GeglOperation *self);
static gboolean calc_source_regions (GeglOperation *self,
                                     gpointer       dynamic_id);

static void
gegl_operation_source_class_init (GeglOperationSourceClass * klass)
{
  GObjectClass       *gobject_class   = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  operation_class->process = process;
  operation_class->associate = associate;

  operation_class->get_defined_region = get_defined_region;
  operation_class->calc_source_regions = calc_source_regions;

  g_object_class_install_property (gobject_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "Ouput pad for generated image buffer.",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PAD_OUTPUT));

}

static void
gegl_operation_source_init (GeglOperationSource *self)
{
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
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
}

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id,
         const gchar   *output_prop)
{
  GeglOperationSourceClass *klass = GEGL_OPERATION_SOURCE_GET_CLASS (operation);
  gboolean success;

  if (strcmp (output_prop, "output"))
    {
      g_warning ("requested processing of %s pad on a source operation", output_prop);
    return FALSE;
    }
  
  g_assert (klass->process);
  success = klass->process (operation, dynamic_id);

  return success;
}

static GeglRectangle
get_defined_region (GeglOperation *self)
{
  GeglRectangle result = {0,0,0,0};
  g_warning ("Gegl Source '%s' has no proper have_rect function",
     G_OBJECT_CLASS_NAME (G_OBJECT_GET_CLASS(self)));
  return result;
}

static gboolean
calc_source_regions (GeglOperation *self,
                     gpointer       dynamic_id)
{
  return TRUE;
}
