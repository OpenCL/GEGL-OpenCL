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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Copyright 2006,2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>
#include <string.h>

#include "gegl-handler.h"
#include "gegl-handler-zoom.h"

G_DEFINE_TYPE (GeglHandlerZoom, gegl_handler_zoom, GEGL_TYPE_TILE_TRAIT)
static GObjectClass * parent_class = NULL;
enum
{
  PROP_0,
  PROP_STORAGE,
  PROP_BACKEND,
};

#include <babl/babl.h>
#include "gegl-tile-backend.h"

static inline void set_blank (GeglTile *dst_tile,
                              gint      width,
                              gint      height,
                              Babl     *format,
                              gint      i,
                              gint      j)
{
  guchar *dst_data  = gegl_tile_get_data (dst_tile);
  gint    bpp       = format->format.bytes_per_pixel;
  gint    rowstride = width * bpp;
  gint    scanline;

  gint    bytes = width * bpp / 2;
  guchar *dst   = dst_data + j * height / 2 * rowstride + i * rowstride / 2;

  for (scanline = 0; scanline < height / 2; scanline++)
    {
      memset (dst, 0x0, bytes);
      dst += rowstride;
    }
}

/* fixme: make the api of this, as well as blank be the
 * same as the downscale functions */
static inline void set_half_nearest (GeglTile *dst_tile,
                                     GeglTile *src_tile,
                                     gint      width,
                                     gint      height,
                                     Babl     *format,
                                     gint      i,
                                     gint      j)
{
  guchar *dst_data = gegl_tile_get_data (dst_tile);
  guchar *src_data = gegl_tile_get_data (src_tile);
  gint    bpp      = format->format.bytes_per_pixel;
  gint    x, y;

  for (y = 0; y < height / 2; y++)
    {
      guchar *dst = dst_data +
                    (
        (
          (y + j * (height / 2)) * width
        ) + i * (width / 2)
                    ) * bpp;

      guchar *src = src_data + (y * 2 * width) * bpp;

      for (x = 0; x < width / 2; x++)
        {
          memcpy (dst, src, bpp);
          dst += bpp;
          src += bpp * 2;
        }
    }
}

static inline void
downscale_float (gint    components,
                 gint    width,
                 gint    height,
                 gint    rowstride,
                 guchar *src_data,
                 guchar *dst_data)
{
  gint y;

  if (!src_data || !dst_data)
    return;
  for (y = 0; y < height / 2; y++)
    {
      gint    x;
      gfloat *dst = (gfloat *) (dst_data + y * rowstride);
      gfloat *src = (gfloat *) (src_data + y * 2 * rowstride);

      for (x = 0; x < width / 2; x++)
        {
          int i;
          for (i = 0; i < components; i++)
            dst[i] = (src[i] +
                      src[i + components] +
                      src[i + (width * components)] +
                      src[i + (width + 1) * components]) /
                     4.0;

          dst += components;
          src += components * 2;
        }
    }
}

static inline void
downscale_u8 (gint    components,
              gint    width,
              gint    height,
              gint    rowstride,
              guchar *src_data,
              guchar *dst_data)
{
  gint y;

  if (!src_data || !dst_data)
    return;
  for (y = 0; y < height / 2; y++)
    {
      gint    x;
      guchar *dst = dst_data + y * rowstride;
      guchar *src = src_data + y * 2 * rowstride;

      for (x = 0; x < width / 2; x++)
        {
          int i;
          for (i = 0; i < components; i++)
            dst[i] = (src[i] +
                      src[i + components] +
                      src[i + rowstride] +
                      src[i + rowstride + components]) /
                     4;

          dst += components;
          src += components * 2;
        }
    }
}

static void inline set_half (GeglTile * dst_tile,
                             GeglTile * src_tile,
                             gint width,
                             gint height,
                             Babl * format,
                             gint i,
                             gint j)
{
  guchar *dst_data   = gegl_tile_get_data (dst_tile);
  guchar *src_data   = gegl_tile_get_data (src_tile);
  gint    components = format->format.components;
  gint    bpp        = format->format.bytes_per_pixel;

  if (i) dst_data += bpp * width / 2;
  if (j) dst_data += bpp * width * height / 2;

  if (format->format.type[0] == (BablType *) babl_type ("float"))
    {
      downscale_float (components, width, height, width * bpp, src_data, dst_data);
    }
  else if (format->format.type[0] == (BablType *) babl_type ("u8"))
    {
      downscale_u8 (components, width, height, width * bpp, src_data, dst_data);
    }
  else
    {
      set_half_nearest (dst_tile, src_tile, width, height, format, i, j);
    }
}

