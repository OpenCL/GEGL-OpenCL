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

/***
 * GeglBuffer:
 *
 * GeglBuffer is the API used by GEGL for storing and retrieving raster data.
 * GeglBuffer heavily relies on babl for translation and description of
 * different pixel formats.
 *
 * Internally GeglBuffer currently uses a tiled mipmap pyramid structure that
 * can be swapped to disk. In the future GeglBuffer might also support a linear
 * backend, a GPU memory backend and a network backend for buffers.
 */
GType           gegl_buffer_get_type          (void) G_GNUC_CONST;

/** 
 * gegl_buffer_new:
 * @extent: the geometry of the buffer (origin, width and height) a GeglRectangle.
 * @format: the Babl pixel format to be used, create one with babl_format("RGBA
 * u8") and similar.
 *
 * Create a new GeglBuffer of a given format with a given extent. It is possible
 * to pass in NULL for both extent and format, a NULL extent creates an exmpty
 * buffer and a NULL format makes the buffer default to "RGBA float".
 */
GeglBuffer*     gegl_buffer_new               (const GeglRectangle *extent,
                                               Babl                *format);

/** 
 * gegl_buffer_create_sub_buffer:
 * @buffer: parent buffer.
 * @extent: coordinates of new buffer.
 *
 * Create a new sub GeglBuffer, that is a view on a larger buffer.
 */
GeglBuffer*     gegl_buffer_create_sub_buffer (GeglBuffer          *buffer,
                                               const GeglRectangle *extent);

/**
 * gegl_buffer_destroy:
 * @buffer: the buffer we're done with.
 *
 * Destroys a buffer and frees up the resources used by a buffer, internally
 * this is done with reference counting and gobject, and gegl_buffer_destroy
 * is a thin wrapper around g_object_unref.
 **/
void            gegl_buffer_destroy           (GeglBuffer          *buffer);

/**
 * gegl_buffer_get_extent:
 * @buffer: the buffer to operate on.
 *
 * Returns a pointer to a GeglRectangle structure defining the geometry of a
 * specific GeglBuffer, this is also the default width/height of buffers passed
 * in to gegl_buffer_set and gegl_buffer_get (with a scale of 1.0 at least).
 */
const GeglRectangle * gegl_buffer_get_extent (GeglBuffer *buffer);

/* convenience access macros */
#define gegl_buffer_get_x(buffer)           (gegl_buffer_get_extent(buffer)->x)
#define gegl_buffer_get_y(buffer)           (gegl_buffer_get_extent(buffer)->y)
#define gegl_buffer_get_width(buffer)       (gegl_buffer_get_extent(buffer)->width)
#define gegl_buffer_get_height(buffer)      (gegl_buffer_get_extent(buffer)->height)
#define gegl_buffer_get_pixel_count(buffer) (gegl_buffer_get_width(buffer) * gegl_buffer_get_height(buffer))

#ifndef GEGL_AUTO_ROWSTRIDE
#define GEGL_AUTO_ROWSTRIDE 0
#endif

/**
 * gegl_buffer_get:
 * @buffer: the buffer to retrieve data from.
 * @scale: sampling scale, 1.0 = pixel for pixel 2.0 = magnify, 0.5 scale down.
 * @rect: the coordinates we want to retrieve data from, and width/height of
 * destination buffer, if NULL equal to the extent of the buffer. The
 * coordinates and dimensions are after scale has been applied.
 * @format: the BablFormat to store in the linear buffer @dest.
 * @dest: the memory destination for a linear buffer for the pixels, the size needed
 * depends on the requested BablFormat.
 * @rowstride: rowstride in bytes, or GEGL_ROWSTRIDE_AUTO to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 *
 * Fetch a rectangular linear buffer of pixel data from the GeglBuffer, the data is
 * converted to the desired BablFormat, if the BablFormat stored and fetched is the
 * same this amounts to a series of memcpy's aligned to demux the tile structure into
 * a linear buffer.
 */
