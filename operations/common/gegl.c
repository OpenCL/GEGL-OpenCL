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
 * Copyright 2006, 2010 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_string (string, _("pipeline"), "invert")
    description(_("[op [property=value] [property=value]] [[op] [property=value]"))

#else

#define GEGL_OP_META
#define GEGL_OP_C_SOURCE gegl.c

#include "gegl-op.h"

static void
attach (GeglOperation *operation)
{
  GeglNode *gegl, *input, *output;

  gegl = operation->node;

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

  gegl_node_link_many (input, output, NULL);
  /*
  gegl_operation_meta_redirect (operation, "scale", multiply, "value");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, add, multiply, subtract, blur, NULL);
  */
}

#include <stdio.h>

static char *cached = NULL;

static void
prepare (GeglOperation *operation)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  GeglNode *gegl, *input, *output;

  gegl = operation->node;

  if (!cached || !g_str_equal (cached, o->string))
  {
    if (cached)
      g_free (cached);
    cached = g_strdup (o->string);

  input    = gegl_node_get_input_proxy (gegl, "input");
  output   = gegl_node_get_output_proxy (gegl, "output");

//  gegl_node_link_many (input, output, NULL);
  gegl_create_chain (o->string, input, output);
  /*
  gegl_operation_meta_redirect (operation, "scale", multiply, "value");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-x");
  gegl_operation_meta_redirect (operation, "std-dev", blur, "std-dev-y");

  gegl_operation_meta_watch_nodes (operation, add, multiply, subtract, blur, NULL);
  */
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
    "categories",  "enhance",
    "description", _("Program a GEGL operation, in the same manner as the GEGL commandline."),
    NULL);
}

#endif
