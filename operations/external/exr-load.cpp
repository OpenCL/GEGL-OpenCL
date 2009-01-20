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
 *
 *
 * Copyright 2006 Dominik Ernst <dernst@gmx.de>
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_string (path, "File", "", "Path of file to load.")

#else

#define GEGL_CHANT_TYPE_SOURCE
#define GEGL_CHANT_C_FILE       "exr-load.cpp"

extern "C" {
#include "gegl-chant.h"
}

#include <ImfInputFile.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>
#include <ImfRgbaYca.h>
#include <ImfStandardAttributes.h>

#include <stdio.h>
#include <string.h>

using namespace Imf;
using namespace Imf::RgbaYca;
using namespace Imath;

enum
{
  COLOR_RGB    = 1<<1,
  COLOR_Y      = 1<<2,
  COLOR_C      = 1<<3,
  COLOR_ALPHA  = 1<<4,
  COLOR_U32    = 1<<5,
  COLOR_FP16   = 1<<6,
  COLOR_FP32   = 1<<7
};

static gfloat chroma_sampling[] =
  {
     0.002128,   -0.007540,
     0.019597,   -0.043159,
       0.087929,   -0.186077,
     0.627123,    0.627123,
    -0.186077,    0.087929,
    -0.043159,    0.019597,
    -0.007540,    0.002128,
  };


static gboolean
query_exr              (const gchar *path,
                        gint        *width,
                        gint        *height,
                        gint        *ff_ptr,
                        gpointer    *format);

static gboolean
import_exr             (GeglBuffer  *gegl_buffer,
                        const gchar *path,
                        gint         format_flags);

static void
convert_yca_to_rgba    (GeglBuffer *buf,
                        gint        has_alpha,
                        const V3f  &yw);

static void
reconstruct_chroma_row (gfloat *pixels,
                        gint    num,
                        gint    has_alpha,
                        gfloat *tmp);

static void
reconstruct_chroma     (GeglBuffer *buf,
                        gint        has_alpha);

static void
fix_saturation_row     (gfloat           *row_top,
                        gfloat           *row_middle,
                        gfloat           *row_bottom,
                        const Imath::V3f &yw,
                        gint              width,
                        gint              nc);

static void
fix_saturation         (GeglBuffer       *buf,
                        const Imath::V3f &yw,
                        gint              has_alpha);

static float
saturation             (const gfloat *in);

static void
desaturate             (const gfloat *in,
                        gfloat        f,
                        const V3f    &yw,
                        gfloat       *out,
                        int           has_alpha);

static void
insert_channels        (FrameBuffer  &fb,
                        const Header &header,
                        char         *base,
                        gint          width,
                        gint          format_flags,
                        gint          bpp);



/* The following functions saturation, desaturate, fix_saturation,
 * reconstruct_chroma_horiz, reconstruct_chroma_vert, convert_rgba_to_yca
 * are based upon their counterparts from the OpenEXR library, and are needed
 * since OpenEXR does not handle chroma subsampled 32-bit/per channel images.
 */

static float
saturation (const gfloat *in)
{
  float rgbMax = MAX (in[0], MAX(in[1], in[2]));
  float rgbMin = MIN (in[0], MIN(in[1], in[2]));

  if (rgbMax > 0)
    return 1 - rgbMin / rgbMax;
  else
    return 0;
}


static void
desaturate (const gfloat *in,
            gfloat        f,
            const V3f    &yw,
            gfloat       *out,
            int           has_alpha)
{
  float rgbMax = MAX (in[0], MAX (in[1], in[2]));

  out[0] = MAX (float (rgbMax - (rgbMax - in[0]) * f), 0.0f);
  out[1] = MAX (float (rgbMax - (rgbMax - in[1]) * f), 0.0f);
  out[2] = MAX (float (rgbMax - (rgbMax - in[2]) * f), 0.0f);
  if (has_alpha)
    out[3] = in[3];

  float Yin  = in[0]*yw.x  + in[1]*yw.y  + in[2]*yw.z;
  float Yout = out[0]*yw.x + out[1]*yw.y + out[2]*yw.z;

  if (Yout)
    {
      out[0] *= Yin / Yout;
      out[1] *= Yin / Yout;
      out[2] *= Yin / Yout;
    }
}