static GeglTile *
get_tile (GeglProvider *gegl_provider,
          gint           x,
          gint           y,
          gint           z)
{
  GeglProvider    *provider = GEGL_HANDLER (gegl_provider)->provider;
  GeglHandlerZoom *zoom   = GEGL_HANDLER_ZOOM (gegl_provider);
  GeglTile        *tile   = NULL;
  Babl            *format = (Babl *) (zoom->backend->format);
  gint             tile_width;
  gint             tile_height;
  gint             tile_size;

  if (provider)
    {
      tile = gegl_provider_get_tile (provider, x, y, z);
    }

  if (tile != NULL)
    {
      /* Check that the tile is fully valid */
      if (!(tile->flags & (GEGL_TILE_DIRT_TL |
                           GEGL_TILE_DIRT_TR |
                           GEGL_TILE_DIRT_BL |
                           GEGL_TILE_DIRT_BR)))
        return tile;
    }

  if (z == 0)/* at base level with no tile found->send null, and shared empty
               tile will be used instead */
    {
      return NULL;
    }

  g_assert (zoom->backend);
  g_object_get (zoom->backend, "tile-width", &tile_width,
                "tile-height", &tile_height,
                "tile-size", &tile_size,
                NULL);

  {
    gint      i, j;
    guchar   *data;
    gboolean  had_tile          = tile != NULL;
    GeglTile *source_tile[2][2] = { { NULL, NULL }, { NULL, NULL } };
    gboolean  fetch[2][2]       = { { FALSE, FALSE },
                                    { FALSE, FALSE } };

    if (had_tile)
      {
        if (tile->flags & GEGL_TILE_DIRT_TL)
          fetch[0][0] = TRUE;
        if (tile->flags & GEGL_TILE_DIRT_TR)
          fetch[1][0] = TRUE;
        if (tile->flags & GEGL_TILE_DIRT_BL)
          fetch[0][1] = TRUE;
        if (tile->flags & GEGL_TILE_DIRT_BR)
          fetch[1][1] = TRUE;

        tile->flags = 0;
      }
    else
      {
        fetch[0][0] = TRUE;
        fetch[1][0] = TRUE;
        fetch[0][1] = TRUE;
        fetch[1][1] = TRUE;
      }

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          /* we get the tile from ourselves, to make successive rescales work
           * correctly */
          if (fetch[i][j])
            source_tile[i][j] = gegl_provider_get_tile (gegl_provider,
                                                          x * 2 + i, y * 2 + j, z - 1);
        }

    if (source_tile[0][0] == NULL &&
        source_tile[0][1] == NULL &&
        source_tile[1][0] == NULL &&
        source_tile[1][1] == NULL)
      {
        if (had_tile)
          {
            g_object_unref (tile);
          }
        return NULL;   /* no data from level below, return NULL and let GeglHandlerEmpty
                          fill in the shared empty tile */
      }

    if (!had_tile)
      {
        tile = gegl_tile_new (tile_size);

        /* it is a bit hacky, but adding enough information (probably too much)
         * enabling the storage system to attempt swapping out of zoom tiles
         */
        tile->storage_x  = x;
        tile->storage_y  = y;
        tile->storage_z  = z;
        tile->x          = x;
        tile->y          = y;
        tile->z          = z;
        tile->storage    = zoom->storage;
        tile->stored_rev = 0;
        tile->rev        = 1;
      }
    gegl_tile_lock (tile);
    data = gegl_tile_get_data (tile);

    for (i = 0; i < 2; i++)
      for (j = 0; j < 2; j++)
        {
          if (source_tile[i][j])
            {
              set_half (tile, source_tile[i][j], tile_width, tile_height, format, i, j);
              g_object_unref (source_tile[i][j]);
            }
          else if (fetch[i][j])
            {
              set_blank (tile, tile_width, tile_height, format, i, j);
            }
          else
            {
              /* keep old data around */
            }
        }
    gegl_tile_unlock (tile);
  }

  tile->flags = 0;

  return tile;
}

static gboolean
message (GeglProvider  *tile_store,
         GeglTileMessage message,
         gint            x,
         gint            y,
         gint            z,
         gpointer        data)
{
  GeglHandler *handler  = GEGL_HANDLER (tile_store);
  GeglProvider *provider = handler->provider;

  if (message == GEGL_TILE_VOID_TL ||
      message == GEGL_TILE_VOID_TR ||
      message == GEGL_TILE_VOID_BL ||
      message == GEGL_TILE_VOID_BR)
    {
      GeglTile *tile = gegl_provider_get_tile (provider, x, y, z);

      if (!tile)
        return FALSE;
      switch (message)
        {
          case GEGL_TILE_VOID_TL:
            tile->flags |= GEGL_TILE_DIRT_TL;
            break;

          case GEGL_TILE_VOID_TR:
            tile->flags |= GEGL_TILE_DIRT_TR;
            break;

          case GEGL_TILE_VOID_BL:
            tile->flags |= GEGL_TILE_DIRT_BL;
            break;

          case GEGL_TILE_VOID_BR:
            tile->flags |= GEGL_TILE_DIRT_BR;
            break;

          default:
            break;
        }
      g_object_unref (tile);
      return FALSE;
    }
  /* pass the message on */
  if (handler->provider)
    return gegl_provider_message (handler->provider, message, x, y, z, data);
  return FALSE; /* pass it on */
}


static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  GeglHandlerZoom *zoom = GEGL_HANDLER_ZOOM (gobject);

  switch (property_id)
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
  GeglHandlerZoom *zoom = GEGL_HANDLER_ZOOM (gobject);

  switch (property_id)
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
  GObject      *object;
  GeglHandlerZoom *zoom;

  object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);
  zoom   = GEGL_HANDLER_ZOOM (object);

  return object;
}


static void
gegl_handler_zoom_class_init (GeglHandlerZoomClass *klass)
{
  GObjectClass       *gobject_class         = G_OBJECT_CLASS (klass);
  GeglProviderClass *gegl_provider_class = GEGL_PROVIDER_CLASS (klass);

  gegl_provider_class->get_tile = get_tile;
  gegl_provider_class->message  = message;
  gobject_class->set_property     = set_property;
  gobject_class->get_property     = get_property;
  gobject_class->constructor      = constructor;

  g_object_class_install_property (gobject_class, PROP_STORAGE,
                                   g_param_spec_object ("storage",
                                                        "storage",
                                                        "storage for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));


  g_object_class_install_property (gobject_class, PROP_BACKEND,
                                   g_param_spec_object ("backend",
                                                        "backend",
                                                        "backend for this tilestore (needed for tile size data)",
                                                        G_TYPE_OBJECT,
                                                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  parent_class = g_type_class_peek_parent (klass);
}

static void
gegl_handler_zoom_init (GeglHandlerZoom *self)
{
  self->backend = NULL;
  self->storage = NULL;
}
