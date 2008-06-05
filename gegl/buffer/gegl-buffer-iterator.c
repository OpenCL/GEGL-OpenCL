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
#include <string.h>
#include <math.h>

#include <glib-object.h>
#include <glib/gprintf.h>

#include "gegl-types.h"
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
} GeglBufferTileIterator;

typedef struct GeglBufferScanIterator {
  GeglBufferTileIterator tile_iterator; /* must be first member since we do
                                           casting */
  gint                   max_size; /* maximum data buffer needed, in bytes */
  gint                   length;   /* how long the current scan is in pixels */
  gpointer               data;     /* the current scans data */
  GeglRectangle          roi;      /* the rectangular subregion of data
                                    * in the buffer represented by this scan.
                                    */

  gint                   row;       /* used internally */
  gint                   next_row;  /* used internally */
} GeglBufferScanIterator;

typedef struct GeglBufferIterators
{
  GeglRectangle  roi;                /* current region of interest */
  gint           length;             /* length of current data in pixels */
  gpointer       data       [GEGL_BUFFER_MAX_ITERATORS]; 

  gint           iterators;
  /* the following is private: */
  gint           iteration_no;
  GeglRectangle  rect       [GEGL_BUFFER_MAX_ITERATORS];
  const Babl    *format     [GEGL_BUFFER_MAX_ITERATORS];
  Babl          *fish       [GEGL_BUFFER_MAX_ITERATORS];
  Babl          *fish_to    [GEGL_BUFFER_MAX_ITERATORS];
  GeglBuffer    *buffer     [GEGL_BUFFER_MAX_ITERATORS];
  guint          flags      [GEGL_BUFFER_MAX_ITERATORS];
  gpointer       buf        [GEGL_BUFFER_MAX_ITERATORS]; 
  gboolean       compatible [GEGL_BUFFER_MAX_ITERATORS];
  GeglBufferScanIterator   i[GEGL_BUFFER_MAX_ITERATORS]; 
} GeglBufferIterators;


static void      gegl_buffer_tile_iterator_init (GeglBufferTileIterator *i,
                                                 GeglBuffer             *buffer,
                                                 GeglRectangle           roi,
                                                 gboolean                write);
static gboolean  gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i);
static void      gegl_buffer_scan_iterator_init (GeglBufferScanIterator *i,
                                                 GeglBuffer             *buffer,
                                                 GeglRectangle           roi,
                                                 gboolean                write);
static gboolean  gegl_buffer_scan_iterator_next (GeglBufferScanIterator *i);
static gboolean  gegl_buffer_scan_compatible    (GeglBuffer             *input,
                                                 gint                    x0,
                                                 gint                    y0,
                                                 GeglBuffer             *output,
                                                 gint                    x1,
                                                 gint                    y1);


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
}

static void gegl_buffer_scan_iterator_init (GeglBufferScanIterator *i,
                                            GeglBuffer             *buffer,
                                            GeglRectangle           roi,
                                            gboolean                write)
{
  GeglBufferTileIterator *tile_i = (GeglBufferTileIterator*)i;
  g_assert (i);
  memset (i, 0, sizeof (GeglBufferScanIterator));
  gegl_buffer_tile_iterator_init (tile_i, buffer, roi, write);
  i->max_size = tile_i->buffer->tile_storage->tile_width *
                tile_i->buffer->tile_storage->tile_height *
                tile_i->buffer->format->format.bytes_per_pixel;
  i->row = 0;
  if (write)
    gegl_buffer_lock (buffer);
}

