/* This file is part of GEGL.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <string.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-buffer-private.h"
#include "gegl-id-pool.h"

#if 0
static GeglIDPool *pool = NULL;

guint
gegl_buffer_share (GeglBuffer *buffer)
{
  guint id;
  if (!pool)
    pool = gegl_id_pool_new (16);
  id = gegl_id_pool_add (pool, buffer);
  /* FIXME: weak reference to void the handle when the buffer is
   * finalized
   */
  return id;
}

GeglBuffer*
gegl_buffer_open (const gchar *uri)
{
  /* only supports local addresses for now */
  guint process; /* self */
  guint handle;

  process = 0;
  handle = 0;

  if (!pool)
    pool = gegl_id_pool_new (16);

  if (!g_str_has_prefix (uri, "buffer://"))
   {
     g_warning ("'%s' does not start like a valid buffer handle", uri);
     return NULL;
   }
  if (g_str_has_prefix (uri, "buffer:////"))
   {
     /* local buffer */
     handle = atoi (uri + 11);
     g_print ("got %i, %p\n", handle, gegl_id_pool_lookup (pool, handle));
     return gegl_buffer_create_sub_buffer (gegl_id_pool_lookup (pool, handle), NULL);
   }
  g_warning ("don't know how to handle buffer path: %s", uri);
  return NULL;
}
#endif
