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
#include <gegl-matrix.h>
#include <gegl-types.h>

G_BEGIN_DECLS

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

/**
 * gegl_buffer_new: (skip)
 * @extent: the geometry of the buffer (origin, width and height) a
 * GeglRectangle.
 * @format: the Babl pixel format to be used, create one with babl_format("RGBA
 * u8") and similar.
 *
 * Create a new GeglBuffer of a given format with a given extent. It is
 * possible to pass in NULL for both extent and format, a NULL extent creates
 * an empty buffer and a NULL format makes the buffer default to "RGBA float".
 */
GeglBuffer *    gegl_buffer_new               (const GeglRectangle *extent,
                                               const Babl          *format);

/**
 * gegl_buffer_new_for_backend:
 * @extent: the geometry of the buffer (origin, width and height) a
 * GeglRectangle.
 * @backend: an instance of a GeglTileBackend subclass.
 *
 * Create a new GeglBuffer from a backend, if NULL is passed in the extent of
 * the buffer will be inherited from the extent of the backend.
 *
 * returns a GeglBuffer, that holds a reference to the provided backend.
 */
GeglBuffer *   gegl_buffer_new_for_backend    (const GeglRectangle *extent,
                                               GeglTileBackend     *backend);

/**
 * gegl_buffer_add_handler:
 * @buffer: a #GeglBuffer
 * @handler: a #GeglTileHandler
 *
 * Add a new tile handler in the existing chain of tile handler of a GeglBuffer.
 */
void           gegl_buffer_add_handler        (GeglBuffer          *buffer,
                                               gpointer             handler);

/**
 * gegl_buffer_remove_handler:
 * @buffer: a #GeglBuffer
 * @handler: a #GeglTileHandler
 *
 * Remove the provided tile handler in the existing chain of tile handler of a GeglBuffer.
 */
void           gegl_buffer_remove_handler     (GeglBuffer          *buffer,
                                               gpointer             handler);

/**
 * gegl_buffer_open:
 * @path: the path to a gegl buffer on disk.
 *
 * Open an existing on-disk GeglBuffer, this buffer is opened in a monitored
 * state so multiple instances of gegl can share the same buffer. Sets on
 * one buffer are reflected in the other.
 *
 * Returns: (transfer full): a GeglBuffer object.
 */
GeglBuffer *    gegl_buffer_open              (const gchar         *path);

/**
 * gegl_buffer_save:
 * @buffer: (transfer none): a #GeglBuffer.
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
 * Returns: (transfer full): a #GeglBuffer object.
 */
GeglBuffer *     gegl_buffer_load             (const gchar         *path);

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
 * @buffer: (transfer none): parent buffer.
 * @extent: (transfer none): coordinates of new buffer.
 *
 * Create a new sub GeglBuffer, that is a view on a larger buffer.
 *
 * Return value: (transfer full): the new sub buffer
 */
GeglBuffer *    gegl_buffer_create_sub_buffer (GeglBuffer          *buffer,
                                               const GeglRectangle *extent);

/**
 * gegl_buffer_get_extent:
 * @buffer: the buffer to operate on.
 *
 * Returns a pointer to a GeglRectangle structure defining the geometry of a
 * specific GeglBuffer, this is also the default width/height of buffers passed
 * in to gegl_buffer_set and gegl_buffer_get (with a scale of 1.0 at least).
 */
const GeglRectangle * gegl_buffer_get_extent  (GeglBuffer *buffer);


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
gboolean          gegl_buffer_set_extent      (GeglBuffer          *buffer,
                                               const GeglRectangle *extent);

/**
 * gegl_buffer_set_abyss:
 * @buffer: the buffer to operate on.
 * @abyss: new abyss.
 *
 * Changes the size and position of the abyss rectangle of a buffer.
 *
 * Returns TRUE if the change of abyss was succesful.
 */
gboolean          gegl_buffer_set_abyss      (GeglBuffer          *buffer,
                                              const GeglRectangle *abyss);

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

