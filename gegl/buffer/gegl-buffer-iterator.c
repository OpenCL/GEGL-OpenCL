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
#include "gegl-tile-storage.h"

gboolean                gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i);
GeglBufferTileIterator *gegl_buffer_tile_iterator_new  (GeglBuffer             *buffer,
                                                        GeglRectangle           roi,
                                                        gboolean                write);
void                    gegl_buffer_tile_iterator_init (GeglBufferTileIterator *i,
                                                        GeglBuffer             *buffer,
                                                        GeglRectangle           roi,
                                                        gboolean                write);



#define gegl_buffer_scan_iterator_get_x(i) \
    ((((GeglBufferTileIterator*)(i))->roi.x) + \
    (((GeglBufferTileIterator*)(i))->real_col))
#define gegl_buffer_scan_iterator_get_y(i) \
    ( (((GeglBufferTileIterator*)(i))->roi.y)+ \
      (((GeglBufferTileIterator*)(i))->real_row)+ \
      ((GeglBufferScanIterator*)(i))->real_row)

#define gegl_buffer_scan_iterator_get_rectangle(i,rect_ptr) \
  do{GeglRectangle *foo = rect_ptr;\
   if (foo) {\
   foo->x=gegl_buffer_scan_iterator_get_x(i);\
   foo->y=gegl_buffer_scan_iterator_get_y(i);\
   foo->width= ((GeglBufferTileIterator*)i)->subrect.width;\
   foo->height=((GeglBufferScanIterator*)i)->length/ foo->width;\
   }}while(0)

void gegl_buffer_tile_iterator_init (GeglBufferTileIterator *i,
                                     GeglBuffer             *buffer,
                                     GeglRectangle           roi,
                                     gboolean                write)
{
  g_assert (i);
  memset (i, 0, sizeof (GeglBufferTileIterator));
  i->buffer = buffer;
  i->roi = roi;
  i->row    = 0;
  i->col = 0;
  i->tile = NULL;
  i->real_col = 0;
  i->real_row = 0;
  i->write = write;
}


GeglBufferTileIterator *
gegl_buffer_tile_iterator_new (GeglBuffer    *buffer,
                               GeglRectangle  roi,
                               gboolean       write)
{
  GeglBufferTileIterator *i = g_malloc (sizeof (GeglBufferTileIterator));
  gegl_buffer_tile_iterator_init (i, buffer, roi, write);
  return i; 
}

void gegl_buffer_scan_iterator_init (GeglBufferScanIterator *i,
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
  i->real_row = 0;
  if (write)
    gegl_buffer_lock (buffer);
}

GeglBufferScanIterator *gegl_buffer_scan_iterator_new (GeglBuffer             *buffer,
                                                       GeglRectangle           roi,
                                                       gboolean                write)
{
  GeglBufferScanIterator *i = g_malloc (sizeof (GeglBufferScanIterator));
  gegl_buffer_scan_iterator_init (i, buffer, roi, write);
  return i;
}

gboolean
gegl_buffer_scan_iterator_next (GeglBufferScanIterator *i)
{
  GeglBufferTileIterator *tile_i = (GeglBufferTileIterator*)i;

  if (tile_i->tile==NULL)
    {
      gulp:
      if (!gegl_buffer_tile_iterator_next (tile_i))
        return FALSE;
      i->length = tile_i->subrect.width;
      i->rowstride = tile_i->subrect.width;
      i->row = 0;
      i->real_row = 0;
    }
  /* we should now have a valid tile */

  if (tile_i->subrect.width == tile_i->buffer->tile_storage->tile_width &&
      i->row < tile_i->subrect.height)
    /* the entire contents of the tile can be expressed as one long scan */
    {
      gint  px_size = tile_i->buffer->format->format.bytes_per_pixel;
      guchar *data = tile_i->data;
      i->length = tile_i->subrect.width * tile_i->subrect.height;
      i->rowstride = tile_i->subrect.width;
      i->data = data + px_size * (tile_i->subrect.width * tile_i->subrect.y);
      i->row = tile_i->subrect.height;
      i->real_row = 0;
/*      if (i->rowstride < 0)
        {
          return FALSE;
        }*/
      gegl_buffer_scan_iterator_get_rectangle (i, &(i->roi));
      return TRUE;
    }
  else if (i->row < tile_i->subrect.height)
    /* iterate thorugh the scanlines in the subrect */
    {
      guchar *data = tile_i->sub_data;
      i->data = data + i->row * tile_i->rowstride;
      i->real_row = i->row;
      i->row ++;
      return TRUE;
    }
  else
    { /* we're done with that tile go get another one if possible */
      goto gulp;
    }

  if (tile_i->write)
    gegl_buffer_unlock (tile_i->buffer);

  return FALSE;
}


