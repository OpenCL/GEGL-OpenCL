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
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-buffer-types.h"
#include "gegl-buffer-iterator.h"
#include "gegl-buffer-private.h"
#include "gegl-tile-storage.h"
#include "gegl-utils.h"

typedef struct GeglBufferTileIterator
{
  GeglBuffer    *buffer;
  GeglRectangle  roi;     /* the rectangular region we're iterating over */
  GeglTile      *tile;    /* current tile */
  gpointer       data;    /* current tile's data */

  gint           col;     /* the column currently provided for */
  gint           row;     /* the row currently provided for */
  gboolean       write;
  GeglRectangle  subrect;    /* the subrect that intersected roi */
  gpointer       sub_data;   /* pointer to the subdata as indicated by subrect */
  gint           rowstride;  /* rowstride for tile, in bytes */

  gint           next_col; /* used internally */
  gint           next_row; /* used internally */
  gint           max_size; /* maximum data buffer needed, in bytes */
  GeglRectangle  roi2;     /* the rectangular subregion of data
                            * in the buffer represented by this scan.
                            */

} GeglBufferTileIterator;

#define GEGL_BUFFER_SCAN_COMPATIBLE   128   /* should be integrated into enum */
#define GEGL_BUFFER_FORMAT_COMPATIBLE 256   /* should be integrated into enum */

#define DEBUG_DIRECT 0

typedef struct GeglBufferIterators
{
  /* current region of interest */
  gint          length;             /* length of current data in pixels */
  gpointer      data[GEGL_BUFFER_MAX_ITERATORS]; 
  GeglRectangle roi[GEGL_BUFFER_MAX_ITERATORS];                 

  /* the following is private: */
  gint           iterators;
  gint           iteration_no;
  GeglRectangle  rect       [GEGL_BUFFER_MAX_ITERATORS];
  const Babl    *format     [GEGL_BUFFER_MAX_ITERATORS];
  GeglBuffer    *buffer     [GEGL_BUFFER_MAX_ITERATORS];
  guint          flags      [GEGL_BUFFER_MAX_ITERATORS];
  gpointer       buf        [GEGL_BUFFER_MAX_ITERATORS]; 
  GeglBufferTileIterator   i[GEGL_BUFFER_MAX_ITERATORS]; 
} GeglBufferIterators;


static void      gegl_buffer_tile_iterator_init (GeglBufferTileIterator *i,
                                                 GeglBuffer             *buffer,
                                                 GeglRectangle           roi,
                                                 gboolean                write);
static gboolean  gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i);

/*
 *  check whether iterations on two buffers starting from the given coordinates with
 *  the same width and height would be able to run parallell.
 */
static gboolean gegl_buffer_scan_compatible (GeglBuffer *bufferA,
                                             gint        xA,
                                             gint        yA,
                                             GeglBuffer *bufferB,
                                             gint        xB,
                                             gint        yB)
{ 
  if (bufferA->tile_storage->tile_width !=
      bufferB->tile_storage->tile_width)
    return FALSE;
  if (bufferA->tile_storage->tile_height !=
      bufferB->tile_storage->tile_height)
    return FALSE;
  if ( (abs((bufferA->shift_x+xA) - (bufferB->shift_x+xB))
        % bufferA->tile_storage->tile_width) != 0)
    return FALSE;
  if ( (abs((bufferA->shift_y+yA) - (bufferB->shift_y+yB))
        % bufferA->tile_storage->tile_height) != 0)
    return FALSE;
  return TRUE;
}

static void gegl_buffer_tile_iterator_init (GeglBufferTileIterator *i,
                                            GeglBuffer             *buffer,
                                            GeglRectangle           roi,
                                            gboolean                write)
{
  g_assert (i);
  memset (i, 0, sizeof (GeglBufferTileIterator));
  if (roi.width == 0 ||
      roi.height == 0)
    g_error ("eeek");
  i->buffer = buffer;
  i->roi = roi;
  i->next_row    = 0;
  i->next_col = 0;
  i->tile = NULL;
  i->col = 0;
  i->row = 0;
  i->write = write;
  i->max_size = i->buffer->tile_storage->tile_width *
                i->buffer->tile_storage->tile_height;
}

