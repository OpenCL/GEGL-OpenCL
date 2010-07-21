
/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 */


#ifndef __RGBE_H__
#define __RGBE_H__

#include <glib.h>

typedef struct _rgbe_file rgbe_file;

/**
 * rgbe_save_path:
 * @param path:   the path to write the rgbe file to
 * @param width:  the width of the image
 * @param height: the height of the image
 * @param pixels: RGB floating point pixel data
 *
 * Writes raw pixel data into an RGBE format file. The pixel data should be
 * 'width x height' elements long, and consist of RGB floats.
 *
 * Returns TRUE on success.
 */
gboolean           rgbe_save_path      (const gchar *path,
                                        guint        width,
                                        guint        height,
                                        gfloat      *pixels);


/**
 * rgbe_load_path:
 * @param path: the path to an RGBE format image file
 *
 * Reads the metadata for an RGBE formatted image, and allows future access
 * to the pixel data and some elements of the image metadata (see
 * rgbe_get_size and rgbe_read_scanlines).
 *
 * The caller should use rgbe_file_free when finished with the file structure.
 *
 * Returns NULL on failure.
 */
rgbe_file        * rgbe_load_path      (const gchar *path);


/**
 * rgbe_file_free:
 * @param file: file structure to dispose of
 *
 * Destroy resources associated with an RGBE file. The pointer will become
 * invalid after this operation. It is safe to pass a NULL pointer to this
 * function.
 */
void               rgbe_file_free      (rgbe_file *file);


/**
 * rgbe_get_size:
 * @param file: the image file to query
 * @param x:    pointer to store the width
 * @param y:    pointer to store the height
 *
 * Query the image for its width and height.
 *
 * Returns TRUE on success.
 */
gboolean           rgbe_get_size       (rgbe_file *file,
                                        guint     *x,
                                        guint     *y);


/**
 * rgbe_read_scanlines:
 * @param file: an open image structure to read from
 *
 * Returns the image's RGBA formatted pixel data. It is the user's
 * responsibility to free the memory when finished.
 */
gfloat *           rgbe_read_scanlines (rgbe_file *file);

#endif /* __RGBE_H__ */