static void
fix_saturation_row (gfloat           *row_top,
                    gfloat           *row_middle,
                    gfloat           *row_bottom,
                    const Imath::V3f &yw,
                    gint              width,
                    gint              nc)
{
  static gint y=-1;
  gint x;
  const gfloat *neighbor1, *neighbor2, *neighbor3, *neighbor4;
  gfloat sMean, sMax, s;

  y++;

  for (x=0; x<width; x++)
    {
      neighbor1 = &row_top[x];
      neighbor2 = &row_bottom[x];

      if (x>0)
        neighbor3 = &row_middle[x-1];
      else
        neighbor3 = &row_middle[x];

      if (x < width-1)
        neighbor4 = &row_middle[x+1];
      else
        neighbor4 = &row_middle[x];

      sMean = MIN (1.0f, 0.25f * (saturation (neighbor1) +
                                  saturation (neighbor2) +
                                  saturation (neighbor3) +
                                  saturation (neighbor4) ));

      s = saturation (&row_middle[x]);
      sMax = MIN (1.0f, 1 - (1-sMean) * 0.25f);

      if (s > sMean && s > sMax)
        desaturate (&row_middle[x], sMax / s, yw, &row_middle[x], nc == 4);
    }
}


static void
fix_saturation (GeglBuffer       *buf,
                const Imath::V3f &yw,
                gint              has_alpha)
{
  gint y;
  const gint nc = has_alpha ? 4 : 3;
  gfloat *row[3], *tmp;
  GeglRectangle rect;
  gint pxsize;

  g_object_get (buf, "px-size", &pxsize, NULL);

  for (y=0; y<3; y++)
    row[y] = (gfloat*) g_malloc0 (pxsize * gegl_buffer_get_width (buf));

  for (y=0; y<2; y++)
    {
      gegl_rectangle_set (&rect, 0,y, gegl_buffer_get_width (buf), 1);
      gegl_buffer_get (buf, 1.0, &rect, NULL, row[y+1], GEGL_AUTO_ROWSTRIDE);
    }

  fix_saturation_row (row[1], row[1], row[2], yw, gegl_buffer_get_width (buf), nc);

  for (y=1; y<gegl_buffer_get_height (buf)-1; y++)
    {
      if (y>1)
        {
          gegl_rectangle_set (&rect, 0, y-2, gegl_buffer_get_width (buf), 1);
          gegl_buffer_set (buf, &rect, NULL, row[0], GEGL_AUTO_ROWSTRIDE);
        }

      gegl_rectangle_set (&rect, 0,y+1, gegl_buffer_get_width (buf), 1);
      gegl_buffer_get (buf, 1.0, &rect, NULL, row[0], GEGL_AUTO_ROWSTRIDE);

      tmp = row[0];
      row[0] = row[1];
      row[1] = row[2];
      row[2] = tmp;

      fix_saturation_row (row[0], row[1], row[2], yw, gegl_buffer_get_width (buf), nc);
    }

  fix_saturation_row (row[1], row[2], row[2], yw, gegl_buffer_get_width (buf), nc);

  for (y=gegl_buffer_get_height (buf)-2; y<gegl_buffer_get_height (buf); y++)
    {
      gegl_rectangle_set (&rect, 0, y, gegl_buffer_get_width (buf), 1);
      gegl_buffer_set (buf, &rect, NULL, row[y-gegl_buffer_get_height (buf)+2], GEGL_AUTO_ROWSTRIDE);
    }

  for (y=0; y<3; y++)
    g_free (row[y]);
}


static void
reconstruct_chroma_row (gfloat *pixels,
                        gint    num,
                        gint    has_alpha,
                        gfloat *tmp)
{
  gint x,i;
  gint nc = has_alpha ? 4 : 3;
  gfloat r,b;
  gfloat *pxl = pixels;

  for (x=0; x<num; x++)
    {
      if (x&1)
        {
          r = b = 0.0;
          for (i=-6; i<=6; i++)
            {
              if (x+(2*i-1) >= 0 && x+(2*i-1) < num)
                {
                  r += *(pxl+(2*i-1)*nc+1) * chroma_sampling[i+6];
                  b += *(pxl+(2*i-1)*nc+2) * chroma_sampling[i+6];
                }
            }
        }
      else
        {
          r = pxl[1];
          b = pxl[2];
        }

      pxl += nc;
      tmp[x*2]   = r;
      tmp[x*2+1] = b;
    }

  pxl = pixels;
  for (i=0; i<num; i++)
    memcpy (&pxl[i*nc+1], &tmp[i*2], sizeof(gfloat)*2);
}


