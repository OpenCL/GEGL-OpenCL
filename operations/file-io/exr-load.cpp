/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2006 Dominik Ernst <dernst@gmx.de>
 */

#if GEGL_CHANT_PROPERTIES

gegl_chant_string (path, "", "Path of file to load.")

#else

#define GEGL_CHANT_SOURCE
#define GEGL_CHANT_NAME         exr_load
#define GEGL_CHANT_DESCRIPTION  "EXR image loader."
#define GEGL_CHANT_SELF         "exr-load.cpp"
#define GEGL_CHANT_CATEGORIES   "hidden"
#define GEGL_CHANT_CLASS_INIT

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
query_exr (const gchar *path,
           gint        *width,
           gint        *height,
           gint        *ff_ptr,
           gpointer    *format);

static gboolean
import_exr (GeglBuffer  *gegl_buffer,
            const gchar *path,
            gint         format_flags);

static void
convert_yca_to_rgba (char      *pixels,
                     gint       num_pixels,
                     gint       has_alpha,
                     const V3f &yw);


static float
saturation (const gfloat *in);

static void
desaturate (const gfloat *in, 
            gfloat        f, 
            const V3f    &yw, 
            gfloat       *out,
            int           has_alpha);

static void 
fix_saturation (const Imath::V3f &yw,
                gint              width,
                gint              height,
                const gfloat     *rgba_in,
                gfloat           *rgba_out,
                gint              has_alpha );

static void
reconstruct_chroma_horiz (char *pixels,
                          gint  width,
                          gint  height,
                          gint  has_alpha);

static void
reconstruct_chroma_horiz (char *pixels,
                          gint  width,
                          gint  height,
                          gint  has_alpha);


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
fix_saturation (const Imath::V3f &yw,
                gint              width,
                gint              height,
                const gfloat     *rgba_in,
                gfloat           *rgba_out,
                gint              has_alpha )
{
  gint x,y;
  gint nc = has_alpha ? 4 : 3;
  gint rowstride = width*nc;
  const gfloat *neighbor1, *neighbor2, *neighbor3, *neighbor4;

  for (y=0; y<height; y++)
    {
      for (x=0; x<width; x++)
        {
          if (y-1 >= 0)
            neighbor1 = rgba_in - rowstride;
          else
            neighbor1 = rgba_in;

          if (y+1 < height)
            neighbor2 = rgba_in + rowstride;
          else
            neighbor2 = rgba_in;

          if (x-1 >= 0)
            neighbor3 = rgba_in - nc;
          else
            neighbor3 = rgba_in;

          if (x+1 < width)
            neighbor4 = rgba_in + nc;
          else
            neighbor4 = rgba_in;

          float sMean = MIN (1.0f, 0.25f * (saturation (neighbor1) +
                                            saturation (neighbor2) +
                                            saturation (neighbor3) +
                                            saturation (neighbor4)));

          float s = saturation (rgba_in);
          float sMax = MIN (1.0f, 1 - (1 - sMean) * 0.25f);

          if (s > sMean && s > sMax)
            desaturate (rgba_in, sMax / s, yw, rgba_out, has_alpha);
          else
            memcpy (rgba_out, rgba_in, sizeof(gfloat)*nc);

          rgba_out += nc;
          rgba_in  += nc;
        }
    }
}

static void
reconstruct_chroma_horiz (char *pixels,
                          gint  width,
                          gint  height,
                          gint  has_alpha)
{
  gint x,x2,y, i;
  gfloat r,b;
  gint nc = has_alpha ? 4 : 3;
  gfloat *pxl = (gfloat*)pixels;
  gfloat *tmp = (gfloat*) g_malloc0 (sizeof(gfloat)*width*2);

  for (y=0; y<height/2; y++)
    {
      for (x=x2=0; x<width; x++)
        {
          if (x&1)
            {
              r = b = 0.0;
              for (i=-6; i<=6; i++)
                {
                  if (x2+i >= 0 && x2+i < width)
                    {
                      r += *(pxl+i*nc+1) * chroma_sampling[i+6];
                      b += *(pxl+i*nc+2) * chroma_sampling[i+6];
                    }
                }
              x2++;
            }
          else
            {
              r = pxl[1];
              b = pxl[2];
              pxl += nc;
            }

          tmp[x*2]   = r;
          tmp[x*2+1] = b;
        }

      pxl = ((gfloat*)pixels)+width*nc*y;
      for (i=0; i<width; i++)
        memcpy (&pxl[i*nc+1], &tmp[i*2], sizeof(gfloat)*2);
      pxl += nc*width;
    }

  g_free (tmp);
}

static void
reconstruct_chroma_vert (gchar *pixels,
                         gint   width,
                         gint   height,
                         gint   has_alpha)
{
  gint x,y,y2, i;
  gint nc = has_alpha ? 4 : 3;
  gint rowstride = width*nc;
  gfloat r,b;
  gfloat *tmp = (gfloat*) g_malloc0 (sizeof(gfloat)*height*2);
  gfloat *pxl;

  for (x=0; x<width; x++)
    {
      pxl = ((gfloat*)pixels)+x*nc;

      for (y=y2=0; y<height; y++)
        {
          if (y&1)
            {
              r = b = 0.0;
              for (i=-6; i<=6; i++)
                {
                  if (y2+i >= 0 && y2+i < height)
                    {
                      r += *(pxl+i*rowstride+1) * chroma_sampling[i+6];
                      b += *(pxl+i*rowstride+2) * chroma_sampling[i+6];
                    }
                }
              y2++;
            }
          else
            {
              r = pxl[1];
              b = pxl[2];
              pxl += rowstride;
            }

          tmp[y*2]   = r;
          tmp[y*2+1] = b;
        }

        pxl = ((gfloat*)pixels)+x*nc;
        for (i = 0; i<height; i++)
            memcpy (&pxl[i*rowstride+1], &tmp[i*2], sizeof(gfloat)*2);
            
    }
}

