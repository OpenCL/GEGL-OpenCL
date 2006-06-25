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
#include "gegl-operation-filter.h"

enum
{
  PROP_0,
  PROP_OUTPUT,
  PROP_INPUT,
  PROP_LAST
};

static void     finalize             (GObject       *gobject);

static void     get_property         (GObject       *gobject,
                                      guint          prop_id,
                                      GValue        *value,
                                      GParamSpec    *pspec);

static void     set_property         (GObject       *gobject,
                                      guint          prop_id,
                                      const GValue  *value,
                                      GParamSpec    *pspec);

static gboolean evaluate             (GeglOperation *operation,
                                      const gchar   *output_prop);

static void     associate            (GeglOperation *operation);

static gboolean calc_have_rect       (GeglOperation *self);
static gboolean calc_need_rect       (GeglOperation *self);
static void     clean_pads           (GeglOperation *operation);

G_DEFINE_TYPE (GeglOperationFilter, gegl_operation_filter, GEGL_TYPE_OPERATION)


static void
gegl_operation_filter_class_init (GeglOperationFilterClass * klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);

  object_class->set_property = set_property;
  object_class->get_property = get_property;
  object_class->finalize     = finalize;

  operation_class->evaluate = evaluate;
  operation_class->associate = associate;
  operation_class->clean_pads = clean_pads;
  operation_class->calc_have_rect = calc_have_rect;
  operation_class->calc_need_rect = calc_need_rect;

  g_object_class_install_property (object_class, PROP_OUTPUT,
                                   g_param_spec_object ("output",
                                                        "Output",
                                                        "An output property",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_READABLE |
                                                        GEGL_PAD_OUTPUT));

  g_object_class_install_property (object_class, PROP_INPUT,
                                   g_param_spec_object ("input",
                                                        "Input",
                                                        "An input property",
                                                        GEGL_TYPE_BUFFER,
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_READWRITE |
                                                        GEGL_PAD_INPUT));
}

static void
gegl_operation_filter_init (GeglOperationFilter *self)
{
  self->input  = NULL;
  self->output = NULL;
}

static void
associate (GeglOperation *self)
{
  GeglOperation  *operation    = GEGL_OPERATION (self);
  GObjectClass   *object_class = G_OBJECT_GET_CLASS (self);

  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "output"));
  gegl_operation_create_pad (operation,
                               g_object_class_find_property (object_class,
                                                             "input"));
}

static void
clean_pads (GeglOperation *operation)
{
  GeglOperationFilter *filter = GEGL_OPERATION_FILTER (operation);
  if (filter->output)
    {
      g_object_unref (filter->output);
      filter->output = NULL;
    }
}

static void
finalize (GObject *object)
{
  clean_pads (GEGL_OPERATION (object));

  G_OBJECT_CLASS (gegl_operation_filter_parent_class)->finalize (object);
}

static void
get_property (GObject      *object,
              guint         prop_id,
              GValue       *value,
              GParamSpec   *pspec)
{
  GeglOperationFilter *self = GEGL_OPERATION_FILTER (object);

  switch (prop_id)
  {
    case PROP_OUTPUT:
      g_value_set_object (value, self->output);
      break;
    case PROP_INPUT:
      g_value_set_object (value, self->input);
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
  GeglOperationFilter *self = GEGL_OPERATION_FILTER (object);

  switch (prop_id)
  {
    case PROP_INPUT:
      self->input = g_value_get_object(value);
      break;
    default:
      break;
  }
}


static gboolean
evaluate (GeglOperation *operation,
          const gchar   *output_prop)
{
  GeglOperationFilter      *gegl_operation_filter;
  GeglOperationFilterClass *klass;
  gboolean success = FALSE;

  gegl_operation_filter = GEGL_OPERATION_FILTER (operation);
  klass                 = GEGL_OPERATION_FILTER_GET_CLASS (operation);

  g_assert (klass->evaluate);

  if (gegl_operation_filter->input != NULL)
    {
      success = klass->evaluate (operation, output_prop);
      g_object_unref (gegl_operation_filter->input);
      gegl_operation_filter->input=NULL;
    }
  else
    {
      g_warning ("%s received NULL input",
		 gegl_node_get_debug_name (operation->node));
    }
  return success;
}

static gboolean
calc_have_rect (GeglOperation *self)
{
  GeglRect *in_rect;

  in_rect = gegl_operation_get_have_rect (self, "input");
  if (!in_rect)
    return FALSE;

  gegl_operation_set_have_rect (self,
     in_rect->x, in_rect->y,
     in_rect->w, in_rect->h);

  return TRUE;
}

static gboolean
calc_need_rect (GeglOperation *self)
{
  GeglRect requested;

  gegl_operation_get_requested_rect (self, "output", &requested);
  gegl_operation_set_need_rect (self,
     requested.x, requested.y,
     requested.w, requested.h);

  return TRUE;
}
