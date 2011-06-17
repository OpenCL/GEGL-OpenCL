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
 * Ali Alsam, Hans Jakob Rivertz, Øyvind Kolås (c) 2011
 */

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (iterations, "Iterations", 1, 200, 6, "Number of iterations, more iterations takes longer time but might also make the image less noise, the number of iterations is the longest distance pixel content from one pixel can be diffusd out in the image.")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "noise-reduction.c"

#include "gegl-chant.h"
#include <math.h>

#define POW2(a) ((a)*(a))

/* core code/formulas to be tweaked for the tuning the implementation */
#define GEN_METRIC(before, center, after) \
                   POW2((center - before) + (center - after))
                   //POW2((center) * 2 - (before) - (after))

/* Condition used to bail diffusion from a direction */
#define BAIL_CONDITION(new,original) ((new) > (original))


static void
noise_reduction (GeglBuffer          *src,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *dst,
                 const GeglRectangle *dst_rect);

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO            *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = o->iterations;
  gegl_operation_set_format (operation, "input",  babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  GeglRectangle compute;
  GeglBuffer   *temp[2] = {NULL, NULL};
  int iteration;

  compute = gegl_operation_get_required_for_output (operation, "input",result);

  if (o->iterations > 1)
    {
      temp[0] = gegl_buffer_new (&compute, babl_format ("R'G'B'A float"));
      temp[1] = gegl_buffer_new (&compute, babl_format ("R'G'B'A float"));
    }

  for (iteration = 0; iteration < o->iterations; iteration++)
    {
      GeglBuffer *source, *target;
      GeglRectangle source_rect;
      GeglRectangle target_rect;

      /* compute extent of rectangles needed for this iteration,
       * minimizing the work neccesary to do
       */
      target_rect = *result;
      target_rect.x      -= (o->iterations-iteration-1);
      target_rect.y      -= (o->iterations-iteration-1);
      target_rect.width  += (o->iterations-iteration-1)*2;
      target_rect.height += (o->iterations-iteration-1)*2;

      source_rect = target_rect;
      source_rect.x -= 1;
      source_rect.y -= 1;
      source_rect.width += 2;
      source_rect.height += 2;

      source = temp[iteration%2];
      target = temp[(iteration+1)%2];
      if (iteration == 0)
        source = input;
      if (iteration == o->iterations-1)
        target = output;

      noise_reduction (source, &source_rect, target, &target_rect);
    }

  if (temp[0])
    g_object_unref (temp[0]);
  if (temp[1])
    g_object_unref (temp[1]);

  return  TRUE;
}

static void
noise_reduction (GeglBuffer          *src,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *dst,
                 const GeglRectangle *dst_rect)
{
  int c;
  int x,y;
  int offset;
  float *src_buf;
  float *dst_buf;

  int src_width = src_rect->width;
#define OFFSETS 8

/* fetch symmetric entry */
#define SYMMETRY(a)  (a+(OFFSETS/2))

  int   rel_offsets[OFFSETS][2] = {
                                   { -1, -1}, {0, -1},{1, -1},
                                   { -1,  0},         {1,  0},
                                   { -1,  1}, {0, 1}, {1,  1}
                                  };
  int   offsets[OFFSETS]; /* sizeof(float) offsets for neighbours */

  /* initialize offsets, dependent on source buffer width */
  for (c = 0; c < OFFSETS; c++)
    offsets[c] = ((rel_offsets[c][0])+((rel_offsets[c][1]) * src_width)) * 4;

  src_buf = g_new0 (float, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (float, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("R'G'B'A float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE);

  offset = 0;
  for (y=0; y<dst_rect->height; y++)
    {
      float *center_pix = src_buf + (1+((y+1) * src_width)) * 4;
      for (x=0; x<dst_rect->width; x++)
        {
          for (c=0; c<3; c++)
            {
              float  original_metric[OFFSETS];
              int    dir;
              float  sum;
              int    count;

              for (dir = 0; dir < OFFSETS/2; dir++)
                {  /* initialize original metrics for the horizontal, vertical and 2 diagonal
                    * metrics
                    */
                  float *before_pix  = center_pix + offsets[dir];
                  float *after_pix   = center_pix + offsets[SYMMETRY(dir)];

                  original_metric[dir] =
                    GEN_METRIC (before_pix[c], center_pix[c], after_pix[c]);
                }

              sum = center_pix[c];
              count = 1;

              /* try smearing in data from each of the 8 neighbours */
              for (dir = 0; dir < OFFSETS; dir++)
                {
                  float *pix   = center_pix + offsets[dir];
                  float  value = pix[c] * 0.5 + center_pix[c] * 0.5;
                  /* ... assumption for magic number 0.2, assume that each
                   * direction contributes on average 0.2
                     thus that seems like a reasonable value to check with.. */
                  int    comparison_dir;
                  gboolean valid = TRUE; /* if diffusion from this direction
                                          * should be made
                                          */
                  for (comparison_dir = 0; comparison_dir < OFFSETS/2; comparison_dir++)
                    if (G_LIKELY (comparison_dir != dir))
                      {
                        float *before_pix = center_pix + offsets[comparison_dir];
                        float *after_pix = center_pix + offsets[SYMMETRY(comparison_dir)];
                        float  new_metric =
                          GEN_METRIC (before_pix[c], value, after_pix[c]);


                        if (G_UNLIKELY (BAIL_CONDITION(new_metric,
                                                       original_metric[comparison_dir])))
                          { /* bail condition */
                            valid = FALSE;
                            break;
                          }
                      }
                  if (valid)
                    {
                      sum += value;   /* accumulate value */
                      count ++;       /* increase denominator */
                    }
                }
              dst_buf[offset*4+c] = sum / count; /* write back the average of center pixel
                                                    and all pixels that would be valid to
                                                    replace the center pixel with while
                                                    continuing to fulfil our criteria.
                                                  */
            }
          dst_buf[offset*4+3] = center_pix[3]; /* copy alpha unmodified */
          offset++;
          center_pix += 4;
        }
    }

  gegl_buffer_set (dst, dst_rect, babl_format ("R'G'B'A float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  GeglRectangle  result = {0,0,0,0};
  GeglRectangle *in_rect = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");
  if (!in_rect)
    return result;
  return *in_rect;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass       *operation_class;
  GeglOperationFilterClass *filter_class;

  operation_class  = GEGL_OPERATION_CLASS (klass);
  filter_class     = GEGL_OPERATION_FILTER_CLASS (klass);

  filter_class->process   = process;
  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;

  operation_class->name        = "gegl:noise-reduction";
  operation_class->categories  = "enhance";
  operation_class->description = "Anisotropic like smoothing operation";
}

#endif
