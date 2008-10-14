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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2008 Bradley Broom <bmbroom@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_pointer (lens_info_pointer, _("Model"),
                    _("Pointer to LensCorrectionModel"))

#else

#define GEGL_CHANT_TYPE_FILTER
#define GEGL_CHANT_C_FILE       "lens-correct.c"

#include "gegl-chant.h"
#include <math.h>
#include "lens-correct.h"

#if 0
#define TRACE       /* Define this to see basic tracing info. */
#endif

/* Find the src pixel that maps to the destination pixel passed as parameters x and y.
 */
static void
find_src_pixel (LensCorrectionModel *lcip, ChannelCorrectionModel *pp,
                gfloat x, gfloat y, gfloat *srcx, gfloat *srcy)
{
  double r, radj;

  r = hypot (x - lcip->cx, y - lcip->cy) / lcip->rscale;
  radj = (((pp->a*r+pp->b)*r+pp->c)*r+pp->d);
  *srcx = (x - lcip->cx) * radj + lcip->cx;
  *srcy = (y - lcip->cy) * radj + lcip->cy;
}

/* Find dst pixel that comes from the source pixel passed as parameters x and y.
 */
static void
find_dst_pixel (LensCorrectionModel *lcip, ChannelCorrectionModel *pp,
        gfloat x, gfloat y, gfloat *dstx, gfloat *dsty)
{
    /* Compute scaled distance of pixel from image center. */
    gfloat srcr = hypot (x - lcip->cx, y - lcip->cy) / lcip->rscale;

    /* Main path doesn't work for srcr == 0, but no need if pixel is close
     * to image center.
     */
    if (srcr < 0.01/lcip->rscale) {
        *dstx = x;
    *dsty = y;
    }
    else {
    gfloat dstr = srcr;
        gfloat r;
    #ifdef COUNT_IT
      int it = 0;
    #endif

    /* Adjust dstr until r is within about one hundred of a pixel of srcr.
     * Usually, one iteration is enough.
     */
    r = (((pp->a*dstr+pp->b)*dstr+pp->c)*dstr+pp->d) * dstr;
    while (fabs (r - srcr) > 0.01/lcip->rscale) {
        /* Compute derivative of lens distortion function at dstr. */
            gfloat dr = ((4.0*pp->a*dstr+3.0*pp->b)*dstr+2.0*pp->c)*dstr+pp->d;
        if (fabs(dr) < 1.0e-5) {
        /* In theory, the derivative of a quartic function can have zeros,
         * but I don't think any valid lens correction model will.
         * Still, we check to avoid an infinite loop if the ChannelCorrectionModel is broken.
         */
            g_warning ("find_dst_pixel: x=%g y=%g found zero slope, bailing out", x, y);
        break;
        }
        /* Adjust dstr based on error distance and current derivative. */
        dstr += (srcr-r)/dr;
        #ifdef COUNT_IT
          it++;
        #endif
            r = (((pp->a*dstr+pp->b)*dstr+pp->c)*dstr+pp->d) * dstr;
    }
    #ifdef COUNT_IT
      g_warning ("find_dst_pixel: iterations == %d", it);
    #endif

    /* Compute dst x,y coords from change in radial distance from image center. */
    *dstx = (x - lcip->cx) * (dstr/srcr) + lcip->cx;
    *dsty = (y - lcip->cy) * (dstr/srcr) + lcip->cy;
    }
}

/* Define the type of the above two functions.
 */
typedef void pixel_map_func (LensCorrectionModel *lcip, ChannelCorrectionModel *pp,
                 gfloat x, gfloat y, gfloat *dstx, gfloat *dsty);

/* Compute the bounding box of the GeglRectangle, *in_rect,
 * when passed through the pixel mapping function given by *mappix,
 * using the LensCorrectionModel *lcip.
 *
 * The implementation assumes that the LensCorrectionModel is monotonic.
 */
