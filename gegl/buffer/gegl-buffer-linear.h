#ifndef __GEGL_BUFFER_LINEAR_H
#define __GEGL_BUFFER_LINEAR_H

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
 * @width: the width of the buffer
 * @height: the height of the buffer
 * @rowstride: the number of bytes between rowstarts in memory (or 0 to
 *             autodetect)
 * @destory_fn: function to call to free data or NULL if memory should not be
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
 * @roi: region to open, pass NULL for entire buffer.
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

#endif
