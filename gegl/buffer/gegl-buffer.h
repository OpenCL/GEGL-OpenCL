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

#ifndef __GEGL_BUFFER_H__
#define __GEGL_BUFFER_H__

#include <glib-object.h>
#include <babl/babl.h>

G_BEGIN_DECLS

#define GEGL_TYPE_BUFFER (gegl_buffer_get_type ())
#define GEGL_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_BUFFER, GeglBuffer))
#ifndef __GEGL_BUFFER_TYPES_H__
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
 * @extent: the geometry of the buffer (origin, width and height) a
 * GeglRectangle.
 * @format: the Babl pixel format to be used, create one with babl_format("RGBA
 * u8") and similar.
 *
 * Create a new GeglBuffer of a given format with a given extent. It is
 * possible to pass in NULL for both extent and format, a NULL extent creates
 * an empty buffer and a NULL format makes the buffer default to "RGBA float".
 */
GeglBuffer*     gegl_buffer_new               (const GeglRectangle *extent,
                                               const Babl          *format);


/**
 * gegl_buffer_open:
 * @path: the path to a gegl buffer on disk.
 *
 * Open an existing on-disk GeglBuffer, this buffer is opened in a monitored
 * state so multiple instances of gegl can share the same buffer. Sets on
 * one buffer are reflected in the other.
 *
 * Returns: a GeglBuffer object.
 */
GeglBuffer*     gegl_buffer_open              (const gchar         *path);

/**
 * gegl_buffer_save:
 * @buffer: a #GeglBuffer.
 * @path: the path where the gegl buffer will be saved, any writable GIO uri is valid.
 * @roi: the region of interest to write, this is the tiles that will be collected and
 * written to disk.
 *
 * Write a GeglBuffer to a file.
 */
void            gegl_buffer_save              (GeglBuffer          *buffer,
                                               const gchar         *path,
                                               const GeglRectangle *roi);

/**
 * gegl_buffer_load:
 * @path: the path to a gegl buffer on disk.
 *
 * Loads an existing GeglBuffer from disk, if it has previously been saved with
 * gegl_buffer_save it should be possible to open through any GIO transport, buffers
 * that have been used as swap needs random access to be opened.
 *
 * Returns: a #GeglBuffer object.
 */
GeglBuffer     *gegl_buffer_load              (const gchar         *path);

/**
 * gegl_buffer_flush:
 * @buffer: a #GeglBuffer
 *
 * Flushes all unsaved data to disk, this is not neccesary for shared
 * geglbuffers opened with gegl_buffer_open since they auto-sync on writes.
 */
void            gegl_buffer_flush             (GeglBuffer          *buffer);


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


/**
 * gegl_buffer_set_extent:
 * @buffer: the buffer to operate on.
 * @extent: new extent.
 *
 * Changes the size and position that is considered active in a buffer, this
 * operation is valid on any buffer, reads on subbuffers outside the master
 * buffer's extent are at the moment undefined.
 *
 * Returns TRUE if the change of extent was succesful.
 */
gboolean gegl_buffer_set_extent (GeglBuffer          *buffer,
                                 const GeglRectangle *extent);

/* convenience access macros */

/**
 * gegl_buffer_get_x:
 * @buffer: a GeglBuffer
 *
 * Evaluates to the X coordinate of the upper left corner of the buffer's extent.
 */
#define gegl_buffer_get_x(buffer)        (gegl_buffer_get_extent(buffer)->x)

/**
 * gegl_buffer_get_y:
 * @buffer: a GeglBuffer
 *
 * Evaluates to the Y coordinate of the upper left corner of the buffer's extent.
 */
#define gegl_buffer_get_y(buffer)        (gegl_buffer_get_extent(buffer)->y)

/**
 * gegl_buffer_get_width:
 * @buffer: a GeglBuffer
 *
 * Evaluates to the width of the buffer's extent.
 */
#define gegl_buffer_get_width(buffer)    (gegl_buffer_get_extent(buffer)->width)

/**
 * gegl_buffer_get_height:
 * @buffer: a GeglBuffer
 *
 * Evaluates to the height of the buffer's extent.
 */
#define gegl_buffer_get_height(buffer)   (gegl_buffer_get_extent(buffer)->height)

/**
 * gegl_buffer_get_pixel_count:
 * @buffer: a GeglBuffer
 *
 * Returns the number of pixels of the extent of the buffer.
 */
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
 * @rowstride: rowstride in bytes, or GEGL_AUTO_ROWSTRIDE to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 *
 * Fetch a rectangular linear buffer of pixel data from the GeglBuffer, the
 * data is converted to the desired BablFormat, if the BablFormat stored and
 * fetched is the same this amounts to a series of memcpy's aligned to demux
 * the tile structure into a linear buffer.
 */
void            gegl_buffer_get               (GeglBuffer          *buffer,
                                               gdouble              scale,
                                               const GeglRectangle *rect,
                                               const Babl          *format,
                                               gpointer             dest,
                                               gint                 rowstride);

/**
 * gegl_buffer_set:
 * @buffer: the buffer to modify.
 * @rect: the coordinates we want to change the data of and the width/height extent, if NULL equal to the extent of the buffer.
 * @format: the babl_format the linear buffer @src.
 * @src: linear buffer of image data to be stored in @buffer.
 * @rowstride: rowstride in bytes, or GEGL_AUTO_ROWSTRIDE to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 *
 * Store a linear raster buffer into the GeglBuffer.
 */
void            gegl_buffer_set               (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               const Babl          *format,
                                               void                *src,
                                               gint                 rowstride);


/**
 * gegl_buffer_get_format:
 * @buffer: a #GeglBuffer
 *
 * Get the native babl format of the buffer.
 *
 * Returns: the babl format used for storing pixels in the buffer.
 * 
 */
const Babl    *gegl_buffer_get_format        (GeglBuffer           *buffer);

/**
 * gegl_buffer_clear:
 * @buffer: a #GeglBuffer
 * @roi: a rectangular region
 *
 * Clears the provided rectangular region by setting all the associated memory
 * to 0
 */
void            gegl_buffer_clear             (GeglBuffer          *buffer,
                                               const GeglRectangle *roi);


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
  GEGL_INTERPOLATION_CUBIC,
  GEGL_INTERPOLATION_LANCZOS,
  GEGL_INTERPOLATION_DOWNSHARP,
  GEGL_INTERPOLATION_DOWNSIZE,
  GEGL_INTERPOLATION_DOWNSMOOTH,
  GEGL_INTERPOLATION_UPSHARP,
  GEGL_INTERPOLATION_UPSIZE,
  GEGL_INTERPOLATION_UPSMOOTH
} GeglInterpolation;