static gboolean
gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i)
{
  GeglBuffer *buffer   = i->buffer;
  gint  tile_width     = buffer->tile_storage->tile_width;
  gint  tile_height    = buffer->tile_storage->tile_height;
  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_x       = buffer->extent.x + buffer_shift_x;
  gint  buffer_y       = buffer->extent.y + buffer_shift_y;

  if (i->roi.width == 0 || i->roi.height == 0)
    return FALSE;

gulp:

  /* unref previously held tile */
  if (i->tile)
    {
      if (i->write && i->subrect.width == tile_width)
        {
          gegl_tile_unlock (i->tile);
        }
      gegl_tile_unref (i->tile);
      i->tile = NULL;
    }

  if (i->next_col < i->roi.width)
    { /* return tile on this row */
      gint tiledx = buffer_x + i->next_col;
      gint tiledy = buffer_y + i->next_row;
      gint offsetx = gegl_tile_offset (tiledx, tile_width);
      gint offsety = gegl_tile_offset (tiledy, tile_height);

        {
         i->subrect.x = offsetx;
         i->subrect.y = offsety;
         if (i->roi.width + offsetx - i->next_col < tile_width)
           i->subrect.width = (i->roi.width + offsetx - i->next_col) - offsetx;
         else
           i->subrect.width = tile_width - offsetx;

         if (i->roi.height + offsety - i->next_row < tile_height)
           i->subrect.height = (i->roi.height + offsety - i->next_row) - offsety;
         else
           i->subrect.height = tile_height - offsety;

         i->tile = gegl_tile_source_get_tile ((GeglTileSource *) (buffer),
                                               gegl_tile_indice (tiledx, tile_width),
                                               gegl_tile_indice (tiledy, tile_height),
                                               0);
         if (i->write && tile_width==i->subrect.width)
           {
             gegl_tile_lock (i->tile);
           }
         i->data = gegl_tile_get_data (i->tile);

         {
         gint bpp = babl_format_get_bytes_per_pixel (i->buffer->format);
         i->rowstride = bpp * tile_width;
         i->sub_data = (guchar*)(i->data) + bpp * (i->subrect.y * tile_width + i->subrect.x);
         }

         i->col = i->next_col;
         i->row = i->next_row;
         i->next_col += tile_width - offsetx;


         i->roi2.x      = i->roi.x + i->col;
         i->roi2.y      = i->roi.y + i->row;
         i->roi2.width  = i->subrect.width;
         i->roi2.height = i->subrect.height;

         return TRUE;
       }
    }
  else /* move down to next row */
    {
      gint tiledy;
      gint offsety;

      i->row = i->next_row;
      i->col = i->next_col;

      tiledy = buffer_y + i->next_row;
      offsety = gegl_tile_offset (tiledy, tile_height);

      i->next_row += tile_height - offsety;
      i->next_col=0;

      if (i->next_row < i->roi.height)
        {
          goto gulp; /* return the first tile in the next row */
        }
      return FALSE;
    }
  return FALSE;
}

#if DEBUG_DIRECT
static glong direct_read = 0;
static glong direct_write = 0;
static glong in_direct_read = 0;
static glong in_direct_write = 0;
#endif

gint
gegl_buffer_iterator_add (GeglBufferIterator  *iterator,
                          GeglBuffer          *buffer,
                          const GeglRectangle *roi,
                          const Babl          *format,
                          guint                flags)
{
  GeglBufferIterators *i = (gpointer)iterator;
  gint self = 0;
  if (i->iterators+1 > GEGL_BUFFER_MAX_ITERATORS)
    {
      g_error ("too many iterators (%i)", i->iterators+1);
    }

  if (i->iterators == 0) /* for sanity, we zero at init */
    {
      memset (i, 0, sizeof (GeglBufferIterators));
    }

  self = i->iterators++;

  if (!roi)
    roi = self==0?&(buffer->extent):&(i->rect[0]);
  i->rect[self]=*roi;

  /* XXX: if this buffer creation could be avoided, it would be a speedup */
  i->buffer[self]=gegl_buffer_create_sub_buffer (buffer, roi);
  if (format)
    i->format[self]=format;
  else
    i->format[self]=buffer->format;
  i->flags[self]=flags;

  if (self==0) /* The first buffer which is always scan aligned */
    {
      i->flags[self] |= GEGL_BUFFER_SCAN_COMPATIBLE;
      gegl_buffer_tile_iterator_init (&i->i[self], i->buffer[self], i->rect[self], ((i->flags[self] & GEGL_BUFFER_WRITE) != 0) );
    }
  else
    {
      /* we make all subsequently added iterators share the width and height of the first one */
      i->rect[self].width = i->rect[0].width;
      i->rect[self].height = i->rect[0].height;

      if (gegl_buffer_scan_compatible (i->buffer[0], i->rect[0].x, i->rect[0].y,
                                       i->buffer[self], i->rect[self].x, i->rect[self].y))
        {
          i->flags[self] |= GEGL_BUFFER_SCAN_COMPATIBLE;
          gegl_buffer_tile_iterator_init (&i->i[self], i->buffer[self], i->rect[self], ((i->flags[self] & GEGL_BUFFER_WRITE) != 0));
        }
    }

  i->buf[self] = NULL;
  
  if (i->format[self] == i->buffer[self]->format)
    {
      i->flags[self] |= GEGL_BUFFER_FORMAT_COMPATIBLE;
    }
  return self;
}