/**
 * gegl_buffer_get: (skip)
 * @buffer: the buffer to retrieve data from.
 * @rect: the coordinates we want to retrieve data from, and width/height of
 * destination buffer, if NULL equal to the extent of the buffer. The
 * coordinates and dimensions are after scale has been applied.
 * @scale: sampling scale, 1.0 = pixel for pixel 2.0 = magnify, 0.5 scale down.
 * @format: the BablFormat to store in the linear buffer @dest.
 * @dest: the memory destination for a linear buffer for the pixels, the size needed
 * depends on the requested BablFormat.
 * @rowstride: rowstride in bytes, or GEGL_AUTO_ROWSTRIDE to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 * @repeat_mode: how requests outside the buffer extent are handled.
 * Valid values: GEGL_ABYSS_NONE (abyss pixels are zeroed), GEGL_ABYSS_WHITE
 * (abyss pixels are white), GEGL_ABYSS_BLACK (abyss pixels are black),
 * GEGL_ABYSS_CLAMP (coordinates are clamped to the abyss rectangle),
 * GEGL_ABYSS_LOOP (buffer contents are tiled if outside of the abyss rectangle).
 *
 * Fetch a rectangular linear buffer of pixel data from the GeglBuffer, the
 * data is converted to the desired BablFormat, if the BablFormat stored and
 * fetched is the same this amounts to a series of memcpy's aligned to demux
 * the tile structure into a linear buffer.
 */
void            gegl_buffer_get               (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               gdouble              scale,
                                               const Babl          *format,
                                               gpointer             dest,
                                               gint                 rowstride,
                                               GeglAbyssPolicy      repeat_mode);

/**
 * gegl_buffer_set: (skip)
 * @buffer: the buffer to modify.
 * @rect: the coordinates we want to change the data of and the width/height of
 * the linear buffer being set, scale specifies the scaling factor applied to
 * the data when setting.
 * @scale_level: the scale level being set, 0 = 1:1 = default = base mipmap level,
 * 1 = 1:2, 2=1:4, 3=1:8 ..
 * @format: the babl_format the linear buffer @src.
 * @src: linear buffer of image data to be stored in @buffer.
 * @rowstride: rowstride in bytes, or GEGL_AUTO_ROWSTRIDE to compute the
 * rowstride based on the width and bytes per pixel for the specified format.
 *
 * Store a linear raster buffer into the GeglBuffer.
 */
void            gegl_buffer_set               (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               gint                 scale_level,
                                               const Babl          *format,
                                               const void          *src,
                                               gint                 rowstride);


/**
 * gegl_buffer_set_color:
 * @buffer: a #GeglBuffer
 * @rect: a rectangular region to fill with a color.
 * @color: the GeglColor to fill with.
 *
 * Sets the region covered by rect to the specified color.
 */
void            gegl_buffer_set_color         (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               GeglColor           *color);


/**
 * gegl_buffer_set_pattern:
 * @buffer: a #GeglBuffer
 * @rect: the region of @buffer to fill
 * @pattern: a #GeglBuffer to be repeated as a pattern
 * @x_offset: where the pattern starts horizontally
 * @y_offset: where the pattern starts vertical
 *
 * Fill a region with a repeating pattern. Offsets parameters are
 * relative to the origin (0, 0) and not to the rectangle. So be carefull
 * about the origin of @pattern and @buffer extents.
 */
void            gegl_buffer_set_pattern       (GeglBuffer          *buffer,
                                               const GeglRectangle *rect,
                                               GeglBuffer          *pattern,
                                               gint                 x_offset,
                                               gint                 y_offset);

/**
 * gegl_buffer_get_format: (skip)
 * @buffer: a #GeglBuffer
 *
 * Get the babl format of the buffer, this might not be the format the buffer
 * was originally created with, you need to use gegl_buffer_set_format (buf,
 * NULL); to retireve the original format (potentially having save away the
 * original format of the buffer to re-set it.)
 *
 * Returns: the babl format used for storing pixels in the buffer.
 *
 */
const Babl *    gegl_buffer_get_format        (GeglBuffer           *buffer);


/**
 * gegl_buffer_set_format: (skip)
 * @buffer: a #GeglBuffer
 * @format: the new babl format, must have same bpp as original format.
 *
 * Set the babl format of the buffer, setting the babl format of the buffer
 * requires the new format to have exactly the same bytes per pixel as the
 * original format. If NULL is passed in the format of the buffer is reset to
 * the original format.
 *
 * Returns: the new babl format or NULL if it the passed in buffer was
 * incompatible (then the original format is still used).
 */