static gboolean
gegl_buffer_scan_iterator_next (GeglBufferScanIterator *i)
{
  GeglBufferTileIterator *tile_i = (GeglBufferTileIterator*)i;

  if (tile_i->tile==NULL)
    {
      gulp:
      if (!gegl_buffer_tile_iterator_next (tile_i))
        return FALSE; /* this is where the scan iterator terminates */

      i->length = tile_i->subrect.width;
      i->next_row = 0;
      i->row = 0;

      i->roi.x      = tile_i->roi.x + tile_i->col;
      i->roi.y      = tile_i->roi.y+ tile_i->row+ i->row;
      i->roi.width  = tile_i->subrect.width;
      i->roi.height = tile_i->subrect.height;
    }
  /* we should now have a valid tile */

  if (i->next_row < tile_i->subrect.height)
    {
      if (tile_i->subrect.width == tile_i->buffer->tile_storage->tile_width)
        /* the entire contents of the tile can be expressed as one long scan */
        {
          i->length = tile_i->subrect.width * tile_i->subrect.height;
          i->data = tile_i->sub_data;
          i->row = 0;
          i->next_row = tile_i->subrect.height;
          return TRUE;
        }
      else 
        /* iterate thorugh the scanlines in the subrect */
        {
          guchar *data = tile_i->sub_data;

          i->roi.height = 1;

          i->data = data + i->next_row * tile_i->rowstride;
          i->row = i->next_row;
          i->next_row ++;
          return TRUE;
        }
    }
  else
    { /* we're done with that tile go get another one if possible */
      goto gulp;
    }

  return FALSE;
}


static gboolean gegl_buffer_scan_compatible (GeglBuffer *input,
                                             gint        x0,
                                             gint        y0,
                                             GeglBuffer *output,
                                             gint        x1,
                                             gint        y1)
{ 
  if (input->tile_storage->tile_width !=
      output->tile_storage->tile_width)
    return FALSE;
  if (input->tile_storage->tile_height !=
      output->tile_storage->tile_height)
    return FALSE;
  if (input->shift_x !=
      output->shift_x)
    return FALSE;
  if (input->shift_y !=
      output->shift_y)
    return FALSE;
  if (x0!=x1 || y0!=y1)
    return FALSE;
  if ( (abs(x0 - x1) % input->tile_storage->tile_width) != 0)
    return FALSE;
  if ( (abs(y0 - y1) % input->tile_storage->tile_height) != 0)
    return FALSE;
  return TRUE;
}

