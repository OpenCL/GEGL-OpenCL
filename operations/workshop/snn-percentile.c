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
 * Copyright 2005 Øyvind Kolås <pippin@gimp.org>,
 *           2007 Øyvind Kolås <oeyvindk@hig.no>
 */


#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_double (radius, _("Radius"), 0.0, 70.0, 8.0,
  _("Radius of square pixel region (width and height will be radius*2+1)"))
gegl_chant_int (pairs, _("Pairs"), 1, 2, 2,
  _("Number of pairs, higher number preserves more acute features"))
gegl_chant_double (percentile, _("Percentile"), 0.0, 100.0, 50.0,
  _("The percentile to return, the default value 50 is equal to the median."))

#else

#define MAX_SAMPLES 20000 /* adapted to percentile level of radius */

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "snn-percentile.c"

#include "gegl-chant.h"
#include <math.h>

#define RGB_LUMINANCE_RED    (0.212671)
#define RGB_LUMINANCE_GREEN  (0.715160)
#define RGB_LUMINANCE_BLUE   (0.072169)

static inline gfloat rgb2luminance (gfloat *pix)
{
  return pix[0] * RGB_LUMINANCE_RED +
         pix[1] * RGB_LUMINANCE_GREEN +
         pix[2] * RGB_LUMINANCE_BLUE;
}

#define POW2(a)((a)*(a))

static inline gfloat colordiff (gfloat *pixA,
                                gfloat *pixB)
{
  return POW2(pixA[0]-pixB[0])+
         POW2(pixA[1]-pixB[1])+
         POW2(pixA[2]-pixB[2]);
}

typedef struct
{
  int       head;
  int       next[MAX_SAMPLES];
  float     luma[MAX_SAMPLES];
  float    *pixel[MAX_SAMPLES];
  int       items;
} RankList;

static void
list_clear (RankList * p)
{
  p->items = 0;
  p->next[0] = -1;
}

static inline void
list_add (RankList *p,
          gfloat    lumniosity,
          gfloat   *pixel)
{
  gint location;

  location = p->items;

  p->items++;
  p->luma[location] = lumniosity;
  p->pixel[location] = pixel;
  p->next[location] = -1;

  if (p->items == 1)
    {
      p->head = location;
      return;
    }
  if (lumniosity <= p->luma[p->head])
    {
      p->next[location] = p->head;
      p->head = location;
    }
  else
    {
      gint   prev, i;
      prev = p->head;
      i = prev;
      while (i >= 0 && p->luma[i] < lumniosity)
        {
          prev = i;
          i = p->next[i];
        }
      p->next[location] = p->next[prev];
      p->next[prev] = location;
    }
}

static inline gfloat *
list_percentile (RankList *p,
                 gdouble   percentile)
{
  gint       i = p->head;
  gint       pos = 0;
  if (!p->items)
    return NULL;
  if (percentile >= 1.0)
    percentile = 1.0;
  while (pos < p->items * percentile &&
         p->pixel[p->next[i]])
    {
      i = p->next[i];
      pos++;
    }
  return p->pixel[i];
}

static void
snn_percentile (GeglBuffer *src,
                GeglBuffer *dst,
                gdouble     radius,
                gdouble     percentile,
                gint        pairs)
{
  gint    x, y;
  gint    offset;
  gfloat *src_buf;
  gfloat *dst_buf;
  RankList list = {0};

  src_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (src) * 4);
  dst_buf = g_new0 (gfloat, gegl_buffer_get_pixel_count (dst) * 4);

  gegl_buffer_get (src, 1.0, NULL, babl_format ("RGBA float"), src_buf, GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  percentile/= 100.0;

  for (y=0; y<gegl_buffer_get_height (dst); y++)
    for (x=0; x<gegl_buffer_get_width (dst); x++)
      {
        gint u,v;
        gfloat *center_pix = src_buf + offset * 4;

        list_clear (&list);

        /* iterate through the upper left quater of pixels */
        for (v=-radius;v<=0;v++)
          for (u=-radius;u<= (pairs==1?radius:0);u++)
            {
              gfloat *selected_pix = center_pix;
              gfloat  best_diff = 1000.0;
              gint    i;

              /* skip computations for the center pixel */
              if (u != 0 &&
                  v != 0)
                {
                  /* compute the coordinates of the symmetric pairs for
                   * this locaiton in the quadrant
                   */
                  gint xs[4] = {x+u, x-u, x-u, x+u};
                  gint ys[4] = {y+v, y-v, y+v, y-v};

                  /* check which member of the symmetric quadruple to use */
                  for (i=0;i<pairs*2;i++)
                    {
                      if (xs[i] >= 0 && xs[i] < gegl_buffer_get_width (src) &&
                          ys[i] >= 0 && ys[i] < gegl_buffer_get_height (src))
                        {
                          gfloat *tpix = src_buf + (xs[i]+ys[i]*gegl_buffer_get_width (src))*4;
                          gfloat diff = colordiff (tpix, center_pix);
                          if (diff < best_diff)
                            {
                              best_diff = diff;
                              selected_pix = tpix;
                            }
                        }
                    }
                }

              list_add (&list, rgb2luminance(selected_pix), selected_pix);

              if (u==0 && v==0)
                break; /* to avoid doubly processing when using only 1 pair */
            }
        {
          gfloat *result = list_percentile (&list, percentile);
          for (u=0; u<4; u++)
            dst_buf[offset*4+u] = result[u];
        }
        offset++;
      }
  gegl_buffer_set (dst, NULL, babl_format ("RGBA float"), dst_buf, GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  area->left = area->right = area->top = area->bottom =
      GEGL_CHANT_PROPERTIES (operation)->radius;
  gegl_operation_set_format (operation, "output", babl_format ("RGBA float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglBuffer   *temp_in;
  GeglRectangle compute  = gegl_operation_get_required_for_output (operation, "input", result);

  if (result->width == 0 ||
      result->height== 0 ||
      o->radius < 1.0)
    {
      output = g_object_ref (input);
    }
  else
    {
      temp_in = gegl_buffer_create_sub_buffer (input, &compute);
      snn_percentile (temp_in, output, o->radius, o->percentile, o->pairs);
      g_object_unref (temp_in);
    }

  return  TRUE;
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

  operation_class->name        = "gegl:snn-percentile";
  operation_class->categories  = "misc";
  operation_class->description =
        _("Noise reducing edge enhancing percentile filter based on Symmetric Nearest Neighbours");
}

#endif