static GeglRectangle
map_region (const GeglRectangle *in_rect, LensCorrectionModel *lcip, pixel_map_func *mappix)
{
  GeglRectangle result = *in_rect;

  #ifdef TRACE
    g_warning ("> map_region in_rect=%dx%d+%d+%d", in_rect->width, in_rect->height, in_rect->x, in_rect->y);
  #endif

  if (result.width != 0 && result.height != 0)
    {
      gint ii;
      gfloat x, y;
      gfloat minx, miny, maxx, maxy;

      /* To find the bounding box of the mapped pixels, we go around the
       * perimeter of in_rect mapping each pixel.  By keeping track of
       * the min/max x/y coordinates thus obtained, we compute the bounding box.
       */
      (*mappix) (lcip, &lcip->green, in_rect->x, in_rect->y, &minx, &miny);
      maxx = minx; maxy = miny;
      for (ii = 0; ii < 2*(in_rect->width+in_rect->height); ii++) {
    /* Compute srcx,srcy coordinate for this point on the perimeter. */
    gint srcx = in_rect->x;
    gint srcy = in_rect->y;
    if (ii < in_rect->width)
      {
        srcx += ii;
      }
    else if (ii < in_rect->width + in_rect->height)
      {
        srcx += in_rect->width;
        srcy += (ii - in_rect->width);
      }
    else if (ii < 2*in_rect->width + in_rect->height)
      {
        srcy += in_rect->height;
        srcx += 2*in_rect->width + in_rect->height - ii;
      }
    else
      {
        srcy += 2*(in_rect->width + in_rect->height) - ii;
      }
    /* Find dst pixels for each color channel. */
        (*mappix) (lcip, &lcip->green, srcx, srcy, &x, &y);
    if (x < minx) minx = x;
    else if (x > maxx) maxx = x;
    if (y < miny) miny = y;
    else if (y > maxy) maxy = y;
        (*mappix) (lcip, &lcip->red, srcx, srcy, &x, &y);
    if (x < minx) minx = x;
    else if (x > maxx) maxx = x;
    if (y < miny) miny = y;
    else if (y > maxy) maxy = y;
        (*mappix) (lcip, &lcip->blue, srcx, srcy, &x, &y);
    if (x < minx) minx = x;
    else if (x > maxx) maxx = x;
    if (y < miny) miny = y;
    else if (y > maxy) maxy = y;
      }
      result.x = minx;
      result.y = miny;
      result.width = maxx - minx + 1;
      result.height = maxy - miny + 1;
    }
  #ifdef TRACE
    g_warning ("< map_region result=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
  #endif
  return result;
}

/* We expect src_extent to include all pixels required to form the
 * pixels of dst_extent.
 */
#define ROW src_extent->width
#define COL 1
static void
copy_through_lens (LensCorrectionModel *oip,
                   GeglBuffer *src,
                   GeglBuffer *dst)
{
  const GeglRectangle *src_extent;
  const GeglRectangle *dst_extent;
  gfloat *src_buf;
  gfloat *dst_buf;
  gint x,y;             /* Coordinate of current dst pixel. */
  gint rgb;             /* Color channel of dst pixel being processed. */
  gint doffset;             /* Buffer offset of current dst pixel. */
  gint tmpx, tmpy, toff;
  ChannelCorrectionModel *ccm[3];   /* Allow access to red,green,blue models in loop. */

  src_extent = gegl_buffer_get_extent (src);
  #ifdef TRACE
    g_warning ("> copy_through_lens src_extent = %dx%d+%d+%d",
                 src_extent->width, src_extent->height, src_extent->x,src_extent->y);
  #endif
  if (dst == NULL)
    {
      #ifdef TRACE
        g_warning ("  dst is NULL");
        g_warning ("< copy_through_lens");
      #endif
      return;
    }
  dst_extent = gegl_buffer_get_extent (dst);
  if (dst_extent == NULL) {
      #ifdef TRACE
        g_warning ("  dst_extent is NULL");
        g_warning ("< copy_through_lens");
      #endif
      return;
  }

  /* Get src pixels. */
  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 3);
  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGB float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  /* Get buffer in which to place dst pixels. */
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 3);

  /* Compute each dst pixel in turn and store into dst buffer. */
  ccm[0] = &oip->red;
  ccm[1] = &oip->green;
  ccm[2] = &oip->blue;
  doffset = 0;
  for (y=dst_extent->y; y<dst_extent->height + dst_extent->y; y++)
    {
      for (x=dst_extent->x; x<dst_extent->width + dst_extent->x; x++)
    {
      for (rgb = 0; rgb < 3; rgb++)
        {
          gfloat gx, gy;
          gfloat val = 0.0;
          gint xx, yy;
          gfloat wx[2], wy[2], wt = 0.0;

          find_src_pixel (oip, ccm[rgb], (gfloat)x, (gfloat)y, &gx, &gy);
          tmpx = gx;
          tmpy = gy;
          wx[1] = gx - tmpx;
          wx[0] = 1.0 - wx[1];
          wy[1] = gy - tmpy;
          wy[0] = 1.0 - wy[1];
          tmpx -= src_extent->x;
          tmpy -= src_extent->y;
          toff = (tmpy * ROW + tmpx) * 3;
          for (xx = 0; xx < 2; xx++)
            {
          for (yy = 0; yy < 2; yy++)
            {
              if (tmpx+xx >= 0 && tmpx+xx < src_extent->width &&
                  tmpy+yy >= 0 && tmpy+yy < src_extent->height)
                {
              val += src_buf[toff+(yy*ROW+xx)*3+rgb] * wx[xx] * wy[yy];
              wt += wx[xx] * wy[yy];
            }
            }
        }
          if (wt <= 0)
        {
          g_warning ("gegl_lens_correct: mapped pixel %g,%g not in %dx%d+%d+%d", gx, gy,
                 src_extent->width, src_extent->height, src_extent->x, src_extent->y);
          g_warning ("                   dst = %dx%d+%d+%d", dst_extent->width, dst_extent->height, dst_extent->x,dst_extent->y);
          dst_buf [doffset+rgb] = 0.0;
        }
          else
        {
          dst_buf [doffset+rgb] = val / wt;
        }
        }
      doffset+=3;
    }
    }

  /* Store dst pixels. */
  gegl_buffer_set (dst, NULL, babl_format ("RGB float"), dst_buf, GEGL_AUTO_ROWSTRIDE);

  /* Free acquired storage. */
  g_free (src_buf);
  g_free (dst_buf);

  #ifdef TRACE
    g_warning ("< copy_through_lens");
  #endif
}