const Babl *    gegl_buffer_set_format        (GeglBuffer          *buffer,
                                               const Babl          *format);

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
 * @src: (transfer none): source buffer.
 * @src_rect: source rectangle (or NULL to copy entire source buffer)
 * @repeat_mode: the abyss policy to be using if src_rect is outside src's extent.
 * @dst: (transfer none): destination buffer.
 * @dst_rect: position of upper left destination pixel, or NULL for top
 * left coordinates of the buffer extents.
 *
 * copies a region from source buffer to destination buffer.
 *
 * If the babl_formats of the buffers are the same, and the tile boundaries
 * align, this will create copy-on-write tiles in the destination buffer.
 *
 * This function never does any scaling. When src_rect and dst_rect do not have
 * the same width and height, the size of src_rect is used.
 */
void            gegl_buffer_copy              (GeglBuffer          *src,
                                               const GeglRectangle *src_rect,
                                               GeglAbyssPolicy      repeat_mode,
                                               GeglBuffer          *dst,
                                               const GeglRectangle *dst_rect);



/**
 * gegl_buffer_dup:
 * @buffer: (transfer none): the GeglBuffer to duplicate.
 *
 * Duplicate a buffer (internally uses gegl_buffer_copy). Aligned tiles
 * will create copy-on-write clones in the new buffer.
 *
 * Return value: (transfer full): the new buffer
 */
GeglBuffer *    gegl_buffer_dup               (GeglBuffer       *buffer);


/**
 * gegl_buffer_sample_at_level: (skip)
 * @buffer: the GeglBuffer to sample from
 * @x: x coordinate to sample in buffer coordinates
 * @y: y coordinate to sample in buffer coordinates
 * @scale: a matrix that indicates scaling factors, see
 * gegl_sampler_compute_scale the same.
 * @dest: buffer capable of storing one pixel in @format.
 * @format: the format to store the sampled color in.
 * @level: mipmap level to sample from (@x and @y are level 0 coordinates)
 * @sampler_type: the sampler type to use,
 * to be ported from working code. Valid values: GEGL_SAMPLER_NEAREST,
 * GEGL_SAMPLER_LINEAR, GEGL_SAMPLER_CUBIC, GEGL_SAMPLER_NOHALO and
 * GEGL_SAMPLER_LOHALO
 * @repeat_mode: how requests outside the buffer extent are handled.
 * Valid values: GEGL_ABYSS_NONE (abyss pixels are zeroed), GEGL_ABYSS_WHITE
 * (abyss pixels are white), GEGL_ABYSS_BLACK (abyss pixels are black),
 * GEGL_ABYSS_CLAMP (coordinates are clamped to the abyss rectangle),
 * GEGL_ABYSS_LOOP (buffer contents are tiled if outside of the abyss rectangle).
 *
 * Query interpolate pixel values at a given coordinate using a specified form
 * of interpolation. The samplers used cache for a small neighbourhood of the
 * buffer for more efficient access.
 */

void              gegl_buffer_sample_at_level (GeglBuffer       *buffer,
                                               gdouble           x,
                                               gdouble           y,
                                               GeglMatrix2      *scale,
                                               gpointer          dest,
                                               const Babl       *format,
                                               gint              level,
                                               GeglSamplerType   sampler_type,
                                               GeglAbyssPolicy   repeat_mode);

/**
 * gegl_buffer_sample: (skip)
 * @buffer: the GeglBuffer to sample from
 * @x: x coordinate to sample in buffer coordinates
 * @y: y coordinate to sample in buffer coordinates
 * @scale: a matrix that indicates scaling factors, see
 * gegl_sampler_compute_scale the same.
 * @dest: buffer capable of storing one pixel in @format.
 * @format: the format to store the sampled color in.
 * @sampler_type: the sampler type to use,
 * to be ported from working code. Valid values: GEGL_SAMPLER_NEAREST,
 * GEGL_SAMPLER_LINEAR, GEGL_SAMPLER_CUBIC, GEGL_SAMPLER_NOHALO and
 * GEGL_SAMPLER_LOHALO
 * @repeat_mode: how requests outside the buffer extent are handled.
 * Valid values: GEGL_ABYSS_NONE (abyss pixels are zeroed), GEGL_ABYSS_WHITE
 * (abyss pixels are white), GEGL_ABYSS_BLACK (abyss pixels are black),
 * GEGL_ABYSS_CLAMP (coordinates are clamped to the abyss rectangle),
 * GEGL_ABYSS_LOOP (buffer contents are tiled if outside of the abyss rectangle).
 *
 * Query interpolate pixel values at a given coordinate using a specified form
 * of interpolation. The samplers used cache for a small neighbourhood of the
 * buffer for more efficient access.
 */