static void
convert_yca_to_rgba (char   *pixels,
                     gint    num_pixels,
                     gint    has_alpha,
                     const V3f &yw)
{
  gfloat r,g,b, y,ry,by;
  gfloat *pxl = (gfloat*)pixels;
  gint i, dx = has_alpha ? 4 : 3;

  for (i=0; i<num_pixels; i++)
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
      fb.insert ("R", Slice (tp, base,    bpp, bpp*width, 1,1, 0.0));
      fb.insert ("G", Slice (tp, base+4,  bpp, bpp*width, 1,1, 0.0));
      fb.insert ("B", Slice (tp, base+8,  bpp, bpp*width, 1,1, 0.0));
    }
  else if (format_flags & COLOR_C)
    {
      fb.insert ("Y",  Slice (tp, base,   bpp, bpp*width, 1,1, 0.5));
      fb.insert ("RY", Slice (tp, base+4, bpp, bpp*width, 2,2, 0.0));
      fb.insert ("BY", Slice (tp, base+8, bpp, bpp*width, 2,2, 0.0));
    }
  else if (format_flags & COLOR_Y)
    {
      fb.insert ("Y",  Slice (tp, base, bpp, bpp*width, 1,1, 0.5));
      alpha_offset = 4;
    }

  if (format_flags & COLOR_ALPHA)
    fb.insert ("A", Slice (tp, base+alpha_offset, bpp, bpp*width, 1,1, 1.0));
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

      char *pixels = (char*) g_malloc0 (gegl_buffer_pixels (gegl_buffer) * 
                                        gegl_buffer_px_size (gegl_buffer));
      char *base = pixels;

      /*
       * The pointer we pass to insert_channels needs to be adjusted, since
       * our buffer always starts at the position where the first pixels
       * occurs, which may be a position not equal to (0 0). OpenEXR expects
       * the pointer to point to (0 0), which may be outside our buffer, but 
       * that is needed so that OpenEXR writes all pixels to the correct
       * position in our buffer.
       */
      base -= gegl_buffer_px_size (gegl_buffer)*dw.min.x;
      base -= gegl_buffer_px_size (gegl_buffer)*gegl_buffer->width*dw.min.y;

      insert_channels (frameBuffer, 
                       file.header(), 
                       base, 
                       gegl_buffer->width,
                       format_flags,
                       gegl_buffer_px_size (gegl_buffer));

      file.setFrameBuffer (frameBuffer);
      file.readPixels (dw.min.y, dw.max.y);

      if (format_flags & COLOR_C)
        {
          Chromaticities cr;
          V3f yw;
  
          if (hasChromaticities(file.header()))
            cr = chromaticities (file.header());

          yw = computeYw (cr);

          reconstruct_chroma_horiz (pixels, 
                                    gegl_buffer->width,
                                    gegl_buffer->height,
                                    format_flags & COLOR_ALPHA);
          reconstruct_chroma_vert (pixels,
                                   gegl_buffer->width,
                                   gegl_buffer->height,
                                   format_flags & COLOR_ALPHA);

          convert_yca_to_rgba (pixels, 
                               gegl_buffer_pixels(gegl_buffer),
                               format_flags & COLOR_ALPHA,
                               yw);

          gchar *fs = (gchar*) g_malloc0 (gegl_buffer_pixels (gegl_buffer) *
                                            gegl_buffer_px_size (gegl_buffer));
          fix_saturation (yw, 
                          gegl_buffer->width, 
                          gegl_buffer->height, 
                          (const gfloat*) pixels, 
                          (gfloat*) fs, 
                          format_flags & COLOR_ALPHA);

          g_free (pixels);
          pixels = fs;
        }

      gegl_buffer_set (gegl_buffer, NULL, pixels, NULL);
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

static gboolean
process (GeglOperation *operation,
         gpointer       dynamic_id)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglBuffer *output;

  {
    gint w,h,ff;
    gpointer format;
    gboolean ok;

    ok = query_exr (self->path, &w, &h, &ff, &format);
    if (!ok)
      {
        output = GEGL_BUFFER (g_object_new (GEGL_TYPE_BUFFER,
                                           "format", babl_format ("R'G'B'A u8"),
                                           "x",      0,
                                           "y",      0,
                                           "width",  10,
                                           "height", 10,
                                           NULL));
      }
    else
      {
        output = GEGL_BUFFER (g_object_new (GEGL_TYPE_BUFFER,
                                            "format", format,
                                            "x",      0,
                                            "y",      0,
                                            "width",  w,
                                            "height", h,
                                            NULL));
        import_exr (output, self->path, ff);
      }

  }
  gegl_operation_set_data (operation, dynamic_id, "output", G_OBJECT (output));
  return TRUE;
}

static GeglRectangle
get_defined_region (GeglOperation *operation)
{
  GeglChantOperation *self = GEGL_CHANT_OPERATION (operation);
  GeglRectangle result = {0, 0, 10, 10};
  gint w,h,ff;
  gpointer format;

  if (query_exr (self->path, &w, &h, &ff, &format))
    {
      result.w = w;
      result.h = h;
    }

  return result;
}

static void class_init (GeglOperationClass *operation_class)
{
  static gboolean done=FALSE;
  if (done)
    return;
  gegl_extension_handler_register (".exr", "exr-load");
  gegl_extension_handler_register (".EXR", "exr-load");
  done = TRUE;
}


#endif

