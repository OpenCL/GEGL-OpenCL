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

gegl_chant_double (edge_preservation, "Smoothing", 0.0, 0.075, 0.005, "Amount of smoothing to do")
gegl_chant_double (diffusion,  "Diffusion",  0.0, 1.0, 0.5, "Amount of diffusion to do per iteration, 1.0 is max")
gegl_chant_int    (iterations, "Iterations", 1,   50, 10, "Number of iterations")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "noise-reduction.c"

#include "gegl-chant.h"
#include <math.h>

static void
noise_reduction (GeglBuffer          *src,
                 const GeglRectangle *src_rect,
                 GeglBuffer          *dst,
                 const GeglRectangle *dst_rect,
                 gdouble              edge_preservation,
                 gdouble              diffusion);

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
      noise_reduction (source, &source_rect, target, &target_rect, o->edge_preservation, o->diffusion);
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
                 const GeglRectangle *dst_rect,
                 gdouble              edge_preservation,
                 gdouble              diffusion)
{
  int c;
  int x,y;
  int offset;
  float *src_buf;
  float *dst_buf;

  int src_width = src_rect->width;
#define DIRECTIONS 8
  int   offsets2[DIRECTIONS][2] = {{ -1, -1}, {0, -1},{1, -1},
                                   { -1,  0},         {1,  0},
                                   { -1,  1}, {0, 1}, {1,  1}};
  int   offsets[DIRECTIONS]; /* sizeof(float) offsets for neighbours */

  src_buf = g_new0 (float, src_rect->width * src_rect->height * 4);
  dst_buf = g_new0 (float, dst_rect->width * dst_rect->height * 4);

  gegl_buffer_get (src, 1.0, src_rect, babl_format ("R'G'B'A float"), src_buf,
                   GEGL_AUTO_ROWSTRIDE);

  for (c = 0; c < DIRECTIONS; c++)
    offsets[c] = ((offsets2[c][0])+((offsets2[c][1]) * src_width)) * 4;

  offset = 0;
  for (y=0; y<dst_rect->height; y++)
    {
      float *center_pix = src_buf + (1+((y+1) * src_width)) * 4;
      for (x=0; x<dst_rect->width; x++)
        {
          for (c=0; c<3; c++)
            {
              float  result_sum = 0.0;
              float  original_gradient[DIRECTIONS];
              int    dir;
              float  max = -100.0;
              float  min = 100.0;
              float  lambda;

              for (dir = 0; dir < DIRECTIONS; dir++)
                {  /* initialize original gradients */
                  float *neighbour_pix = center_pix + offsets[dir];
                  original_gradient[dir] = center_pix[c] - neighbour_pix[c];
                  original_gradient[dir] *= original_gradient[dir];

                  if (neighbour_pix[c] > max)
                    max = neighbour_pix[c];
                  if (neighbour_pix[c] < min)
                    min = neighbour_pix[c];
                }

              lambda = edge_preservation * (max-min); /* scale lambda based on
                                                         neighbourhood range */

              for (dir = 0; dir < DIRECTIONS; dir++)
                {
                  float *neighbour_pix = center_pix + offsets[dir];
                  float  result        = (center_pix[c] * (1.0-diffusion) + neighbour_pix[c] * diffusion);
                  int    comparing_dir;
                  for (comparing_dir = 0; comparing_dir < DIRECTIONS; comparing_dir++)
                    if (G_LIKELY (comparing_dir != dir))
                      {
                        float *pix2 = center_pix + offsets[comparing_dir];
                        float  new_gradient = (result - pix2[c]);
                        new_gradient *= new_gradient;
                        if (G_UNLIKELY (new_gradient > original_gradient[comparing_dir] + lambda))
                          {
                            /* if influencing the center pixel by a smooth
                               in this direction, skip it in the summed average
                               of smoothing directions */
                            result = center_pix[c];
                            break;
                          }
                      }
                   result_sum += result;
                }
              dst_buf[offset*4+c] = result_sum / DIRECTIONS;
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