static void
reconstruct_chroma (GeglBuffer *buf,
                    gint        has_alpha)
{
  gfloat *tmp, *pixels;
  gint i;
  GeglRectangle rect;
  gint pxsize;
  g_object_get (buf, "px-size", &pxsize, NULL);

  pixels = (gfloat*) g_malloc0 (MAX(gegl_buffer_get_width (buf), gegl_buffer_get_height (buf))*pxsize);
  tmp = (gfloat*) g_malloc0 (MAX(gegl_buffer_get_width (buf), gegl_buffer_get_height (buf))*2*sizeof(gfloat));

  for (i=0; i<gegl_buffer_get_height (buf); i+=2)
    {
      gegl_rectangle_set (&rect, 0, i,  gegl_buffer_get_width (buf), 1);
      gegl_buffer_get (buf, 1.0, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);

      reconstruct_chroma_row (pixels, gegl_buffer_get_width (buf), has_alpha, tmp);
      gegl_buffer_set (buf, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    }

  for (i=0; i<gegl_buffer_get_width (buf); i++)
    {
      gegl_rectangle_set (&rect, i, 0, 1, gegl_buffer_get_height (buf));
      gegl_buffer_get (buf, 1.0, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);

      reconstruct_chroma_row (pixels, gegl_buffer_get_height (buf), has_alpha, tmp);
      gegl_buffer_set (buf, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (tmp);
  g_free (pixels);
}


static void
convert_yca_to_rgba (GeglBuffer *buf,
                     gint        has_alpha,
                     const V3f  &yw)
{
  gchar *pixels;
  gfloat r,g,b, y, ry, by, *pxl;
  gint row, i, dx = has_alpha ? 4 : 3;
  GeglRectangle rect;
  gint pxsize;
  g_object_get (buf, "px-size", &pxsize, NULL);

  pixels = (gchar*) g_malloc0 (gegl_buffer_get_width (buf) * pxsize);

  for (row=0; row<gegl_buffer_get_height (buf); row++)
    {
      gegl_rectangle_set (&rect, 0, row, gegl_buffer_get_width (buf), 1);
      gegl_buffer_get (buf, 1.0, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
      pxl = (gfloat*) pixels;

      for (i=0; i<gegl_buffer_get_width (buf); i++)
        {
          y  = pxl[0];
          ry = pxl[1];
          by = pxl[2];

          r = y*(ry+1.0);
          b = y*(by+1.0);
          g = (y - r*yw.x - b*yw.z) / yw.y;

          pxl[0] = r;
          pxl[1] = g;
          pxl[2] = b;

          pxl += dx;
        }

      gegl_buffer_set (buf, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
    }

  g_free (pixels);
}


static void
insert_channels (FrameBuffer  &fb,
                 const Header &header,
                 char         *base,
                 gint          width,
                 gint          format_flags,
                 gint          bpp)
{
  gint alpha_offset = 12;
  PixelType tp;

  if (format_flags & COLOR_U32)
    tp = UINT;
  else
    tp = FLOAT;

  if (format_flags & COLOR_RGB)
    {
      fb.insert ("R", Slice (tp, base,    bpp, 0, 1,1, 0.0));
      fb.insert ("G", Slice (tp, base+4,  bpp, 0, 1,1, 0.0));
      fb.insert ("B", Slice (tp, base+8,  bpp, 0, 1,1, 0.0));
    }
  else if (format_flags & COLOR_C)
    {
      fb.insert ("Y",  Slice (tp, base,   bpp,   0, 1,1, 0.5));
      fb.insert ("RY", Slice (tp, base+4, bpp*2, 0, 2,2, 0.0));
      fb.insert ("BY", Slice (tp, base+8, bpp*2, 0, 2,2, 0.0));
    }
  else if (format_flags & COLOR_Y)
    {
      fb.insert ("Y",  Slice (tp, base, bpp, 0, 1,1, 0.5));
      alpha_offset = 4;
    }

  if (format_flags & COLOR_ALPHA)
    fb.insert ("A", Slice (tp, base+alpha_offset, bpp, 0, 1,1, 1.0));
}


static gboolean
import_exr (GeglBuffer  *gegl_buffer,
            const gchar *path,
            gint         format_flags)
{
  try
    {
      InputFile file (path);
      FrameBuffer frameBuffer;
      Box2i dw = file.header().dataWindow();
      gint pxsize;

      g_object_get (gegl_buffer, "px-size", &pxsize, NULL);


      char *pixels = (char*) g_malloc0 (gegl_buffer_get_width (gegl_buffer) * pxsize);

      char *base = pixels;

      /*
       * The pointer we pass to insert_channels needs to be adjusted, since
       * our buffer always starts at the position where the first pixels
       * occurs, which may be a position not equal to (0 0). OpenEXR expects
       * the pointer to point to (0 0), which may be outside our buffer, but
       * that is needed so that OpenEXR writes all pixels to the correct
       * position in our buffer.
       */
      base -= pxsize * dw.min.x;

      insert_channels (frameBuffer,
                       file.header(),
                       base,
                       gegl_buffer_get_width (gegl_buffer),
                       format_flags,
                       pxsize);

      file.setFrameBuffer (frameBuffer);

      {
        gint i;
        GeglRectangle rect;

        for (i=dw.min.y; i<=dw.max.y; i++)
          {
            gegl_rectangle_set (&rect, 0, i-dw.min.y,gegl_buffer_get_width (gegl_buffer), 1);
            file.readPixels (i);
            gegl_buffer_set (gegl_buffer, &rect, NULL, pixels, GEGL_AUTO_ROWSTRIDE);
          }
      }

      if (format_flags & COLOR_C)
        {
          Chromaticities cr;
          V3f yw;

          if (hasChromaticities(file.header()))
            cr = chromaticities (file.header());

          yw = computeYw (cr);

          reconstruct_chroma (gegl_buffer, format_flags & COLOR_ALPHA);
          convert_yca_to_rgba (gegl_buffer,
                               format_flags & COLOR_ALPHA,
                               yw);

          fix_saturation (gegl_buffer, yw, format_flags & COLOR_ALPHA);
        }

      g_free (pixels);
    }
  catch (...)
    {
      g_warning ("failed to load `%s'", path);
      return FALSE;
    }
  return TRUE;
}


static gboolean
query_exr (const gchar *path,
           gint        *width,
           gint        *height,
           gint        *ff_ptr,
           gpointer    *format)
{
  gchar format_string[16];
  gint format_flags = 0;

  try
    {
      InputFile file (path);
      Box2i dw = file.header().dataWindow();
      const ChannelList& ch = file.header().channels();
      const Channel *chan;
      PixelType pt;

      *width  = dw.max.x - dw.min.x + 1;
      *height = dw.max.y - dw.min.y + 1;

      if (ch.findChannel ("R") || ch.findChannel ("G") || ch.findChannel ("B"))
        {
          strcpy (format_string, "RGB");
          format_flags = COLOR_RGB;

          if ((chan = ch.findChannel ("R")))
            pt = chan->type;
          else if ((chan = ch.findChannel ("G")))
            pt = chan->type;
          else
            pt = ch.findChannel ("B")->type;
        }
      else if (ch.findChannel ("Y") &&
               (ch.findChannel("RY") || ch.findChannel("BY")))
        {
          strcpy (format_string, "RGB");
          format_flags = COLOR_Y | COLOR_C;

          pt = ch.findChannel ("Y")->type;
        }
      else if (ch.findChannel ("Y"))
        {
          strcpy (format_string, "Y");
          format_flags = COLOR_Y;
          pt = ch.findChannel ("Y")->type;
        }
      else
        {
          g_warning ("color type mismatch");
          return FALSE;
        }

      if (ch.findChannel ("A"))
        {
          strcat (format_string, "A");
          format_flags |= COLOR_ALPHA;
        }

      switch (pt)
        {
          case UINT:
            format_flags |= COLOR_U32;
            strcat (format_string, " u32");
            break;
          case HALF:
          case FLOAT:
          default:
            format_flags |= COLOR_FP32;
            strcat (format_string, " float");
            break;
        }
    }
  catch (...)
    {
      g_warning ("can't query `%s'. is this really an EXR file?", path);
      return FALSE;
    }

  *ff_ptr = format_flags;
  *format = babl_format (format_string);
  return TRUE;
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = {0, 0, 10, 10};
  gint          w, h, ff;
  gpointer      format;

  if (query_exr (o->path, &w, &h, &ff, &format))
    {
      result.width = w;
      result.height = h;
      gegl_operation_set_format (operation, "output", (Babl*)format);
    }

  return result;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gint        w,h,ff;
  gpointer    format;
  gboolean    ok;

  ok = query_exr (o->path, &w, &h, &ff, &format);

  if (ok)
    {
      import_exr (output, o->path, ff);
    }
  else
    {
      return FALSE;
    }

  return TRUE;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  return get_bounding_box (operation);
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationSourceClass *source_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  source_class    = GEGL_OPERATION_SOURCE_CLASS (klass);

  source_class->process = process;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->get_cached_region = get_cached_region;
  operation_class->name        = "gegl:exr-load";
  operation_class->categories  = "hidden";
  operation_class->description = "EXR image loader.";

  gegl_extension_handler_register (".exr", "gegl:exr-load");
  gegl_extension_handler_register (".EXR", "gegl:exr-load");
}

#endif

