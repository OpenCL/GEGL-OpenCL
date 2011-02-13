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

#include "gegl-buffer-types.h"
#include "gegl-tile-source.h"
#include "gegl-tile-handler.h"
#include "gegl-tile-handler-chain.h"
#include "gegl-tile-storage.h"

G_DEFINE_TYPE (GeglTileHandler, gegl_tile_handler, GEGL_TYPE_TILE_SOURCE)

enum
{
  PROP0,
  PROP_SOURCE
};

gboolean gegl_tile_storage_cached_release (GeglTileStorage *storage);

static void
gegl_tile_handler_dispose (GObject *object)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (object);

  if (handler->source != NULL)
    {
      if (!(GEGL_IS_TILE_STORAGE (handler->source) &&
	    gegl_tile_storage_cached_release ((void*)handler->source)))
        g_object_unref (handler->source);
      handler->source = NULL;
    }

  G_OBJECT_CLASS (gegl_tile_handler_parent_class)->dispose (object);
}

static gpointer
gegl_tile_handler_command (GeglTileSource *gegl_tile_source,
                           GeglTileCommand command,
                           gint            x,
                           gint            y,
                           gint            z,
                           gpointer        data)
{
  GeglTileHandler *handler = (GeglTileHandler*)gegl_tile_source;

  return gegl_tile_handler_source_command (handler, command, x, y, z, data);
}

static void
gegl_tile_handler_get_property (GObject    *gobject,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (gobject);

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

void
gegl_tile_handler_set_source (GeglTileHandler *handler,
                              GeglTileSource  *source)
{
  if (handler->source != NULL)
    g_object_unref (handler->source);

  if (source == NULL)
    {
      handler->source = NULL;
      return;
    }
  handler->source = g_object_ref (source);

}

static void
gegl_tile_handler_set_property (GObject      *gobject,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  GeglTileHandler *handler = GEGL_TILE_HANDLER (gobject);

  switch (property_id)
    {
      case PROP_SOURCE:
        gegl_tile_handler_set_source (handler, GEGL_TILE_SOURCE (g_value_get_object (value)));
        return;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
gegl_tile_handler_class_init (GeglTileHandlerClass *klass)
{
  GObjectClass    *gobject_class  = G_OBJECT_CLASS (klass);

  gobject_class->set_property = gegl_tile_handler_set_property;
  gobject_class->get_property = gegl_tile_handler_get_property;
  gobject_class->dispose      = gegl_tile_handler_dispose;

  g_object_class_install_property (gobject_class, PROP_SOURCE,
                                   g_param_spec_object ("source",
                                                        "GeglBuffer",
                                                        "The tilestore to be a facade for",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
gegl_tile_handler_init (GeglTileHandler *self)
{
  ((GeglTileSource*)self)->command = gegl_tile_handler_command;
  self->source = NULL;
}


