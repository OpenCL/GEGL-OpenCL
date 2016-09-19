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
 * Copyright 2016 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_string (string, _("pipeline"), "gaussian-blur std-dev-x=0.3rel std-dev-y=0.3rel")
    description(_("[op [property=value] [property=value]] [[op] [property=value]"))
    ui_meta ("multiline", "true")

property_string (error, _("Eeeeeek"), "")
    description (_("There is a problem in the syntax or in the application of parsed property values. Things might mostly work nevertheless."))
    ui_meta ("error", "true")

#else

#define GEGL_OP_META
#define GEGL_OP_NAME     gegl
#define GEGL_OP_C_SOURCE gegl.c

#include "gegl-op.h"

/* XXX: leaking o->user_data */

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output;

  gegl = operation->node;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, output, NULL);
}

#include <stdio.h>

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl, *input, *output;
  GError *error = NULL;

  gegl = operation->node;

  if (!o->user_data || !g_str_equal (o->user_data, o->string))
  {
    if (o->user_data)
      g_free (o->user_data);
    o->user_data = g_strdup (o->string);

  input  = gegl_node_get_input_proxy (gegl,  "input");
  output = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, output, NULL);
  gegl_create_chain (o->string, input, output, 0.0,
                     gegl_node_get_bounding_box (input).height,
                     &error);

  if (error)
  {
    gegl_node_set (gegl, "error", error->message, NULL);
    g_error_free (error);
    error = NULL;
  }
  else
    g_object_set (operation, "error", "", NULL);
  }
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class;

  operation_class = GEGL_OPERATION_CLASS (klass);

  operation_class->attach = attach;
  operation_class->prepare = prepare;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:gegl",
    "title",       _("GEGL graph"),
    "categories",  "generic",
    "description", _("Do a chain of operations, with key=value pairs after each operation name to set properties. And aux=[ source filter ] for specifying a chain with a source as something connected to an aux pad."),
    NULL);
}

#endif
