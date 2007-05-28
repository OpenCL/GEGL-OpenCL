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
 * Copyright 2006, 2007 Øyvind Kolås <pippin@gimp.org>
 */
#include <glib.h>
#include <glib-object.h>

#include <babl/babl.h>

#ifndef _GEGL_BUFFER_H
#define _GEGL_BUFFER_H

#define GEGL_TYPE_BUFFER (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#ifndef _GEGL_BUFFER_TYPES_H
typedef struct _GeglBuffer     GeglBuffer;
#endif


GType           gegl_buffer_get_type     (void) G_GNUC_CONST;

/** 
 * gegl_buffer_new:
 * @extent: the geometry of the buffer (origin, width and height) a GeglRectangle.
 * @format: the Babl pixel format to be used, create one with babl_format("RGBA
 * u8") and similar.
 *
 * Create a new GeglBuffer of a given format with a given extent.
 */
GeglBuffer*     gegl_buffer_new          (GeglRectangle    *extent,
                                          Babl             *format);

/** 
 * gegl_buffer_new_from_buf:
 * @buffer: parent buffer.
 * @extent: coordinates of new buffer.
 *
 * Create a new sub GeglBuffer, that is a view on a larger buffer.
 */
GeglBuffer*     gegl_buffer_new_from_buf (GeglBuffer       *buffer,
                                          GeglRectangle    *extent);

/**
 * gegl_buffer_destroy:
 * @buffer: the buffer we're done with.
 *
 * Destroys a buffer and frees up the resources used by a buffer, internally
 * this is done with reference counting and gobject, and gegl_buffer_destroy
 * is a thing wrapper around g_object_unref.
 **/
void            gegl_buffer_destroy      (GeglBuffer       *buffer);

/**
 * gegl_buffer_extent:
 * @buffer: the buffer to operate on.
 *
 * Returns a pointer to a GeglRectangle structure defining the geometry of a
 * specific GeglBuffer, this is also the default width/height of buffers passed
 * in to gegl_buffer_set and gegl_buffer_get (with a scale of 1.0 at least).
 */
GeglRectangle * gegl_buffer_extent       (GeglBuffer       *buffer);

/**
 * gegl_buffer_get:
 * @buffer: the buffer to retrieve data from.
 * @rect: the coordinates we want to retrieve data from, and width/height
 * of destination buffer, if NULL equal to the extent of the buffer.
 * @scale: sampling scale, 1.0 = pixel for pixel 2.0 = magnify, 0.5 scale down.
 * @format: the BablFormat to store in the linear buffer @dest.
 * @dest: the memory destination for a linear buffer for the pixels, the size needed
 * depends on the requested BablFormat.
 *
 * Fetch a rectangular linear buffer of pixel data from the GeglBuffer, the data is
 * converted to the desired BablFormat, if the BablFormat stored and fetched is the
 * same this amounts to s series of memcpy's aligned to demux the tile structure into
 * a linear buffer.
 */
void            gegl_buffer_get          (GeglBuffer       *buffer,
                                          GeglRectangle    *rect,
                                          gdouble           scale,
                                          Babl             *format,
                                          void             *dest);

/**
 * gegl_buffer_set:
 * @buffer: the buffer to modify.
 * @rect: the coordinates we want to change the data of and the width/height extent, if NULL equal to
 * the extent of the buffer.
 * @format: the babl_format the linear buffer @src.
 * @src: linear buffer of image data to be stored in @buffer.
 *
 * Modify a rectangle of the buffer, 
 * 
 * 
 * 
 */
void            gegl_buffer_set          (GeglBuffer       *buffer,
                                          GeglRectangle    *rect,
                                          Babl             *format,
                                          void             *src);

typedef enum {
  GEGL_INTERPOLATION_NEAREST
} GeglInterpolation;


/**
 * gegl_buffer_sample:
 * @buffer: the GeglBuffer to sample from
 * @x: x coordinate to sample in buffer coordinates
 * @y: y coordinate to sample in buffer coordinates
 * @scale: the scale we're fetching at (<1.0 can leads to decimation)
 * @dest: buffer capable of storing one pixel in @format.
 * @interpolation: the interpolation behavior to use, currently only nearest
 * neighbour is implemented for this API, bilinear, bicubic and lanczos needs
 * to be ported from working code.
 *
 * Resample the buffer at some given coordinates using a specified format. For some
 * operations this might be sufficient, but it might be considered prototyping convenience
 * that needs to be optimized away from algorithms using it later.
 */
void            gegl_buffer_sample       (GeglBuffer       *buffer,
                                          gdouble           x,
                                          gdouble           y,
                                          gdouble           scale,
                                          gpointer          dest,
                                          Babl             *format,
                                          GeglInterpolation interpolation);

#endif