void              gegl_buffer_sample          (GeglBuffer       *buffer,
                                               gdouble           x,
                                               gdouble           y,
                                               GeglMatrix2      *scale,
                                               gpointer          dest,
                                               const Babl       *format,
                                               GeglSamplerType   sampler_type,
                                               GeglAbyssPolicy   repeat_mode);



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

typedef void (*GeglSamplerGetFun)  (GeglSampler     *self,
                                    gdouble          x,
                                    gdouble          y,
                                    GeglMatrix2     *scale,
                                    void            *output,
                                    GeglAbyssPolicy  repeat_mode);

/**
 * gegl_sampler_get_fun: (skip)
 */
GeglSamplerGetFun gegl_sampler_get_fun (GeglSampler *sampler);


/**
 * gegl_buffer_sampler_new: (skip)
 * @buffer: buffer to create a new sampler for
 * @format: format we want data back in
 * @sampler_type: the sampler type to use,
 * to be ported from working code. Valid values: GEGL_SAMPLER_NEAREST,
 * GEGL_SAMPLER_LINEAR, GEGL_SAMPLER_CUBIC, GEGL_SAMPLER_NOHALO and
 * GEGL_SAMPLER_LOHALO
 *
 * Create a new sampler, when you are done with the sampler, g_object_unref
 * it.
 *
 * Samplers only hold weak references to buffers, so if its buffer is freed
 * the sampler will become invalid.
 */
GeglSampler *    gegl_buffer_sampler_new      (GeglBuffer       *buffer,
                                               const Babl       *format,
                                               GeglSamplerType   sampler_type);

/**
 * gegl_buffer_sampler_new_at_level: (skip)
 * @buffer: buffer to create a new sampler for
 * @format: format we want data back in
 * @sampler_type: the sampler type to use,
 * @level: the mipmap level to create a sampler for
 * to be ported from working code. Valid values: GEGL_SAMPLER_NEAREST,
 * GEGL_SAMPLER_LINEAR, GEGL_SAMPLER_CUBIC, GEGL_SAMPLER_NOHALO and
 * GEGL_SAMPLER_LOHALO
 *
 * Create a new sampler, when you are done with the sampler, g_object_unref
 * it.
 *
 * Samplers only hold weak references to buffers, so if its buffer is freed
 * the sampler will become invalid.
 */
GeglSampler *    gegl_buffer_sampler_new_at_level (GeglBuffer       *buffer,
                                               const Babl       *format,
                                               GeglSamplerType   sampler_type,
                                               gint              level);


/**
 * gegl_sampler_get:
 * @sampler: a GeglSampler gotten from gegl_buffer_sampler_new
 * @x: x coordinate to sample
 * @y: y coordinate to sample
 * @scale: matrix representing extent of sampling area in source buffer.
 * @output: memory location for output data.
 * @repeat_mode: how requests outside the buffer extent are handled.
 * Valid values: GEGL_ABYSS_NONE (abyss pixels are zeroed), GEGL_ABYSS_WHITE
 * (abyss pixels are white), GEGL_ABYSS_BLACK (abyss pixels are black),
 * GEGL_ABYSS_CLAMP (coordinates are clamped to the abyss rectangle),
 * GEGL_ABYSS_LOOP (buffer contents are tiled if outside of the abyss rectangle).
 *
 * Perform a sampling with the provided @sampler.
 */
void              gegl_sampler_get            (GeglSampler    *sampler,
                                               gdouble         x,
                                               gdouble         y,
                                               GeglMatrix2    *scale,
                                               void           *output,
                                               GeglAbyssPolicy repeat_mode);

