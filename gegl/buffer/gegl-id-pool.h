/* GEGL -
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
 * Author: Øyvind Kolås <pippin@o-hand.com>
 *
 * Clutter
 * An OpenGL based 'interactive canvas' library.
 * Authored By Matthew Allum  <mallum@openedhand.com>
 *
 * Copyright (C) 2008 OpenedHand
 */

#ifndef __GEGL_ID_POOL_H__
#define __GEGL_ID_POOL_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _GeglIDPool   GeglIDPool;

GeglIDPool *gegl_id_pool_new     (guint       initial_size);
void        gegl_id_pool_free    (GeglIDPool *id_pool);
guint32     gegl_id_pool_add     (GeglIDPool *id_pool,
                                  gpointer    ptr);
void        gegl_id_pool_remove  (GeglIDPool *id_pool,
                                  guint32     id);
gpointer    gegl_id_pool_lookup  (GeglIDPool *id_pool,
                                  guint32     id);


G_END_DECLS

#endif /* __CLUTTER_ID_POOL_H__ */
