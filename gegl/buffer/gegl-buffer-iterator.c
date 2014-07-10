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
 *           2013 Daniel Sabo
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
#include "gegl-buffer-iterator-private.h"
#include "gegl-buffer-private.h"
#include "gegl-buffer-cl-cache.h"
#include "gegl-config.h"

#define GEGL_ITERATOR_INCOMPATIBLE (1 << 2)

typedef enum {
  GeglIteratorState_Start,
  GeglIteratorState_InTile,
  GeglIteratorState_InRows,
  GeglIteratorState_Linear,
  GeglIteratorState_Invalid,
} GeglIteratorState;

typedef enum {
  GeglIteratorTileMode_Invalid,
  GeglIteratorTileMode_DirectTile,
  GeglIteratorTileMode_LinearTile,
  GeglIteratorTileMode_GetBuffer,
  GeglIteratorTileMode_Empty,
} GeglIteratorTileMode;

typedef struct _SubIterState {
  GeglRectangle        full_rect; /* The entire area we are iterating over */
  GeglBuffer          *buffer;
  GeglAccessMode       access_mode;
  GeglAbyssPolicy      abyss_policy;
  const Babl          *format;
  gint                 format_bpp;
  GeglIteratorTileMode current_tile_mode;
  gint                 row_stride;
  GeglRectangle        real_roi;
  gint                 level;
  /* Direct data members */
  GeglTile            *current_tile;
  /* Indirect data members */
  gpointer             real_data;
  /* Linear data members */
  GeglTile            *linear_tile;
  gpointer             linear;
} SubIterState;

struct _GeglBufferIteratorPriv
{
  gint              num_buffers;
  GeglIteratorState state;
  GeglRectangle     origin_tile;
  gint              remaining_rows;
  SubIterState      sub_iter[GEGL_BUFFER_MAX_ITERATORS];
};

static gboolean threaded = TRUE;

GeglBufferIterator *
gegl_buffer_iterator_empty_new (void)
{
  GeglBufferIterator *iter = g_slice_new (GeglBufferIterator);
  iter->priv               = g_slice_new (GeglBufferIteratorPriv);

  iter->priv->num_buffers = 0;
  iter->priv->state       = GeglIteratorState_Start;

  threaded = gegl_config_threads () > 1;

  return iter;
}

GeglBufferIterator *
gegl_buffer_iterator_new (GeglBuffer          *buf,
                          const GeglRectangle *roi,
                          gint                 level,
                          const Babl          *format,
                          GeglAccessMode       access_mode,
                          GeglAbyssPolicy      abyss_policy)
{
  GeglBufferIterator *iter = gegl_buffer_iterator_empty_new ();

  gegl_buffer_iterator_add (iter, buf, roi, level, format,
                            access_mode, abyss_policy);

  return iter;
}

int
gegl_buffer_iterator_add (GeglBufferIterator  *iter,
                          GeglBuffer          *buf,
                          const GeglRectangle *roi,
                          gint                 level,
                          const Babl          *format,
                          GeglAccessMode       access_mode,
                          GeglAbyssPolicy      abyss_policy)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  int                     index;
  SubIterState           *sub;

  g_return_val_if_fail (priv->num_buffers < GEGL_BUFFER_MAX_ITERATORS, 0);


  index = priv->num_buffers++;
  sub = &priv->sub_iter[index];

  if (!format)
    format = gegl_buffer_get_format (buf);

  if (!roi)
    roi = &buf->extent;

  sub->buffer       = buf;
  sub->full_rect    = *roi;

  if (level)
  {
    sub->full_rect.x >>= level;
    sub->full_rect.y >>= level;
    sub->full_rect.width >>= level;
    sub->full_rect.height >>= level;
  }

  sub->access_mode  = access_mode;
  sub->abyss_policy = abyss_policy;
  sub->current_tile = NULL;
  sub->real_data    = NULL;
  sub->linear_tile  = NULL;
  sub->format       = format;
  sub->format_bpp   = babl_format_get_bytes_per_pixel (format);
  sub->level        = level;

  if (index > 0)
    {
      priv->sub_iter[index].full_rect.width  = priv->sub_iter[0].full_rect.width;
      priv->sub_iter[index].full_rect.height = priv->sub_iter[0].full_rect.height;
    }

  //if (level != 0)
  //  g_warning ("iterator level != 0");

  return index;
}

