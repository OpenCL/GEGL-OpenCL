/*
 * Gegl id-pool
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * GeglIDPool: pool of reusable integer ids associated with pointers.
 *
 * Copyright (C) 2008 OpenedHand
 * Author: Øyvind Kolås <pippin@o-hand-com>
 *
 */

#define DEBUG_POOL

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gegl-id-pool.h"

struct _GeglIDPool
{
  GArray *array;     /* Array of pointers    */
  GSList *free_ids;  /* A stack of freed ids */
};

GeglIDPool *
gegl_id_pool_new  (guint initial_size)
{
  GeglIDPool *self;

  self = g_slice_new (GeglIDPool);

  self->array = g_array_sized_new (FALSE, FALSE,
                                   sizeof (gpointer), initial_size);
  self->free_ids = NULL;
  return self;
}

void
gegl_id_pool_free (GeglIDPool *id_pool)
{
  g_return_if_fail (id_pool != NULL);

  g_array_free (id_pool->array, TRUE);
  g_slist_free (id_pool->free_ids);
  g_slice_free (GeglIDPool, id_pool);
}

guint32
gegl_id_pool_add (GeglIDPool *id_pool,
                  gpointer    ptr)
{
  gpointer *array;
  guint32   id;

  g_return_val_if_fail (id_pool != NULL, 0);

  if (id_pool->free_ids) /* There are items on our freelist, reuse one */
    {
      array = (void*) id_pool->array->data;
      id = GPOINTER_TO_UINT (id_pool->free_ids->data);

      id_pool->free_ids = g_slist_remove (id_pool->free_ids,
                                          id_pool->free_ids->data);
      array[id] = ptr;
      return id;
    }

  /* Allocate new id */
  id = id_pool->array->len;
  g_array_append_val (id_pool->array, ptr);
  return id;
}

void
gegl_id_pool_remove (GeglIDPool *id_pool,
                     guint32     id)
{
  gpointer *array;
#ifdef DEBUG_POOL
  return;
#endif

  g_return_if_fail (id_pool != NULL);
  array = (void*) id_pool->array->data;

  array[id] = (void*)0xdecafbad;   /* set pointer to a recognizably voided
                                      value */

  id_pool->free_ids = g_slist_prepend (id_pool->free_ids,
                                       GUINT_TO_POINTER (id));
}

gpointer
gegl_id_pool_lookup (GeglIDPool *id_pool,
                     guint32     id)
{
  gpointer *array;

  g_return_val_if_fail (id_pool != NULL, NULL);
  g_return_val_if_fail (id_pool->array != NULL, NULL);

  g_return_val_if_fail (id < id_pool->array->len, NULL);

  array = (void*) id_pool->array->data;

  return array[id];
}