static gboolean
gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i)
{
  GeglBuffer *buffer = i->buffer;
  gint  tile_width   = buffer->tile_storage->tile_width;
  gint  tile_height  = buffer->tile_storage->tile_height;
  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_x       = buffer->extent.x + buffer_shift_x;
  gint  buffer_y       = buffer->extent.y + buffer_shift_y;

  if (i->roi.width == 0 || i->roi.height == 0)
    return FALSE;

gulp:

  /* unref previous held tile */
  if (i->tile)
    {
      if (i->write)
        {
          gegl_tile_unlock (i->tile);
        }
      g_object_unref (i->tile);
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
         if (i->write)
           {
             gegl_tile_lock (i->tile);
           }
         i->data = gegl_tile_get_data (i->tile);

         {
         gint bpp = i->buffer->format->format.bytes_per_pixel;
         i->rowstride = bpp * tile_width;
         i->sub_data = (guchar*)(i->data) + bpp * (i->subrect.y * tile_width + i->subrect.x);
         }

         i->col = i->next_col;
         i->row = i->next_row;
         i->next_col += tile_width - offsetx;

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


gint
gegl_buffer_iterator_add (GeglBufferIterator  *iterator,
                          GeglBuffer          *buffer,
                          GeglRectangle        roi,
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

  i->rect[self]=roi;
  i->buffer[self]=buffer;
  if (format)
    i->format[self]=format;
  else
    i->format[self]=buffer->format;
  i->flags[self]=flags;

  if (self==0) /* The first buffer which is always scan aligned */
    {
      i->compatible[self]= TRUE;
      gegl_buffer_scan_iterator_init (&i->i[self], i->buffer[self], i->rect[self], ((i->flags[self] & GEGL_BUFFER_WRITE) != 0) );
    }
  else
    {
      i->rect[self].width = i->rect[0].width;
      i->rect[self].height = i->rect[0].height;
      if (gegl_buffer_scan_compatible (i->buffer[0], i->rect[0].x, i->rect[0].y,
                                       i->buffer[self], i->rect[self].x, i->rect[self].y))
        {
          i->compatible[self] = TRUE;
          gegl_buffer_scan_iterator_init (&i->i[self], i->buffer[self], i->rect[self], ((i->flags[self] & GEGL_BUFFER_WRITE) != 0));
        }
      else
        {
          i->buf[self] = gegl_malloc (i->format[self]->format.bytes_per_pixel *
                                      i->i[0].max_size);
        }
    }

  if (i->format[self] != i->buffer[self]->format ||
      i->compatible[self] == FALSE)
    {
      i->buf[self] = gegl_malloc (i->format[self]->format.bytes_per_pixel *
                                  i->i[0].max_size);
      if (i->flags[self] & GEGL_BUFFER_READ)
        i->fish[self] = babl_fish (i->buffer[self]->format, i->format[self]);
      if (i->flags[self] & GEGL_BUFFER_WRITE)
        i->fish_to[self] = babl_fish (i->format[self], i->buffer[self]->format);
     }

  return self;
}

gboolean gegl_buffer_iterator_next     (GeglBufferIterator *iterator)
{
  GeglBufferIterators *i = (gpointer)iterator;
  gboolean result = FALSE;
  gint no;
  /* first we need to finish off any pending write work */

  if (i->buf[0] == (void*)0xdeadbeef)
    g_error ("%s called on finished buffer iterator", G_STRFUNC);
  if (i->iteration_no > 0)
    {
      for (no=0; no<i->iterators;no++)
        {
          if (i->flags[no] & GEGL_BUFFER_WRITE)
            {
              if (i->compatible[no])
                {
                  if (i->fish_to[no])
                    babl_process (i->fish_to[no], i->buf[no], i->i[no].data, i->i[no].length);
                }
              else
                {
                  GeglRectangle rect = i->roi;
                  rect.x += (i->rect[no].x-i->rect[0].x);
                  rect.y += (i->rect[no].y-i->rect[0].y);
                  gegl_buffer_set (i->buffer[no], &rect, i->format[no], i->buf[no], GEGL_AUTO_ROWSTRIDE);
                }
            }
        }
    }

  g_assert (i->iterators > 0);

  /* then we iterate all */
  for (no=0; no<i->iterators;no++)
    {
      if (i->compatible[no])
        {
          gboolean res;
          res = gegl_buffer_scan_iterator_next (&i->i[no]);
          if (no == 0)
            {
              result = res;
              i->roi = i->i->roi; /* the "clock" we're operating from is
                                     the first buffer*/
              i->length = i->i->length;
            }
          else
            {
              g_assert (i->length == i->i->length);
            }

          /* since they were scan compatible this should be true */
          if (res != result)
            {
              g_print ("%i %i %i\n", res, result);
             } 
          g_assert (res == result);

          if (i->buf[no]!=NULL)
            {
              if (i->flags[no] & GEGL_BUFFER_READ)
                babl_process (i->fish[no], i->i[no].data, i->buf[no], i->i[no].length);
              i->data[no]=i->buf[no];
            }
          else
            {
              i->data[no]=i->i[no].data;
            }
        }
      else
        {
          GeglRectangle rect = i->roi;
          rect.x += (i->rect[no].x-i->rect[0].x);
          rect.y += (i->rect[no].y-i->rect[0].y);
          g_assert (i->buf[no]);

          gegl_buffer_get (i->buffer[no], 1.0, &rect, i->format[no], i->buf[no], GEGL_AUTO_ROWSTRIDE);
          i->data[no]=i->buf[no];
        }
    }

  i->iteration_no++;

  if (result == FALSE)
    {
      for (no=0; no<i->iterators;no++)
        {
          if (i->buf[no])
            gegl_free (i->buf[no]);
          i->buf[no]=NULL;
        }
      i->buf[0]=(void*)0xdeadbeef;
      g_free (i);
    }

  return result;
}

GeglBufferIterator *gegl_buffer_iterator_new (GeglBuffer          *buffer,
                                              GeglRectangle        roi, 
                                              const Babl          *format,
                                              guint                flags)
{
  GeglBufferIterator *i = (gpointer)g_new0 (GeglBufferIterators, 1);
  gegl_buffer_iterator_add (i, buffer, roi, format, flags);
  return i;
}
