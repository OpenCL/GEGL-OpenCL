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
#include "gegl-tile-zoom.h"

G_DEFINE_TYPE(GeglTileZoom, gegl_tile_zoom, GEGL_TYPE_TILE_TRAIT)
static GObjectClass *parent_class = NULL;
enum {
  PROP_0,
  PROP_STORAGE,
  PROP_BACKEND,
};

#include <babl/babl.h>
#include "gegl-tile-backend.h"

static void inline set_blank(GeglTile *dst_tile,
                             gint      width,
                             gint      height,
                             Babl     *format,
                             gint      i,
                             gint      j)
{
  guchar *dst_data = gegl_tile_get_data (dst_tile);
  gint  bpp = format->format.bytes_per_pixel;
  gint x,y;

  for (y=0;y<height/2;y++)
    {
      guchar *dst = dst_data + 
           (
               (
                  (y + j * (height/2)) * width
               ) + i * (width/2)
           )*bpp;
      for (x=0;x<width/2;x++)
        {
          memset (dst, 0x0, bpp);
          dst += bpp;
        }
    }
}

static void inline set_half_nearest (GeglTile *dst_tile,
                                     GeglTile *src_tile,
                                     gint      width,
                                     gint      height,
                                     Babl     *format,
                                     gint      i,
                                     gint      j)
{
  guchar *dst_data = gegl_tile_get_data (dst_tile);
  guchar *src_data = gegl_tile_get_data (src_tile);
  gint  bpp = format->format.bytes_per_pixel;
  gint x,y;

  for (y=0; y<height/2; y++)
    {
      guchar *dst = dst_data + 
           (
               (
                  (y + j * (height/2)) * width
               ) + i * (width/2)
           )*bpp;

      guchar *src = src_data + (y * 2 * width) * bpp;

      for (x=0;x<width/2;x++)
        {
          memcpy (dst, src, bpp);
          dst += bpp;
          src += bpp*2;
        }
    }
}

static inline void
downscale_u8 (gint    components,
              guchar *src_data,
              gint    src_width,
              gint    src_height,
              gint    src_rowstride,
              guchar *dst_data,
              gint    dst_rowstride)
{
  gint y;
  if (!src_data || !dst_data)
    return;
  for (y=0; y<src_height/2; y++)
    {
       gint x;
       guchar *dst = dst_data + y*dst_rowstride;
       guchar *src = src_data + y*2*src_rowstride;

       for (x=0; x<src_width/2; x++)
         {
            int i;
            for (i=0; i<components; i++)
              dst[i] = (src[i] + src[i+components] + src[i + src_rowstride] + src[i + src_rowstride + components]) /
components;

            dst+=components;
            src+=components*2;
         }
    }
}

static void inline set_half_u8 (GeglTile *dst_tile,
                                GeglTile *src_tile,
                                gint      width,
                                gint      height,
                                Babl     *format,
                                gint      i,
                                gint      j)
{
  guchar *dst_data = gegl_tile_get_data (dst_tile);
  guchar *src_data = gegl_tile_get_data (src_tile);
  gint  components = format->format.components;
  gint  bpp        = format->format.bytes_per_pixel;

  if (i)dst_data+= bpp*width/2;
  if (j)dst_data+= bpp*width*height/2;

  downscale_u8 (components, src_data, width, height, width*bpp,
                            dst_data, width*bpp);
}

static void inline set_half (GeglTile *dst_tile,
                             GeglTile *src_tile,
                             gint      width,
                             gint      height,
                             Babl     *format,
                             gint      i,
                             gint      j)
{
  if (format->format.type[0] == (BablType*)babl_type ("u8"))
    {
      set_half_u8 (dst_tile, src_tile, width, height, format, i, j);
    }
  else
    {
      set_half_nearest (dst_tile, src_tile, width, height, format, i, j);
    }
}


/* FIXME: I do not think z!=0 tiles are currently freed|voided from
 * storage
 */

static GeglTile *
get_tile (GeglTileStore *gegl_tile_store,
          gint          x,
          gint          y,
          gint          z)
{
  GeglTileStore *source = GEGL_TILE_TRAIT (gegl_tile_store)->source;
  GeglTileZoom  *zoom  = GEGL_TILE_ZOOM (gegl_tile_store);
  GeglTile      *tile   = NULL;
  Babl          *format = (Babl*)(zoom->backend->format);
  gint           tile_width;
  gint           tile_height;
  gint           tile_size;

  if (source)
    {
      tile = gegl_tile_store_get_tile (source, x, y, z);
    }

  if (tile != NULL)
    {
      return tile;
    }

  if (z==0 &&
      tile == NULL)
    return NULL;

  g_assert (zoom->backend);
  g_object_get (zoom->backend, "tile-width",  &tile_width,
                               "tile-height", &tile_height,
                               "tile-size",   &tile_size,
                               NULL);

  tile = gegl_tile_new (tile_size);
    {
      gint i,j;
      guchar   *data;
      data = gegl_tile_get_data (tile);

      for (i=0;i<2;i++)
        for (j=0;j<2;j++)
          {
            GeglTile *source_tile;
            source_tile = gegl_tile_store_get_tile (source, x*2+i, y*2+j, z-1);

            if (source_tile)
              {
                set_half (tile, source_tile, tile_width, tile_height, format, i, j);
                g_object_unref (source_tile);
              }
            else
              {
                set_blank (tile, tile_width, tile_height, format, i, j);
              }
          }
    }
  return tile;
}


static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglTileZoom *zoom = GEGL_TILE_ZOOM (gobject);
  switch(property_id)
    {
      case PROP_STORAGE:
        g_value_set_object (value, zoom->storage);
        break;
      case PROP_BACKEND:
        g_value_set_object (value, zoom->backend);
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
  GeglTileZoom *zoom = GEGL_TILE_ZOOM (gobject);
  switch(property_id)
    {
      case PROP_STORAGE:
        zoom->storage = g_value_get_object (value);
        break;
      case PROP_BACKEND:
        zoom->backend = g_value_get_object (value);
        break;
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
  GeglTileZoom *zoom;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  zoom = GEGL_TILE_ZOOM (object);

  return object;
}


static void
gegl_tile_zoom_class_init (GeglTileZoomClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GeglTileStoreClass *gegl_tile_store_class = GEGL_TILE_STORE_CLASS (klass);

  gegl_tile_store_class->get_tile = get_tile;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
  gobject_class->constructor  = constructor;

  g_object_class_install_property (gobject_class, PROP_STORAGE,
                                   g_param_spec_object ("storage",
                                                        "storage",
                                                        "storage for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));


  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_object ("backend",
                                                        "backend",
                                                        "backend for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));

  parent_class = g_type_class_peek_parent (klass);
}

static void
gegl_tile_zoom_init (GeglTileZoom *self)
{
  self->backend = NULL;
  self->storage = NULL;
}
