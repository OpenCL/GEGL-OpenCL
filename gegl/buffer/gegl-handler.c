/* This file is part of GEGL.
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-tile-store.h"
#include "gegl-handler.h"
#include "gegl-tile-traits.h"

G_DEFINE_TYPE (GeglHandler, gegl_handler, GEGL_TYPE_TILE_STORE)
static GObjectClass * parent_class = NULL;

enum
{
  PROP0,
  PROP_SOURCE
};

static void
dispose (GObject *object)
{
  GeglHandler *handler = GEGL_HANDLER (object);

  if (handler->source != NULL)
    {
      g_object_unref (handler->source);
      handler->source = NULL;
    }

  (*G_OBJECT_CLASS (parent_class)->dispose)(object);
}

static GeglTile *
get_tile (GeglTileStore *gegl_tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglHandler *handler = GEGL_HANDLER (gegl_tile_store);
  GeglTile      *tile  = NULL;

  if (handler->source)
    tile = gegl_tile_store_get_tile (handler->source, x, y, z);
  if (tile != NULL)
    return tile;
  return tile;
}

static gboolean
message (GeglTileStore  *gegl_tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglHandler *handler = GEGL_HANDLER (gegl_tile_store);

  if (handler->source)
    return gegl_tile_store_message (handler->source, message, x, y, z, data);
  return FALSE;
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglHandler *handler = GEGL_HANDLER (gobject);

  switch (property_id)
    {
      case PROP_SOURCE:
        g_value_set_object (value, handler->source);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglHandler *handler = GEGL_HANDLER (gobject);

  switch (property_id)
    {
      case PROP_SOURCE:
        if (handler->source != NULL)
          g_object_unref (handler->source);
        handler->source = GEGL_TILE_STORE (g_value_dup_object (value));

        /* special case if we are the Traits subclass of Trait
         * also set the source at the end of the chain.
         */
        if (GEGL_IS_TILE_TRAITS (handler))
          {
            GeglHandlers *handlers = GEGL_HANDLERS (handler);
            GSList         *iter   = (void *) handlers->chain;
            while (iter && iter->next)
              iter = iter->next;
            if (iter)
              {
                g_object_set (GEGL_HANDLER (iter->data), "source", handler->source, NULL);
              }
          }
        return;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_handler_class_init (GeglHandlerClass *klass)
{
  GObjectClass       *gobject_class    = G_OBJECT_CLASS (klass);
  GeglTileStoreClass *tile_store_class = GEGL_TILE_STORE_CLASS (klass);

  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  parent_class                = g_type_class_peek_parent (klass);
  gobject_class->dispose      = dispose;

  g_object_class_install_property (gobject_class, PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        "GeglBuffer",
                                                        "The tilestore to be a facade for",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  tile_store_class->get_tile = get_tile;
  tile_store_class->message  = message;
}

static void
gegl_handler_init (GeglHandler *self)
{
  self->source = NULL;
}