/* code template utility, updates the jacobian matrix using
 * a user defined mapping function for displacement, example
 * with an identity transform (note that for the identity
 * transform this is massive computational overhead that can
 * be skipped by passing NULL to the sampler.
 *
 * #define gegl_unmap(x,y,dx,dy) { dx=x; dy=y; }
 *
 * gegl_sampler_compute_scale (scale, x, y);
 * gegl_unmap(x,y,sample_x,sample_y);
 * gegl_buffer_sample (buffer, sample_x, sample_y, scale, dest, format,
 *                     GEGL_SAMPLER_LOHALO);
 *
 * #undef gegl_unmap      // IMPORTANT undefine map macro
 */
#define gegl_sampler_compute_scale(matrix, x, y) \
{                                       \
  float ax, ay, bx, by;                 \
  gegl_unmap(x + 0.5, y, ax, ay);       \
  gegl_unmap(x - 0.5, y, bx, by);       \
  matrix.coeff[0][0] = ax - bx;         \
  matrix.coeff[1][0] = ay - by;         \
  gegl_unmap(x, y + 0.5, ax, ay);       \
  gegl_unmap(x, y - 0.5, bx, by);       \
  matrix.coeff[0][1] = ax - bx;         \
  matrix.coeff[1][1] = ay - by;         \
}


/**
 * gegl_sampler_get_context_rect:
 * @sampler: a GeglSampler gotten from gegl_buffer_sampler_new
 *
 * Returns:The context rectangle of the given @sampler.
 */
const GeglRectangle * gegl_sampler_get_context_rect (GeglSampler *sampler);

/**
 * gegl_buffer_linear_new: (skip)
 * @extent: dimensions of buffer.
 * @format: desired pixel format.
 *
 * Creates a GeglBuffer backed by a linear memory buffer, of the given
 * @extent in the specified @format. babl_format ("R'G'B'A u8") for instance
 * to make a normal 8bit buffer.
 *
 * Returns: a GeglBuffer that can be used as any other GeglBuffer.
 */
GeglBuffer *  gegl_buffer_linear_new          (const GeglRectangle *extent,
                                               const Babl          *format);

/**
 * gegl_buffer_linear_new_from_data: (skip)
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
                                               GDestroyNotify       destroy_fn,
                                               gpointer             destroy_fn_data);

/**
 * gegl_buffer_linear_open: (skip)
 * @buffer: a #GeglBuffer.
 * @extent: region to open, pass NULL for entire buffer.
 * @rowstride: return location for rowstride.
 * @format: desired format or NULL to use buffers format.
 *
 * Raw direct random access to the full data of a buffer in linear memory.
 *
 * Returns: a pointer to a linear memory region describing the buffer, if the
 * request is compatible with the underlying data storage direct access
 * to the underlying data is provided. Otherwise, it returns a copy of the data.
 */
gpointer        gegl_buffer_linear_open       (GeglBuffer          *buffer,
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
void            gegl_buffer_linear_close      (GeglBuffer    *buffer,
                                               gpointer       linear);


/**
 * gegl_buffer_get_abyss:
 * @buffer: a #GeglBuffer.
 *
 * Return the abyss extent of a buffer, this expands out to the parents extent in
 * subbuffers.
 */
const GeglRectangle * gegl_buffer_get_abyss   (GeglBuffer           *buffer);



/**
 * gegl_buffer_signal_connect:
 * @buffer: a GeglBuffer
 * @detailed_signal: only "changed" expected for now
 * @c_handler: (scope async) : c function callback
 * @data: user data:
 *
 * This function should be used instead of g_signal_connect when connecting to
 * the GeglBuffer::changed signal handler, GeglBuffer contains additional
 * machinery to avoid the overhead of changes when no signal handler have been
 * connected, if regular g_signal_connect is used; then no signals will be
 * emitted.
 *
 * Returns: an handle like g_signal_connect.
 */
glong gegl_buffer_signal_connect (GeglBuffer *buffer,
                                  const char *detailed_signal,
                                  GCallback   c_handler,
                                  gpointer    data);

#include <gegl-buffer-iterator.h>

G_END_DECLS
#endif