static void
release_tile (GeglBufferIterator *iter,
              int index)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->current_tile_mode == GeglIteratorTileMode_DirectTile)
    {
      if (sub->access_mode & GEGL_ACCESS_WRITE)
        gegl_tile_unlock (sub->current_tile);
      gegl_tile_unref (sub->current_tile);

      sub->current_tile = NULL;
      iter->data[index] = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_LinearTile)
    {
      sub->current_tile = NULL;
      iter->data[index] = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_GetBuffer)
    {
      if (sub->access_mode & GEGL_ACCESS_WRITE)
        {
          gegl_buffer_set_unlocked_no_notify (sub->buffer,
                                              &sub->real_roi,
                                              sub->level,
                                              sub->format,
                                              sub->real_data,
                                              GEGL_AUTO_ROWSTRIDE);
        }

      gegl_free (sub->real_data);
      sub->real_data = NULL;
      iter->data[index] = NULL;

      sub->current_tile_mode = GeglIteratorTileMode_Empty;
    }
  else if (sub->current_tile_mode == GeglIteratorTileMode_Empty)
    {
      return;
    }
  else
    {
      g_warn_if_reached ();
    }
}

static void
retile_subs (GeglBufferIterator *iter,
             int                 x,
             int                 y)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  GeglRectangle real_roi;
  int index;

  int shift_x = priv->origin_tile.x;
  int shift_y = priv->origin_tile.y;

  int tile_x = gegl_tile_indice (x + shift_x, priv->origin_tile.width);
  int tile_y = gegl_tile_indice (y + shift_y, priv->origin_tile.height);

  /* Reset tile size */
  real_roi.x = (tile_x * priv->origin_tile.width)  - shift_x;
  real_roi.y = (tile_y * priv->origin_tile.height) - shift_y;
  real_roi.width  = priv->origin_tile.width;
  real_roi.height = priv->origin_tile.height;

  /* Trim tile down to the iteration roi */
  gegl_rectangle_intersect (&iter->roi[0], &real_roi, &priv->sub_iter[0].full_rect);
  priv->sub_iter[0].real_roi = iter->roi[0];

  for (index = 1; index < priv->num_buffers; index++)
    {
      SubIterState *lead_sub = &priv->sub_iter[0];
      SubIterState *sub = &priv->sub_iter[index];

      int roi_offset_x = sub->full_rect.x - lead_sub->full_rect.x;
      int roi_offset_y = sub->full_rect.y - lead_sub->full_rect.y;

      iter->roi[index].x = iter->roi[0].x + roi_offset_x;
      iter->roi[index].y = iter->roi[0].y + roi_offset_y;
      iter->roi[index].width  = iter->roi[0].width;
      iter->roi[index].height = iter->roi[0].height;
      sub->real_roi = iter->roi[index];
    }
}

static gboolean
initialize_rects (GeglBufferIterator *iter)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[0];

  retile_subs (iter, sub->full_rect.x, sub->full_rect.y);

  return TRUE;
}

static gboolean
increment_rects (GeglBufferIterator *iter)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[0];

  /* Next tile in row */
  int x = iter->roi[0].x + iter->roi[0].width;
  int y = iter->roi[0].y;

  if (x >= sub->full_rect.x + sub->full_rect.width)
    {
      /* Next row */
      x  = sub->full_rect.x;
      y += iter->roi[0].height;

      if (y >= sub->full_rect.y + sub->full_rect.height)
        {
          /* All done */
          return FALSE;
        }
    }

  retile_subs (iter, x, y);

  return TRUE;
}

