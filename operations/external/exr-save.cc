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
 * Copyright 2011 Rasmus Hahn <rassahah@googlemail.com>
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string  (path, "File", "", "path of file to write to.")
gegl_chant_int  (tile, "Tile", 0, 2048, 0, "tile size to use.")

#else

#define GEGL_CHANT_TYPE_SINK
#define GEGL_CHANT_C_FILE "exr-save.cc"

extern "C" {
#include "gegl-chant.h"
} /* extern "C" */

#include "config.h"
#include <exception>
#include <ImfTiledOutputFile.h>
#include <ImfOutputFile.h>
#include <ImfChannelList.h>

/**
 * create an Imf::Header for writing up to 4 channels (given in d).
 * d must be between 1 and 4.
 */
static Imf::Header
create_header (int w,
               int h,
               int d)
{
  Imf::Header header (w, h);
  Imf::FrameBuffer fbuf;

  if (d <= 2)
    {
      header.channels ().insert ("Y", Imf::Channel (Imf::FLOAT));
    }
  else
    {
      header.channels ().insert ("R", Imf::Channel (Imf::FLOAT));
      header.channels ().insert ("G", Imf::Channel (Imf::FLOAT));
      header.channels ().insert ("B", Imf::Channel (Imf::FLOAT));
    }
  if (d == 2 || d == 4)
    {
      header.channels ().insert ("A", Imf::Channel (Imf::FLOAT));
    }
  return header;
}

/**
 * create an Imf::FrameBuffer object for w*h*d floats and return it.
 */
static Imf::FrameBuffer
create_frame_buffer (int          w,
                     int          h,
                     int          d,
                     const float *data)
{
  Imf::FrameBuffer fbuf;

  if (d <= 2)
    {
      fbuf.insert ("Y", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
    }
  else
    {
      fbuf.insert ("R", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 0),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("G", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 1),
          d * sizeof *data, d * sizeof *data * w));
      fbuf.insert ("B", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + 2),
          d * sizeof *data, d * sizeof *data * w));
    }
  if (d == 2 || d == 4)
    {
      fbuf.insert ("A", Imf::Slice (Imf::FLOAT, (char *) (&data[0] + (d - 1)),
          d * sizeof *data, d * sizeof *data * w));
    }
  return fbuf;
}

/**
 * write the float buffer to an exr file with tile-size tw x th.
 * The buffer must contain w*h*d floats.
 * Currently supported are only values 1-4 for d:
 * d=1: write a single Y-channel.
 * d=2: write Y and A.
 * d=3: write RGB.
 * d=4: write RGB and A.
 */
static void
write_tiled_exr (const float       *pixels,
                 int                w,
                 int                h,
                 int                d,
                 int                tw,
                 int                th,
                 const std::string &filename)
{
  Imf::Header header (create_header (w, h, d));
  header.setTileDescription (Imf::TileDescription (tw, th, Imf::ONE_LEVEL));
  Imf::TiledOutputFile out (filename.c_str (), header);
  Imf::FrameBuffer fbuf (create_frame_buffer (w, h, d, pixels));
  out.setFrameBuffer (fbuf);
  out.writeTiles (0, out.numXTiles () - 1, 0, out.numYTiles () - 1);
}

/**
 * write an openexr file in scanline mode.
 * pixels contains the image data as an array of w*h*d floats.
 * The data is written to the file named filename.
 */
static void
write_scanline_exr (const float       *pixels,
                    int                w,
                    int                h,
                    int                d,
                    const std::string &filename)
{
  Imf::Header header (create_header (w, h, d));
  Imf::OutputFile out (filename.c_str (), header);
  Imf::FrameBuffer fbuf (create_frame_buffer (w, h, d, pixels));
  out.setFrameBuffer (fbuf);
  out.writePixels (h);
}

/**
 * write the given pixel buffer, which is w * h * d to filename using the
 * tilesize as tile width and height. This is the only function calling
 * the openexr lib and therefore should be exception save.
 */
static void
exr_save_process (const float       *pixels,
                  int                w,
                  int                h,
                  int                d,
                  int                tile_size,
                  const std::string &filename)
{
  if (tile_size == 0)
    {
      /* write a scanline exr image. */
      write_scanline_exr (pixels, w, h, d, filename);
    }
  else
    {
      /* write a tiled exr image. */
      write_tiled_exr (pixels, w, h, d, tile_size, tile_size, filename);
    }
}

/**
 * main entry function for the exr saver.
 */
static gboolean
gegl_exr_save_process (GeglOperation       *operation,
                       GeglBuffer          *input,
                       const GeglRectangle *rect,
                       gint                 level)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  std::string filename (o->path);
  std::string output_format;
  int tile_size (o->tile);
  /*
   * determine the number of channels in the input buffer and determine
   * format, data is written with. Currently this only checks for the
   * numbers of channels and writes only Y or RGB data with optional alpha.
   */
  const Babl *original_format = gegl_buffer_get_format (input);
  unsigned depth = babl_format_get_n_components (original_format);
  switch (depth)
    {
      case 1: output_format = "Y float";    break;
      case 2: output_format = "YA float";   break;
      case 3: output_format = "RGB float";  break;
      case 4: output_format = "RGBA float"; break;
      default:
        g_warning ("exr-save: cannot write exr with depth %d.", depth);
        return FALSE;
        break;
    }
  /*
   * get the pixel data. The position of the rectangle is effectively
   * ignored. Always write a file width x height; @todo: check if exr
   * can set the origin.
   */
  float *pixels
    = (float *) g_malloc (rect->width * rect->height * depth * sizeof *pixels);
  if (pixels == 0)
    {
      g_warning ("exr-save: could allocate %d*%d*%d pixels.",
        rect->width, rect->height, depth);
      return FALSE;
    }
  gegl_buffer_get (input, rect, 1.0, babl_format (output_format.c_str ()),
                   pixels, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  bool status;
  try
    {
      exr_save_process (pixels, rect->width, rect->height,
                        depth, tile_size, filename);
      status = TRUE;
    }
  catch (std::exception &e)
    {
      g_warning ("exr-save: failed to write to '%s': %s",
         filename.c_str (), e.what ());
      status = FALSE;
    }
  g_free (pixels);
  return status;
}

static void gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass *operation_class;
  GeglOperationSinkClass *sink_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  sink_class      = GEGL_OPERATION_SINK_CLASS (klass);

  sink_class->process = gegl_exr_save_process;
  sink_class->needs_full = TRUE;

  gegl_operation_class_set_keys (operation_class,
    "name"        , "gegl:exr-save",
    "categories"  , "output",
    "description" , "OpenEXR image saver",
    NULL);

  gegl_extension_handler_register_saver (".exr", "gegl:exr-save");
}

#endif