/**
 * gegl_buffer_sample:
 * @buffer: the GeglBuffer to sample from
 * @x: x coordinate to sample in buffer coordinates
 * @y: y coordinate to sample in buffer coordinates
 * @scale: the scale we're fetching at (<1.0 can lead to decimation)
 * @dest: buffer capable of storing one pixel in @format.
 * @format: the format to store the sampled color in.
 * @interpolation: the interpolation behavior to use, currently only nearest
 * neighbour is implemented for this API, bilinear, bicubic and lanczos needs
 * to be ported from working code. Valid values: GEGL_INTERPOLATION_NEAREST and
 * GEGL_INTERPOLATION_LINEAR, GEGL_INTERPOLATON_CUBIC and
 * GEGL_INTERPOLATION_LANCZOS.
 *
 * Query interpolate pixel values at a given coordinate using a specified form
 * of interpolation. The samplers used cache for a small neighbourhood of the
 * buffer for more efficient access.
 */
void            gegl_buffer_sample            (GeglBuffer       *buffer,
                                               gdouble           x,
                                               gdouble           y,
                                               gdouble           scale,
                                               gpointer          dest,
                                               const Babl       *format,
                                               GeglInterpolation interpolation);


/**
 * gegl_buffer_sample_cleanup:
 * @buffer: the GeglBuffer to sample from
 *
 * Clean up resources used by sampling framework of buffer (will be freed
 * automatically later when the buffer is destroyed, for long lived buffers
 * cleaning up the sampling infrastructure when it has been used for its
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
 * gegl_buffer_linear_new:
 * @extent: dimensions of buffer.
 * @format: desired pixel format.
 *
 * Creates a GeglBuffer backed by a linear memory buffer, of the given
 * @extent in the specified @format. babl_format ("R'G'B'A u8") for instance
 * to make a normal 8bit buffer.
 *
 * Returns: a GeglBuffer that can be used as any other GeglBuffer.
 */
GeglBuffer *gegl_buffer_linear_new           (const GeglRectangle *extent,
                                              const Babl          *format);

/**
 * gegl_buffer_linear_new_from_data:
 * @data: a pointer to a linear buffer in memory.
 * @format: the format of the data in memory
 * @extent: the dimensions (and upper left coordinates) of linear buffer.
 * @rowstride: the number of bytes between rowstarts in memory (or 0 to
 *             autodetect)
 * @destroy_fn: function to call to free data or NULL if memory should not be
 *              freed.
 * @destroy_fn_data: extra argument to be passed to void destroy(ptr, data) type
 *              function.
 *
 * Creates a GeglBuffer backed by a linear memory buffer that already exists,
 * of the given @extent in the specified @format. babl_format ("R'G'B'A u8")
 * for instance to make a normal 8bit buffer. 
 *
 * Returns: a GeglBuffer that can be used as any other GeglBuffer.
 */
GeglBuffer * gegl_buffer_linear_new_from_data (const gpointer       data,
                                                const Babl          *format,
                                                const GeglRectangle *extent,
                                                gint                 rowstride,
                                                GCallback            destroy_fn,
                                                gpointer             destroy_fn_data);

/**
 * gegl_buffer_linear_open:
 * @buffer: a #GeglBuffer.
 * @extent: region to open, pass NULL for entire buffer.
 * @rowstride: return location for rowstride.
 * @format: desired format or NULL to use buffers format.
 *
 * Raw direct random access to the full data of a buffer in linear memory.
 *
 * Returns: a pointer to a linear memory region describing the buffer, if the
 * request is compatible with the underlying data storage direct access
 * to the underlying data is provided.
 */
gpointer       *gegl_buffer_linear_open      (GeglBuffer          *buffer,
                                              const GeglRectangle *extent,
                                              gint                *rowstride,
                                              const Babl          *format);

/**
 * gegl_buffer_linear_close:
 * @buffer: a #GeglBuffer.
 * @linear: a previously returned buffer.
 *
 * This function makes sure GeglBuffer and underlying code is aware of changes
 * being made to the linear buffer. If the request was not a compatible one
 * it is written back to the buffer. Multiple concurrent users can be handed
 * the same buffer (both raw access and converted).
 */
void            gegl_buffer_linear_close     (GeglBuffer    *buffer,
                                              gpointer       linear);


/**
 * gegl_buffer_get_abyss:
 * @buffer: a #GeglBuffer.
 *
 * Return the abyss extent of a buffer, this expands out to the parents extent in
 * subbuffers.
 */
const GeglRectangle* gegl_buffer_get_abyss  (GeglBuffer           *buffer);

/**
 */
G_END_DECLS
#endif