static void
get_tile (GeglBufferIterator *iter,
          int        index)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  GeglBuffer *buf = priv->sub_iter[index].buffer;

  if (sub->linear_tile)
    {
      sub->current_tile = sub->linear_tile;

      sub->real_roi = buf->extent;

      sub->current_tile_mode = GeglIteratorTileMode_LinearTile;
    }
  else
    {
      int shift_x = buf->shift_x; // XXX: affect by level?
      int shift_y = buf->shift_y;

      int tile_width  = buf->tile_width;
      int tile_height = buf->tile_height;

      int tile_x = gegl_tile_indice (iter->roi[index].x + shift_x, tile_width);
      int tile_y = gegl_tile_indice (iter->roi[index].y + shift_y, tile_height);

      sub->current_tile = gegl_buffer_get_tile (buf, tile_x, tile_y, sub->level);

      if (sub->access_mode & GEGL_ACCESS_WRITE)
        gegl_tile_lock (sub->current_tile);

      sub->real_roi.x = (tile_x * tile_width)  - shift_x;
      sub->real_roi.y = (tile_y * tile_height) - shift_y;
      sub->real_roi.width  = tile_width;
      sub->real_roi.height = tile_height;

      sub->current_tile_mode = GeglIteratorTileMode_DirectTile;
    }

  sub->row_stride = buf->tile_width * sub->format_bpp;

  iter->data[index] = gegl_tile_get_data (sub->current_tile);
}

static void
get_indirect (GeglBufferIterator *iter,
              int        index)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  sub->real_data = gegl_malloc (sub->format_bpp * sub->real_roi.width * sub->real_roi.height);

  if (sub->access_mode & GEGL_ACCESS_READ)
    {
      gegl_buffer_get_unlocked (sub->buffer, sub->level?1.0/(1<<sub->level):1.0, &sub->real_roi, sub->format, sub->real_data,
                                GEGL_AUTO_ROWSTRIDE, sub->abyss_policy);
    }

  sub->row_stride = sub->real_roi.width * sub->format_bpp;

  iter->data[index] = sub->real_data;
  sub->current_tile_mode = GeglIteratorTileMode_GetBuffer;
}

static gboolean
needs_indirect_read (GeglBufferIterator *iter,
                     int        index)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->access_mode & GEGL_ITERATOR_INCOMPATIBLE)
    return TRUE;

  /* Needs abyss generation */
  if (!gegl_rectangle_contains (&sub->buffer->abyss, &iter->roi[index]))
    return TRUE;

  return FALSE;
}

static gboolean
needs_rows (GeglBufferIterator *iter,
            int        index)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState           *sub  = &priv->sub_iter[index];

  if (sub->current_tile_mode == GeglIteratorTileMode_GetBuffer)
   return FALSE;

  if (iter->roi[index].width  != sub->buffer->tile_width ||
      iter->roi[index].height != sub->buffer->tile_height)
    return TRUE;

  return FALSE;
}