/* FIXME: we are currently leaking this buf pool, it should be
 * freeing it when gegl is uninitialized
 */

typedef struct BufInfo {
  gint     size;
  gint     used;  /* if this buffer is currently allocated */
  gpointer buf;
} BufInfo;

static GArray *buf_pool = NULL;

#if ENABLE_MT
static GStaticMutex pool_mutex = G_STATIC_MUTEX_INIT;
#endif

static gpointer iterator_buf_pool_get (gint size)
{
  gint i;
#if ENABLE_MT
  g_static_mutex_lock (&pool_mutex);
#endif

  if (G_UNLIKELY (!buf_pool))
    {
      buf_pool = g_array_new (TRUE, TRUE, sizeof (BufInfo));
    }
  for (i=0; i<buf_pool->len; i++)
    {
      BufInfo *info = &g_array_index (buf_pool, BufInfo, i);
      if (info->size >= size && info->used == 0)
        {
          info->used ++;
#if ENABLE_MT
          g_static_mutex_unlock (&pool_mutex);
#endif
          return info->buf;
        }
    }
  {
    BufInfo info = {0, 1, NULL};
    info.size = size;
    info.buf = gegl_malloc (size);
    g_array_append_val (buf_pool, info);
#if ENABLE_MT
    g_static_mutex_unlock (&pool_mutex);
#endif
    return info.buf;
  }
}

static void iterator_buf_pool_release (gpointer buf)
{
  gint i;
#if ENABLE_MT
  g_static_mutex_lock (&pool_mutex);
#endif
  for (i=0; i<buf_pool->len; i++)
    {
      BufInfo *info = &g_array_index (buf_pool, BufInfo, i);
      if (info->buf == buf)
        {
          info->used --;
#if ENABLE_MT
          g_static_mutex_unlock (&pool_mutex);
#endif
          return;
        }
    }
  g_assert (0);
#if ENABLE_MT
  g_static_mutex_unlock (&pool_mutex);
#endif
}

static void ensure_buf (GeglBufferIterators *i, gint no)
{
  if (i->buf[no]==NULL)
    i->buf[no] = iterator_buf_pool_get (babl_format_get_bytes_per_pixel (i->format[no]) *
                                        i->i[0].max_size);
}

