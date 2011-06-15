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

gegl_chant_int (iterations, "Iterations", 1, 50, 5, "Number of iterations, more iterations takes longer time but might also make the image less noisy.")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "noise-reduction.c"

#include "gegl-chant.h"
#include <math.h>

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
  int   rel_offsets[8][2] = {{ -1, -1}, {0, -1},{1, -1},
                             { -1,  0},         {1,  0},
                             { -1,  1}, {0, 1}, {1,  1}};
  int   offsets[8]; /* sizeof(float) offsets for neighbours */

  /* initialize offsets, dependent on source buffer width */
  for (c = 0; c < 8; c++)
    offsets[c] = ((rel_offsets[c][0])+((rel_offsets[c][1]) * src_width)) * 4;

  src_buf = g_new0 (float, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (float, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("R'G'B'A float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE);

#define POW2(a) ((a)*(a))
#define SYMMETRY_PIXEL(a)  ((a+4)%8)

  offset = 0;
  for (y=0; y<dst_rect->height; y++)
    {
      float *center_pix = src_buf + (1+((y+1) * src_width)) * 4;
      for (x=0; x<dst_rect->width; x++)
        {
          for (c=0; c<3; c++)
            {
              float  result_sum = center_pix[c];
              int    count = 1;
              float  original_gradient[4];
              int    dir;

              for (dir = 0; dir < 4; dir++)
                {  /* initialize original gradients */
                  float *cpix  = center_pix + offsets[dir];
                  float *cpixb = center_pix + offsets[SYMMETRY_PIXEL(dir)];
                  original_gradient[dir] = POW2((center_pix[c] - cpix[c]) + (center_pix[c] - cpixb[c]));
                }

              /* try smearing in data from each of the 8 neighbours */
              for (dir = 0; dir < 8; dir++)
                {
                  float *pix    = center_pix + offsets[dir];
                  float  result = (pix[c] + center_pix[c]) * 0.5;
                  int    comparison_dir;
                  gboolean valid = TRUE;

                  for (comparison_dir = 0; comparison_dir < 4; comparison_dir++)
                    if (G_LIKELY (comparison_dir != dir))
                      {
                        float *cpix  = center_pix + offsets[comparison_dir];
                        float *cpixb = center_pix + offsets[SYMMETRY_PIXEL(comparison_dir)];
                        float  new_gradient = POW2((result - cpix[c]) + (result - cpixb[c]));

                        if (G_UNLIKELY (new_gradient > original_gradient[comparison_dir]))
                          { /* The 2nd order derivative increased, use original value instead */
                            valid = FALSE;
                            break;
                          }
                      }
                  if (valid)
                    {
                      count ++;
                      result_sum += result;
                    }
                }

              /* Do a run with smearing across the pixel, ignoring center pixel,
                 XXX: mostly repeating the code block above, apart from the criterium
                 for how to blur
               */
              for (dir = 0; dir < 4; dir++)
                {
                  float *pix     = center_pix + offsets[dir];
                  float *pixb    = center_pix + offsets[SYMMETRY_PIXEL(dir)];
                  float  result = (pix[c] + pixb[c]) /2;
                  int    comparison_dir;
                  gboolean valid = TRUE;

                  for (comparison_dir = 0; comparison_dir < 4; comparison_dir++)
                    if (G_LIKELY (comparison_dir != dir))
                      {
                        float *cpix  = center_pix + offsets[comparison_dir];
                        float *cpixb = center_pix + offsets[SYMMETRY_PIXEL(comparison_dir)];
                        float  new_gradient = POW2((result - cpix[c]) + (result - cpixb[c]));

                        if (G_UNLIKELY (new_gradient > original_gradient[comparison_dir]))
                          { /* The 2nd order derivative increased, use original value instead */
                            valid = FALSE;
                            break;
                          }
                      }
                  if (valid)
                    {
                      count += 2;  /* give such an average a high weight, since it seemingly
                                      would be a smoothing that cancels out an original center pixel
                                    */
                      result_sum += result * 2;
                    }
                }

              dst_buf[offset*4+c] = result_sum / count;
            }
          dst_buf[offset*4+3] = center_pix[3]; /* copy alpha */
          offset++;
          center_pix += 4;
        }
    }

  gegl_buffer_set (dst, dst_rect, babl_format ("R'G'B'A float"), dst_buf,
                   GEGL_AUTO_ROWSTRIDE);
  g_free (src_buf);
  g_free (dst_buf);
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

  operation_class->name        = "gegl:noise-reduction";
  operation_class->categories  = "enhance";
  operation_class->description = "Anisotropic like smoothing operation";
}

#endif