void            gegl_buffer_get               (GeglBuffer          *buffer,
                                               gdouble              scale,
                                               const GeglRectangle *rect,
                                               Babl                *format,
                                               gpointer             dest,
                                               gint                 rowstride);

/**
 * gegl_buffer_set:
 * @buffer: the buffer to modify.
 * @rect: the coordinates we want to change the data of and the width/height extent, if NULL equal to the extent of the buffer.
 * @format: the babl_format the linear buffer @src.
 * @src: linear buffer of image data to be stored in @buffer.
 *
 * Store a linear raster buffer into the GeglBuffer.
 */
void            gegl_buffer_set               (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               Babl                *format,
                                               void                *src);


/**
 * gegl_buffer_copy:
 * @src: source buffer.
 * @src_rect: source rectangle (or NULL to copy entire source buffer)
 * @dst: destination buffer.
 * @dst_rect: position of upper left destination pixel, or NULL for top
 * left coordinates of the buffer extents.
 *
 * copies a region from source buffer to destination buffer.
 *
 * If the babl_formats of the buffers are the same, and the tile boundaries
 * align, this should optimally lead to shared tiles that are copy on write,
 * this functionality is not implemented yet.
 */
void            gegl_buffer_copy              (GeglBuffer          *src,
                                               const GeglRectangle *src_rect,
                                               GeglBuffer          *dst,
                                               const GeglRectangle *dst_rect);


/** 
 * gegl_buffer_dup:
 * @buffer: the GeglBuffer to duplicate.
 *
 * duplicate a buffer (internally uses gegl_buffer_copy), this should ideally
 * lead to a buffer that shares the raster data with the original on a tile
 * by tile COW basis. This is not yet implemented
 */
GeglBuffer    * gegl_buffer_dup               (GeglBuffer       *buffer);

typedef enum {
  GEGL_INTERPOLATION_NEAREST = 0,
  GEGL_INTERPOLATION_LINEAR,
} GeglInterpolation;

/**
 * gegl_buffer_sample:
 * @buffer: the GeglBuffer to sample from
 * @x: x coordinate to sample in buffer coordinates
 * @y: y coordinate to sample in buffer coordinates
 * @scale: the scale we're fetching at (<1.0 can leads to decimation)
 * @dest: buffer capable of storing one pixel in @format.
 * @format: the format to store the sampled color in.
 * @interpolation: the interpolation behavior to use, currently only nearest
 * neighbour is implemented for this API, bilinear, bicubic and lanczos needs
 * to be ported from working code. Valid values: GEGL_INTERPOLATION_NEAREST
 *
 * Resample the buffer at some given coordinates using a specified format. For
 * some operations this might be sufficient, but it might be considered
 * prototyping convenience that needs to be optimized away from algorithms
 * using it later.
 */
void            gegl_buffer_sample            (GeglBuffer       *buffer,
                                               gdouble           x,
                                               gdouble           y,
                                               gdouble           scale,
                                               gpointer          dest,
                                               Babl             *format,
                                               GeglInterpolation interpolation);

/**
 * gegl_buffer_sample_cleanup:
 * @buffer: the GeglBuffer to sample from
 *
 * Clean up resources used by sampling framework of buffer (will be freed
 * automatically later when the buffer is destroyed, for long lived buffers
 * cleaning up the sampling infrastructure when it has been used for it's
 * purpose will sometimes be more efficient).
 */ 
void            gegl_buffer_sample_cleanup    (GeglBuffer *buffer);

/**
 * gegl_buffer_interpolation_from_string:
 * @string: the string to look up
 *
 * Looks up the GeglInterpolation corresponding to a string, if no matching
 * interpolation is found returns GEGL_INTERPOLATION_NEAREST.
 */
GeglInterpolation gegl_buffer_interpolation_from_string (const gchar *string);

/**
 */
#endif