gboolean gegl_buffer_iterator_next     (GeglBufferIterator *iterator)
{
  GeglBufferIterators *i = (gpointer)iterator;
  gboolean result = FALSE;
  gint no;

  if (i->buf[0] == (void*)0xdeadbeef)
    g_error ("%s called on finished buffer iterator", G_STRFUNC);
  if (i->iteration_no == 0)
    {
#if ENABLE_MT
      for (no=0; no<i->iterators;no++)
        {
          gint j;
          gboolean found = FALSE;
          for (j=0; j<no; j++)
            if (i->buffer[no]==i->buffer[j])
              {
                found = TRUE;
                break;
              }
          if (!found)
            gegl_buffer_lock (i->buffer[no]);
        }
#endif
    }
  else
    {
      /* complete pending write work */
      for (no=0; no<i->iterators;no++)
        {
          if (i->flags[no] & GEGL_BUFFER_WRITE)
            {

              if (i->flags[no] & GEGL_BUFFER_SCAN_COMPATIBLE &&
                  i->flags[no] & GEGL_BUFFER_FORMAT_COMPATIBLE &&
                  i->roi[no].width == i->i[no].buffer->tile_storage->tile_width && (i->flags[no] & GEGL_BUFFER_FORMAT_COMPATIBLE))
                {
                   /* direct access */
#if DEBUG_DIRECT
                   direct_write += i->roi[no].width * i->roi[no].height;
#endif
                }
              else
                {
#if DEBUG_DIRECT
                  in_direct_write += i->roi[no].width * i->roi[no].height;
#endif

                  ensure_buf (i, no);

  /* XXX: should perhaps use _set_unlocked, and keep the lock in the
   * iterator.
   */
                  gegl_buffer_set (i->buffer[no], &(i->roi[no]), i->format[no], i->buf[no], GEGL_AUTO_ROWSTRIDE);
                }
            }
        }
    }

  g_assert (i->iterators > 0);

  /* then we iterate all */
  for (no=0; no<i->iterators;no++)
    {
      if (i->flags[no] & GEGL_BUFFER_SCAN_COMPATIBLE)
        {
          gboolean res;
          res = gegl_buffer_tile_iterator_next (&i->i[no]);
          if (no == 0)
            {
              result = res;
            }
          i->roi[no] = i->i[no].roi2;

          /* since they were scan compatible this should be true */
          if (res != result)
            {
              g_print ("%i==%i != 0==%i\n", no, res, result);
             } 
          g_assert (res == result);

          if ((i->flags[no] & GEGL_BUFFER_FORMAT_COMPATIBLE) && 
              i->roi[no].width == i->i[no].buffer->tile_storage->tile_width 
           )
            {
              /* direct access */
              i->data[no]=i->i[no].sub_data;
#if DEBUG_DIRECT
              direct_read += i->roi[no].width * i->roi[no].height;
#endif
            }
          else
            {
              ensure_buf (i, no);

              if (i->flags[no] & GEGL_BUFFER_READ)
                {
                  gegl_buffer_get_unlocked (i->buffer[no], 1.0, &(i->roi[no]), i->format[no], i->buf[no], GEGL_AUTO_ROWSTRIDE);
                }

              i->data[no]=i->buf[no];
#if DEBUG_DIRECT
              in_direct_read += i->roi[no].width * i->roi[no].height;
#endif
            }
        }
      else
        { 
          /* we copy the roi from iterator 0  */
          i->roi[no] = i->roi[0];
          i->roi[no].x += (i->rect[no].x-i->rect[0].x);
          i->roi[no].y += (i->rect[no].y-i->rect[0].y);

          ensure_buf (i, no);

          if (i->flags[no] & GEGL_BUFFER_READ)
            {
              gegl_buffer_get_unlocked (i->buffer[no], 1.0, &(i->roi[no]), i->format[no], i->buf[no], GEGL_AUTO_ROWSTRIDE);
            }
          i->data[no]=i->buf[no];

#if DEBUG_DIRECT
          in_direct_read += i->roi[no].width * i->roi[no].height;
#endif
        }
      i->length = i->roi[no].width * i->roi[no].height;
    }

  i->iteration_no++;

  if (result == FALSE)
    {

#if ENABLE_MT
      for (no=0; no<i->iterators;no++)
        {
          gint j;
          gboolean found = FALSE;
          for (j=0; j<no; j++)
            if (i->buffer[no]==i->buffer[j])
              {
                found = TRUE;
                break;
              }
          if (!found)
            gegl_buffer_unlock (i->buffer[no]);
        }
#endif

      for (no=0; no<i->iterators;no++)
        {
          if (i->buf[no])
            iterator_buf_pool_release (i->buf[no]);
          i->buf[no]=NULL;
          g_object_unref (i->buffer[no]);
        }
#if DEBUG_DIRECT
      g_print ("%f %f\n", (100.0*direct_read/(in_direct_read+direct_read)),
                           100.0*direct_write/(in_direct_write+direct_write));
#endif
      i->buf[0]=(void*)0xdeadbeef;
      g_slice_free (GeglBufferIterators, i);
    }


  return result;
}

GeglBufferIterator *gegl_buffer_iterator_new (GeglBuffer          *buffer,
                                              const GeglRectangle *roi, 
                                              const Babl          *format,
                                              guint                flags)
{
  GeglBufferIterator *i = (gpointer)g_slice_new0 (GeglBufferIterators);
  gegl_buffer_iterator_add (i, buffer, roi, format, flags);
  return i;
}