/* Do the final setup of the iter struct */
static void
prepare_iteration (GeglBufferIterator *iter)
{
  int index;
  GeglBufferIteratorPriv *priv = iter->priv;
  gint origin_offset_x;
  gint origin_offset_y;

  /* Set up the origin tile */
  /* FIXME: Pick the most compatable buffer, not just the first */
  {
    GeglBuffer *buf = priv->sub_iter[0].buffer;

    priv->origin_tile.x      = buf->shift_x;
    priv->origin_tile.y      = buf->shift_y;
    priv->origin_tile.width  = buf->tile_width;
    priv->origin_tile.height = buf->tile_height;

    origin_offset_x = buf->shift_x + priv->sub_iter[0].full_rect.x;
    origin_offset_y = buf->shift_y + priv->sub_iter[0].full_rect.y;
  }

  for (index = 0; index < priv->num_buffers; index++)
    {
      SubIterState *sub = &priv->sub_iter[index];
      GeglBuffer *buf   = sub->buffer;

      gint current_offset_x = buf->shift_x + priv->sub_iter[index].full_rect.x;
      gint current_offset_y = buf->shift_y + priv->sub_iter[index].full_rect.y;

      /* Format converison needed */
      if (gegl_buffer_get_format (sub->buffer) != sub->format)
        sub->access_mode |= GEGL_ITERATOR_INCOMPATIBLE;
      /* Incompatable tiles */
      else if ((priv->origin_tile.width  != buf->tile_width) ||
               (priv->origin_tile.height != buf->tile_height) ||
               (abs(origin_offset_x - current_offset_x) % priv->origin_tile.width != 0) ||
               (abs(origin_offset_y - current_offset_y) % priv->origin_tile.height != 0))
        {
          /* Check if the buffer is a linear buffer */
          if ((buf->extent.x      == -buf->shift_x) &&
              (buf->extent.y      == -buf->shift_y) &&
              (buf->extent.width  == buf->tile_width) &&
              (buf->extent.height == buf->tile_height))
            {
              sub->linear_tile = gegl_buffer_get_tile (sub->buffer, 0, 0, 0);

              if (sub->access_mode & GEGL_ACCESS_WRITE)
                gegl_tile_lock (sub->linear_tile);
            }
          else
            sub->access_mode |= GEGL_ITERATOR_INCOMPATIBLE;
        }

      gegl_buffer_lock (sub->buffer);
    }
}

static void
load_rects (GeglBufferIterator *iter)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  GeglIteratorState next_state = GeglIteratorState_InTile;
  int index;

  for (index = 0; index < priv->num_buffers; index++)
    {
      if (needs_indirect_read (iter, index))
        get_indirect (iter, index);
      else
        get_tile (iter, index);

      if ((next_state != GeglIteratorState_InRows) && needs_rows (iter, index))
        {
          next_state = GeglIteratorState_InRows;
        }
    }

  if (next_state == GeglIteratorState_InRows)
    {
      if (iter->roi[0].height == 1)
        next_state = GeglIteratorState_InTile;

      priv->remaining_rows = iter->roi[0].height - 1;

      for (index = 0; index < priv->num_buffers; index++)
        {
          SubIterState *sub = &priv->sub_iter[index];

          int offset_x = iter->roi[index].x - sub->real_roi.x;
          int offset_y = iter->roi[index].y - sub->real_roi.y;

          iter->data[index] = ((char *)iter->data[index]) + (offset_y * sub->row_stride + offset_x * sub->format_bpp);
          iter->roi[index].height = 1;
        }
    }

  iter->length = iter->roi[0].width * iter->roi[0].height;
  priv->state  = next_state;
}

void
gegl_buffer_iterator_stop (GeglBufferIterator *iter)
{
  int index;
  GeglBufferIteratorPriv *priv = iter->priv;
  priv->state = GeglIteratorState_Invalid;

  for (index = 0; index < priv->num_buffers; index++)
    {
      SubIterState *sub = &priv->sub_iter[index];

      if (sub->current_tile_mode != GeglIteratorTileMode_Empty)
        release_tile (iter, index);

      if (sub->linear_tile)
        {
          if (sub->access_mode & GEGL_ACCESS_WRITE)
            gegl_tile_unlock (sub->linear_tile);
          gegl_tile_unref (sub->linear_tile);
        }

      gegl_buffer_unlock (sub->buffer);

      if (sub->access_mode & GEGL_ACCESS_WRITE)
        gegl_buffer_emit_changed_signal (sub->buffer, &sub->full_rect);
    }

  g_slice_free (GeglBufferIteratorPriv, iter->priv);
  g_slice_free (GeglBufferIterator, iter);
}