/*****************************************************************************/

/* Compute the region for which this operation is defined.
 */
static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation, "input");
  GeglChantO    *area = GEGL_CHANT_PROPERTIES (operation);

  #ifdef TRACE
    g_warning ("> get_bounding_box pointer == 0x%x in_rect == 0x%x", area->lens_info_pointer, in_rect);
  #endif
  if (in_rect && area->lens_info_pointer)
    result = map_region (in_rect, area->lens_info_pointer, find_dst_pixel);

  #ifdef TRACE
    g_warning ("< get_bounding_box result = %dx%d+%d+%d", result.width, result.height, result.x, result.y);
  #endif
  return result;
}

/* Compute the input rectangle required to compute the specified region of interest (roi).
 */
static GeglRectangle
get_required_for_output (GeglOperation        *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  GeglChantO   *area = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation, "input");
  #ifdef TRACE
    g_warning ("> get_required_for_output src=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
    if (roi)
      g_warning ("  ROI == %dx%d+%d+%d", roi->width, roi->height, roi->x, roi->y);
  #endif
  if (roi && area->lens_info_pointer) {
    result = map_region (roi, area->lens_info_pointer, find_src_pixel);
    result.width++;
    result.height++;
  }
  #ifdef TRACE
    g_warning ("< get_required_for_output res=%dx%d+%d+%d", result.width, result.height, result.x, result.y);
  #endif
  return result;
}

/* Specify the input and output buffer formats.
 */
static void
prepare (GeglOperation *operation)
{
  #ifdef TRACE
    g_warning ("> prepare");
  #endif
  gegl_operation_set_format (operation, "input", babl_format ("RGB float"));
  gegl_operation_set_format (operation, "output", babl_format ("RGB float"));
  #ifdef TRACE
    g_warning ("< prepare");
  #endif
}

/* Perform the specified operation.
 */
static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);

  copy_through_lens (o->lens_info_pointer, input, output);

  return TRUE;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  operation_class->get_required_for_output = get_required_for_output;

  operation_class->name        = "gegl:lens-correct";
  operation_class->categories  = "blur";
  operation_class->description =
        _("Copies image performing lens distortion correction.");
}

#endif
