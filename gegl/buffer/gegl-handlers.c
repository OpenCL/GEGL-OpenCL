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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"

#include <glib.h>

#include "gegl-handlers.h"
#include "gegl-handler-cache.h"


G_DEFINE_TYPE (GeglHandlers, gegl_handlers, GEGL_TYPE_HANDLER)

static void   gegl_handlers_rebind (GeglHandlers *handlers);

static void
gegl_handlers_nuke_cache (GeglHandlers *handlers)
{
  GSList *iter;

  while (gegl_handlers_get_first (handlers, GEGL_TYPE_HANDLER_CACHE))
    {
      iter = handlers->chain;
      while (iter)
        {
          if (GEGL_IS_HANDLER_CACHE (iter->data))
            {
              g_object_unref (iter->data);
              handlers->chain = g_slist_remove (handlers->chain, iter->data);
              gegl_handlers_rebind (handlers);
              break;
            }
          iter = iter->next;
        }
    }
}

static void
dispose (GObject *object)
{
  GeglHandlers *handlers = GEGL_HANDLERS (object);
  GSList       *iter;

  /* Get rid of the cache before any further parts of the deconstruction of the
   * TileStore chain, unwritten tiles need a living TileStore for their
   * deconstruction.
   */
  gegl_handlers_nuke_cache (handlers);

  iter = handlers->chain;
  while (iter)
    {
      if (iter->data)
        g_object_unref (iter->data);
      iter = iter->next;
    }

  if (handlers->chain)
    g_slist_free (handlers->chain);
  handlers->chain = NULL;

  G_OBJECT_CLASS (gegl_handlers_parent_class)->dispose (object);
}


static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (gegl_handlers_parent_class)->finalize (object);
}

static gpointer
command (GeglSource     *tile_store,
         GeglTileCommand command,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglHandlers *handlers = (GeglHandlers *) tile_store;
  GeglSource *source = ((GeglHandler *) tile_store)->source;

  if (handlers->chain != NULL)
    return gegl_source_command ((GeglSource *)(handlers->chain->data),
                                  command, x, y, z, data);
  else if (source)
    return gegl_source_command (source, command, x, y, z, data);
  else
    g_assert (0);

  return FALSE;
}

static void
gegl_handlers_class_init (GeglHandlersClass *class)
{
  GObjectClass      *gobject_class;
  GeglSourceClass *tile_store_class;

  gobject_class    = (GObjectClass *) class;
  tile_store_class = (GeglSourceClass *) class;

  tile_store_class->command  = command;

  gobject_class->finalize = finalize;
  gobject_class->dispose  = dispose;
}

static void
gegl_handlers_init (GeglHandlers *self)
{
  self->chain = NULL;
}


static void
gegl_handlers_rebind (GeglHandlers *handlers)
{
  GSList *iter;


  iter = handlers->chain;
  while (iter)
    {
      GeglHandler  *handler;
      GeglSource *source = NULL;

      handler = iter->data;
      if (iter->next)
        {
          source = g_object_ref (iter->next->data);
        }
      else
        {
          g_object_get (handlers, "source", &source, NULL);
        }
      g_object_set (G_OBJECT (handler), "source", source, NULL);
      g_object_unref (source);
      iter = iter->next;
    }
}

GeglHandler *
gegl_handlers_add (GeglHandlers *handlers,
                   GeglHandler  *handler)
{
  handlers->chain = g_slist_prepend (handlers->chain, handler);
  gegl_handlers_rebind (handlers);
  return handler;
}

/*
 * return the first handler of a given type
 */
GeglHandler *
gegl_handlers_get_first (GeglHandlers *handlers,
                         GType         type)
{
  GSList *iter;

  iter = handlers->chain;
  while (iter)
    {
      if ((G_TYPE_CHECK_INSTANCE_TYPE ((iter->data), type)))
        {
          return iter->data;
        }
      iter = iter->next;
    }
  return NULL;
}