gboolean gegl_buffer_scan_compatible (GeglBuffer *input,
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

gboolean
gegl_buffer_tile_iterator_next (GeglBufferTileIterator *i)
{
  GeglBuffer *buffer = i->buffer;
  gint  tile_width   = buffer->tile_storage->tile_width;
  gint  tile_height  = buffer->tile_storage->tile_height;
  gint  buffer_shift_x = buffer->shift_x;
  gint  buffer_shift_y = buffer->shift_y;
  gint  buffer_x       = buffer->extent.x + buffer_shift_x;
  gint  buffer_y       = buffer->extent.y + buffer_shift_y;
  gint  buffer_abyss_x = buffer->abyss.x + buffer_shift_x;
  gint  abyss_x_total  = buffer_abyss_x + buffer->abyss.width;

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

  if (i->col < i->roi.width)
    { /* return tile on this row */
      gint tiledx = buffer_x + i->col;
      gint tiledy = buffer_y + i->row;
      gint offsetx = gegl_tile_offset (tiledx, tile_width);
      gint offsety = gegl_tile_offset (tiledy, tile_height);
      gint pixels; 

      if (i->roi.width + offsetx - i->col < tile_width)
        pixels = (i->roi.width + offsetx - i->col) - offsetx;
      else
        pixels = tile_width - offsetx;

     /*if (!(buffer_x + i->col + tile_width >= buffer_abyss_x &&
                      buffer_x + i->col < abyss_x_total))
       { 
          g_warning ("entire tile in abyss?");

          i->col += tile_width - offsetx;
          goto gulp;
       }
     else*/
       {
         i->subrect.x = offsetx;
         i->subrect.y = offsety;

         i->subrect.width = 
           //pixels;
        (i->roi.width - i->col + tile_width < tile_width) ?
                             (i->roi.width - i->col + tile_width) - i->subrect.x:
                             tile_width - i->subrect.x;



         i->subrect.height = (i->roi.height - i->row + tile_height < tile_height) ?
                             (i->roi.height - i->row + tile_height) - i->subrect.y:
                             tile_height - i->subrect.y;

         if(0){
         gint lskip = (buffer_abyss_x) - (buffer_x + i->col);
         /* gap between left side of tile, and abyss */
         gint rskip = (buffer_x + i->col + i->subrect.width) - abyss_x_total;
         /* gap between right side of tile, and abyss */

           if (lskip < 0)
              lskip = 0;
           if (lskip > i->subrect.width)
              lskip = i->subrect.width;
           if (rskip < 0)
              rskip = 0;
           if (rskip > i->subrect.width)
              rskip = i->subrect.width;
           i->subrect.width = i->subrect.width - rskip - lskip;
         }

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

         /* update with new future position (note this means that the
          * coordinates read from the iterator do not make full sense 
          * */
         i->real_col = i->col;
         i->real_row = i->row;
         i->col += tile_width - offsetx;

         return TRUE;
       }
    }
  else /* move down to next row */
    {
      gint tiledy = buffer_y + i->row;
      gint offsety = gegl_tile_offset (tiledy, tile_height);

      i->real_row = i->row;
      i->real_col = i->col;
      i->row += tile_height - offsety;
      i->col=0;

      if (i->row < i->roi.height)
        {
          goto gulp; /* return the first tile in the next row */
        }

      return FALSE;
    }
  return FALSE;
}