static void linear_shortcut (GeglBufferIterator *iter)
{
  GeglBufferIteratorPriv *priv = iter->priv;
  SubIterState *sub0 = &priv->sub_iter[0];
  int index;
  int re_use_first[16] = {0,};

  for (index = priv->num_buffers-1; index >=0 ; index--)
  {
    SubIterState *sub = &priv->sub_iter[index];

    sub->real_roi    = sub0->full_rect;
    iter->roi[index] = sub0->full_rect;
    iter->length = iter->roi[0].width * iter->roi[0].height;

    if (priv->sub_iter[0].buffer == sub->buffer && index != 0)
    {
      if (sub->format == priv->sub_iter[0].format)
        re_use_first[index] = 1;
    }

    if (!re_use_first[index])
    {
      gegl_buffer_lock (sub->buffer);
      if (index == 0)
        get_tile (iter, index);
      else
      {
        if (sub->buffer->tile_width == sub->buffer->extent.width 
            && sub->buffer->tile_height == sub->buffer->extent.height
            && sub->buffer->extent.x == iter->roi[index].x
            && sub->buffer->extent.y == iter->roi[index].y)
        {
          get_tile (iter, index);
        }
        else
          get_indirect (iter, index);
      }
    }
  }
  for (index = 1; index < priv->num_buffers; index++)
  {
    if (re_use_first[index])
    {
      g_print ("!\n");
      iter->data[index] = iter->data[0];
    }
  }

  priv->state = GeglIteratorState_Invalid; /* quit on next iterator_next */
}

gboolean
gegl_buffer_iterator_next (GeglBufferIterator *iter)
{
  GeglBufferIteratorPriv *priv = iter->priv;

  if (priv->state == GeglIteratorState_Start)
    {
      int index;
      GeglBuffer *primary = priv->sub_iter[0].buffer;
      if (primary->tile_width == primary->extent.width 
          && primary->tile_height == primary->extent.height 
          && priv->sub_iter[0].full_rect.width == primary->tile_width 
          && priv->sub_iter[0].full_rect.height == primary->tile_height
          && priv->sub_iter[0].full_rect.x == primary->extent.x
          && priv->sub_iter[0].full_rect.y == primary->extent.y
          && priv->sub_iter[0].buffer->extent.x == iter->roi[0].x
          && priv->sub_iter[0].buffer->extent.y == iter->roi[0].y
          && FALSE) /* XXX: conditions are not strict enough, GIMPs TIFF
                       plug-in fails; but GEGLs buffer test suite passes */
      {
        if (gegl_cl_is_accelerated ())
          for (index = 0; index < priv->num_buffers; index++)
            {
              SubIterState *sub = &priv->sub_iter[index];
              gegl_buffer_cl_cache_flush (sub->buffer, &sub->full_rect);
            }
        linear_shortcut (iter);
        return TRUE;
      }

      prepare_iteration (iter);

      if (gegl_cl_is_accelerated ())
        for (index = 0; index < priv->num_buffers; index++)
          {
            SubIterState *sub = &priv->sub_iter[index];
            gegl_buffer_cl_cache_flush (sub->buffer, &sub->full_rect);
          }

      initialize_rects (iter);

      load_rects (iter);

      return TRUE;
    }
  else if (priv->state == GeglIteratorState_InRows)
    {
      int index;

      for (index = 0; index < priv->num_buffers; index++)
        {
          iter->data[index]   = ((char *)iter->data[index]) + priv->sub_iter[index].row_stride;
          iter->roi[index].y += 1;
        }

      priv->remaining_rows -= 1;

      if (priv->remaining_rows == 0)
        priv->state = GeglIteratorState_InTile;

      return TRUE;
    }
  else if (priv->state == GeglIteratorState_InTile)
    {
      int index;

      for (index = 0; index < priv->num_buffers; index++)
        {
          release_tile (iter, index);
        }

      if (increment_rects (iter) == FALSE)
        {
          gegl_buffer_iterator_stop (iter);
          return FALSE;
        }

      load_rects (iter);

      return TRUE;
    }
  else
    {
      gegl_buffer_iterator_stop (iter);
      return FALSE;
    }
}
