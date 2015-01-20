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
 * Copyright 2010 Michael Muré <batolettre@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_pointer (buffer, _("Cache buffer"),
                  _("The GeglBuffer where the caching is done"))

#else

#define GEGL_OP_FILTER
#define GEGL_OP_C_SOURCE buffer-cache.c

#include "gegl-op.h"

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglProperties   *o = GEGL_PROPERTIES (operation);

  if (o->buffer)
    return *gegl_buffer_get_extent (GEGL_BUFFER (o->buffer));
  else
    return *gegl_operation_source_get_bounding_box (operation, "input");
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties           *o = GEGL_PROPERTIES (operation);

  if (!o->buffer)
    {
      o->buffer = gegl_buffer_dup (input);
    }

  output = gegl_buffer_dup (o->buffer);

  return TRUE;
}

static void
dispose (GObject *object)
{
  GeglProperties *o = GEGL_PROPERTIES (object);

  if (o->buffer)
    {
      g_object_unref (o->buffer);
      o->buffer = NULL;
    }

  G_OBJECT_CLASS (gegl_op_parent_class)->dispose (object);
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationFilterClass   *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->get_bounding_box = get_bounding_box;

  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);
  filter_class->process = process;

  G_OBJECT_CLASS (klass)->dispose = dispose;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:buffer-cache",
    "categories"  , "core",
    "description" , _("Cache the input buffer internally, further process take this buffer as input."),
    NULL);

}
#endif
