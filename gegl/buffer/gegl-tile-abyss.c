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

#include "gegl-tile-trait.h"
#include "gegl-tile-abyss.h"

G_DEFINE_TYPE(GeglTileAbyss, gegl_tile_abyss, GEGL_TYPE_TILE_TRAIT)
static GObjectClass *parent_class = NULL;
enum {
  PROP_0,
  PROP_BACKEND,
  PROP_X,
  PROP_Y,
  PROP_WIDTH,
  PROP_HEIGHT
};

static void
finalize (GObject *object)
{
  GeglTileAbyss *abyss = GEGL_TILE_ABYSS (object);
  if (abyss->tile)
    g_object_unref (abyss->tile);
  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static GeglTile *
get_tile (GeglTileStore *tile_store,
          gint           x,
          gint           y,
          gint           z)
{
  GeglTileStore   *source  = GEGL_TILE_TRAIT (tile_store)->source;
  GeglTileAbyss   *abyss   = GEGL_TILE_ABYSS (tile_store);
  GeglTile        *tile    = NULL;

  /* FIXME: Z will have issues here */
  if (source &&
      x >= abyss->x && 
      y >= abyss->y && 
      x < abyss->x + abyss->width && 
      y < abyss->y + abyss->height)
    tile = gegl_tile_store_get_tile (source, x, y, z);
  if (tile != NULL)
    return tile;

  tile = gegl_tile_dup (abyss->tile);

  return tile;
}

static gboolean
message (GeglTileStore   *tile_store,
         GeglTileMessage  message,
         gint             x,
         gint             y,
         gint             z,
         gpointer         data)
{
  GeglTileStore   *source  = GEGL_TILE_TRAIT (tile_store)->source;
  GeglTileAbyss   *abyss   = GEGL_TILE_ABYSS (tile_store);

  /* FIXME: Z will have issues here */
  if (source &&
      x >= abyss->x && 
      y >= abyss->y && 
      x < abyss->x + abyss->width && 
      y < abyss->y + abyss->height)
    return gegl_tile_store_message (source, message, x, y, z, data);
  return FALSE;
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileAbyss *abyss = GEGL_TILE_ABYSS (gobject);
  switch(property_id)
    {
      case PROP_BACKEND:
        g_value_set_object (value, abyss->backend);
        break;
      case PROP_WIDTH:
        g_value_set_int (value, abyss->width);
        break;
      case PROP_HEIGHT:
        g_value_set_int (value, abyss->height);
        break;
      case PROP_X:
        g_value_set_int (value, abyss->x);
        break;
      case PROP_Y:
        g_value_set_int (value, abyss->y);
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
  GeglTileAbyss *abyss = GEGL_TILE_ABYSS (gobject);
  switch(property_id)
    {
      case PROP_BACKEND:
        abyss->backend = g_value_get_object (value);
        break;
     case PROP_WIDTH:
        abyss->width = g_value_get_int (value);
        break;;
      case PROP_HEIGHT:
        abyss->height = g_value_get_int (value);
        break;;
      case PROP_X:
        abyss->x = g_value_get_int (value);
        break;;
      case PROP_Y:
        abyss->y = g_value_get_int (value);
        break;;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static GObject *
constructor (GType                  type,
             guint                  n_params,
             GObjectConstructParam *params)
{
  GObject         *object;
  GeglTileAbyss *abyss;
  gint tile_width;
  gint tile_height;
  gint tile_size;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  abyss = GEGL_TILE_ABYSS (object);

  g_assert (abyss->backend);
  g_object_get (abyss->backend, "tile-width", &tile_width,
                                "tile-height", &tile_height,
                                "tile-size", &tile_size,
                               NULL);
  /* FIXME: need babl format here */
  abyss->tile = gegl_tile_new (tile_size);
  memset (gegl_tile_get_data (abyss->tile), 0x00, tile_size);
  
  return object;
}


static void
gegl_tile_abyss_class_init (GeglTileAbyssClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglTileStoreClass *gegl_tile_store_class = GEGL_TILE_STORE_CLASS (klass);

  gegl_tile_store_class->get_tile = get_tile;
  gegl_tile_store_class->message = message;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor  = constructor;

  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_object ("backend",
                                                        "backend",
                                                        "backend for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
                                   g_param_spec_int ("width", "width", "width of legal rectangle",
                                                     0, G_MAXINT, G_MAXINT,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                   g_param_spec_int ("height", "height", "height of legal rectangle",
                                                     0, G_MAXINT, G_MAXINT,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_X,
                                   g_param_spec_int ("x", "x", "upper left corner of legal rectangle",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));
  g_object_class_install_property (gobject_class, PROP_Y,
                                   g_param_spec_int ("y", "y", "upper lft corner of legal rectangle",
                                                     G_MININT, G_MAXINT, 0,
                                                     G_PARAM_READWRITE|
                                                     G_PARAM_CONSTRUCT_ONLY));


  
  gobject_class->finalize = finalize;
  parent_class = g_type_class_peek_parent (klass);
}

static void
gegl_tile_abyss_init (GeglTileAbyss *self)
{
}
