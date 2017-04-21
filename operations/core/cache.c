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
 * Copyright 2014 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl.h"
#include "gegl-node-private.h" /* to fish out the cache */

#ifdef GEGL_PROPERTIES
  property_object (cache, _("Cache"), GEGL_TYPE_BUFFER)
      description (_("NULL or a GeglBuffer containing cached rendering results, this is a special buffer where gegl_buffer_list_valid_rectangles returns the part of the cache that is valid."))
#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     cache
#define GEGL_OP_C_SOURCE cache.c

#include "gegl-op.h"

static void
prepare (GeglOperation *operation)
{
  const Babl *format = gegl_operation_get_source_format (operation, "input");

  if (! format)
    format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o  = GEGL_PROPERTIES (operation);
  const Babl *format = gegl_operation_get_format (operation, "input");

  memcpy (out_buf, in_buf, n_pixels * babl_format_get_bytes_per_pixel (format));

  if (o->cache != (void *) operation->node->cache)
    {
      if (o->cache)
        {
          g_object_unref (o->cache);
          o->cache = NULL;
        }

      if (operation->node->cache)
        o->cache = g_object_ref (operation->node->cache);
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class;
  GeglOperationPointFilterClass *point_filter_class;

  operation_class    = GEGL_OPERATION_CLASS (klass);
  point_filter_class = GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  operation_class->no_cache      = FALSE;
  operation_class->want_in_place = FALSE;
  operation_class->prepare       = prepare;
  point_filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",        "gegl:cache",
    "title",       _("Explicit cache of results at this point of graph."),
    "categories",  "programming",
    "description", _("An explicit caching node"),
    NULL);
}

#endif
