/* This file is part of GEGL.
 * ck
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
 * Copyright 2006-2008 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_BUFFER_PRIVATE_H__
#define __GEGL_BUFFER_PRIVATE_H__

#include "gegl-buffer-types.h"
#include "gegl-buffer.h"
#include "gegl-tile-handler.h"
#include "gegl-buffer-iterator.h"
#include "gegl-buffer-cl-iterator.h"

// #define GEGL_USE_TILE_MUTEX

#define GEGL_BUFFER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_BUFFER, GeglBufferClass))
#define GEGL_IS_BUFFER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_BUFFER))
#define GEGL_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_BUFFER, GeglBufferClass))

struct _GeglBuffer
{
  GeglTileHandler   parent_instance; /* which is a GeglTileHandler which has a
                                        source field which is used for chaining
                                        sub buffers with their anchestors */

  GeglRectangle     extent;        /* the dimensions of the buffer */

  const Babl       *format;  /* the pixel format used for pixels in this
                                buffer */
  const Babl  *soft_format;  /* the format the buffer pretends to be, might
                                be different from format */

  gint              shift_x; /* The relative offset of origins compared with */
  gint              shift_y; /* anchestral tile_storage buffer, during            */
                             /* construction relative to immediate source  */

  GeglRectangle     abyss;


  GeglSampler      *sampler; /* cached sampler for speeding up random
                                access interpolated fetches from the
                                buffer */
  const Babl       *sampler_format; /* the format of the cached sampler */

  GeglTileStorage  *tile_storage;

  gint              tile_width;
  gint              tile_height;
  gchar            *path;

  gint              lock_count;

  gchar            *alloc_stack_trace; /* Stack trace for allocation,
                                          useful for debugging */

  gpointer          backend;
};

struct _GeglBufferClass
{
  GeglTileHandlerClass parent_class;
};



gint              gegl_buffer_leaks       (void);

void              gegl_buffer_stats       (void);

void              gegl_buffer_save        (GeglBuffer          *buffer,
                                           const gchar         *path,
                                           const GeglRectangle *roi);


const gchar      *gegl_swap_dir           (void);


void              gegl_tile_cache_init    (void);

void              gegl_tile_cache_destroy (void);

GeglTileBackend * gegl_buffer_backend     (GeglBuffer *buffer);

gboolean          gegl_buffer_is_shared   (GeglBuffer *buffer);

gboolean          gegl_buffer_try_lock    (GeglBuffer *buffer);
gboolean          gegl_buffer_lock        (GeglBuffer *buffer);
gboolean          gegl_buffer_unlock      (GeglBuffer *buffer);
void              gegl_buffer_set_unlocked (GeglBuffer          *buffer,
                                            const GeglRectangle *rect,
                                            const Babl          *format,
                                            const void          *src,
                                            gint                 rowstride);
void              gegl_buffer_get_unlocked (GeglBuffer          *buffer,
                                            gdouble              scale,
                                            const GeglRectangle *rect,
                                            const Babl          *format,
                                            gpointer             dest_buf,
                                            gint                 rowstride);

GeglBuffer *
gegl_buffer_new_ram (const GeglRectangle *extent,
                     const Babl          *format);

void            gegl_buffer_sampler           (GeglBuffer     *buffer,
                                               gdouble         x,
                                               gdouble         y,
                                               gdouble         scale,
                                               gpointer        dest,
                                               const Babl     *format,
                                               gpointer        sampler);



/* the instance size of a GeglTile is a bit large, and should if possible be
 * trimmed down
 */
struct _GeglTile
{
 /* GObject          parent_instance;*/
  gint             ref_count;

  guchar          *data;        /* actual pixel data for tile, a linear buffer*/
  gint             size;        /* The size of the linear buffer */

  GeglTileStorage *tile_storage; /* the buffer from which this tile was
                                  * retrieved needed for the tile to be able to
                                  * store itself back (for instance when it is
                                  * unreffed for the last time)
                                  */
  gint             x, y, z;


  guint            rev;         /* this tile revision */
  guint            stored_rev;  /* what revision was we when we from tile_storage?
                                   (currently set to 1 when loaded from disk */

  gchar            lock;        /* number of times the tile is write locked
                                 * should in theory just have the values 0/1
                                 */


#ifdef GEGL_USE_TILE_MUTEX
  GMutex          *mutex;
#endif

  /* the shared list is a doubly linked circular list */
  GeglTile        *next_shared;
  GeglTile        *prev_shared;

  /* called when the tile is about to be destroyed */
  GDestroyNotify   destroy_notify;
  gpointer         destroy_notify_data;

  /* called when the tile has been unlocked which typically means tile
   * data has changed
   */
  GeglTileCallback unlock_notify;
  gpointer         unlock_notify_data;
};

void _gegl_buffer_drop_hot_tile (GeglBuffer *buffer);

#ifndef __GEGL_TILE_C
#define gegl_tile_get_data(tile)  ((guchar*)((tile)->data))
#endif // __GEGL_TILE_C


/* computes the positive integer remainder (also for negative dividends)
 */
#define GEGL_REMAINDER(dividend, divisor) \
                   (((dividend) < 0) ? \
                    (divisor) - 1 - ((-((dividend) + 1)) % (divisor)) : \
                    (dividend) % (divisor))

#define gegl_tile_offset(coordinate, stride) GEGL_REMAINDER((coordinate), (stride))

/* helper function to compute tile indices and offsets for coordinates
 * based on a tile stride (tile_width or tile_height)
 */
#define gegl_tile_indice(coordinate,stride) \
  (((coordinate) >= 0)?\
      (coordinate) / (stride):\
      ((((coordinate) + 1) /(stride)) - 1))


#endif
