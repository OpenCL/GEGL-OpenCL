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
 * Copyright Dov Grobgeld 2013 <dov.grobgeld (a) gmail.com>
 *
 * This operation saves a buffer in the npy file format. It may be
 * read into python as follows:
 *
 *   import numpy
 *   img = numpy.load('image.npy')
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_PROPERTIES

property_file_path (path, _("File"), "")
    description (_("Target path and filename, use '-' for stdout."))

#else

#define GEGL_OP_SINK
#define GEGL_OP_NAME npy_save
#define GEGL_OP_C_SOURCE npy-save.c

#include "gegl-op.h"
#include <stdio.h>

static int npywrite_header(FILE *fp, int width, int height, int num_channels)
{
  const gchar* format;
  gsize header_len;
  gchar *header;

  // Write header and version number to file
  fwrite("\223NUMPY"
         "\001\000"
         , 1, 8, fp);
  
  
  if (num_channels == 3)
    format = "{'descr': '<f4', 'fortran_order': False, 'shape': (%d, %d, 3), } \n";
  else
    format = "{'descr': '<f4', 'fortran_order': False, 'shape': (%d, %d), } \n";
  
  header = g_strdup_printf(format, height, width);
  header_len = strlen(header);
  fwrite(&header_len, 2, 1, fp);
  fwrite(header, header_len, 1, fp);
  g_free(header);
  
  return 0;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         const GeglRectangle *rect,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);

  FILE     *fp;
  guchar   *data;
  gsize     bpc;
  gsize     numbytes_scanline;
  gsize     numchannels;
  gboolean  ret = FALSE;
  gint      row;
  gint      slice_thickness = 32;
  const Babl *output_format;
  const Babl *input_format = gegl_buffer_get_format(input); 

  // Get the current format and use it to decide whether to save
  // the output in color or gray level formats.
  bpc = sizeof(gfloat);
  if (babl_format_get_n_components(input_format) >= 3)
    {
      numchannels = 3;
      output_format = babl_format("RGB float");
    }
  else
    {
      numchannels = 1;
      output_format = babl_format ("Y float");
    }

  numbytes_scanline = rect->width * numchannels * bpc;

  fp = (!strcmp (o->path, "-") ? stdout : fopen(o->path, "wb") );

  npywrite_header(fp, rect->width, rect->height, numchannels);

  data = g_malloc (numbytes_scanline * slice_thickness);
  
  for (row=0; row < rect->height; row+= slice_thickness)
    {
      GeglRectangle rect_slice;
      rect_slice.x = rect->x;
      rect_slice.width = rect->width;
      rect_slice.y = rect->y+row;
      rect_slice.height = MIN(slice_thickness, rect->height-row);
      
      gegl_buffer_get (input, &rect_slice, 1.0, output_format, data,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      fwrite(data, numbytes_scanline, rect_slice.height, fp);
    }

  g_free (data);

  return ret;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass     *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name",       "gegl:npy-save",
    "title",      _("NPY File Saver"),
    "categories", "output",
    "description",
        _("NPY image saver (Numerical python file saver.)"),
        NULL);

  gegl_operation_handlers_register_saver (
    ".npy", "gegl:npy-save");
}

#endif
