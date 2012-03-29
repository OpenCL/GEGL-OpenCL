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

gegl_chant_int (iterations, "Strength", 1, 32, 4, "How many iteratarion to run the algorithm.")

#else

#define GEGL_CHANT_TYPE_AREA_FILTER
#define GEGL_CHANT_C_FILE       "noise-reduction.c"

#include "gegl-chant.h"
#include <math.h>

/* The core noise_reduction function, which is implemented as
 * portable C - this is the function where most cpu time goes
 */
static void
noise_reduction (float *src_buf,     /* source buffer, one pixel to the left
                                        and up from the starting pixel */
                 int    src_stride,  /* stridewidth of buffer in pixels */
                 float *dst_buf,     /* destination buffer */
                 int    dst_width,   /* width to render */
                 int    dst_height,  /* height to render */
                 int    dst_stride)  /* stride of target buffer */
{
  int c;
  int x,y;
  int dst_offset;

#define NEIGHBOURS 8
#define AXES       (NEIGHBOURS/2)

#define POW2(a) ((a)*(a))
/* core code/formulas to be tweaked for the tuning the implementation */
#define GEN_METRIC(before, center, after) \
                   POW2((center) * 2 - (before) - (after))

/* Condition used to bail diffusion from a direction */
#define BAIL_CONDITION(new,original) ((new) > (original))

#define SYMMETRY(a)  (NEIGHBOURS - (a) - 1) /* point-symmetric neighbour pixel */

#define O(u,v) (((u)+((v) * src_stride)) * 4)
  int   offsets[NEIGHBOURS] = {  /* array of the relative distance i float
                                  * pointers to each of neighbours
                                  * in source buffer, allows quick referencing.
                                  */
              O( -1, -1), O(0, -1), O(1, -1),
              O( -1,  0),           O(1,  0),
              O( -1,  1), O(0, 1),  O(1,  1)};
#undef O

  dst_offset = 0;
  for (y=0; y<dst_height; y++)
    {
      float *center_pix = src_buf + ((y+1) * src_stride + 1) * 4;
      dst_offset = dst_stride * y;
      for (x=0; x<dst_width; x++)
        {
          for (c=0; c<3; c++) /* do each color component individually */
            {
              float  metric_reference[AXES];
              int    axis;
              int    direction;
              float  sum;
              int    count;

              for (axis = 0; axis < AXES; axis++)
                { /* initialize original metrics for the horizontal, vertical
                     and 2 diagonal metrics */
                  float *before_pix  = center_pix + offsets[axis];
                  float *after_pix   = center_pix + offsets[SYMMETRY(axis)];

                  metric_reference[axis] =
                    GEN_METRIC (before_pix[c], center_pix[c], after_pix[c]);
                }

              sum   = center_pix[c];
              count = 1;

              /* try smearing in data from all neighbours */
              for (direction = 0; direction < NEIGHBOURS; direction++)
                {
                  float *pix   = center_pix + offsets[direction];
                  float  value = pix[c] * 0.5 + center_pix[c] * 0.5;
                  int    axis;
                  int    valid;

                  /* check if the non-smoothing operating check is true if
                   * smearing from this direction for any of the axes */
                  valid = 1; /* assume it will be valid */
                  for (axis = 0; axis < AXES; axis++)
                    {
                      float *before_pix = center_pix + offsets[axis];
                      float *after_pix  = center_pix + offsets[SYMMETRY(axis)];
                      float  metric_new =
                             GEN_METRIC (before_pix[c], value, after_pix[c]);

                      if (BAIL_CONDITION(metric_new, metric_reference[axis]))
                        {
                          valid = 0; /* mark as not a valid smoothing, and .. */
                          break;     /* .. break out of loop */
                        }
                    }
                  if (valid) /* we were still smooth in all axes */
                    {        /* add up contribution to final result  */
                      sum += value;
                      count ++;
                    }
                }
              dst_buf[dst_offset*4+c] = sum / count;
            }
          dst_buf[dst_offset*4+3] = center_pix[3]; /* copy alpha unmodified */
          dst_offset++;
          center_pix += 4;
        }
    }
}

static void prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglChantO              *o = GEGL_CHANT_PROPERTIES (operation);

  area->left = area->right = area->top = area->bottom = o->iterations;
  gegl_operation_set_format (operation, "input",  babl_format ("R'G'B'A float"));
  gegl_operation_set_format (operation, "output", babl_format ("R'G'B'A float"));
}

#define INPLACE 1

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglChantO   *o = GEGL_CHANT_PROPERTIES (operation);
  int iteration;
  int stride;
  float *src_buf;
#ifndef INPLACE
  float *dst_buf;
#endif
  GeglRectangle rect;
  rect = *result;

  stride = result->width + o->iterations * 2;

  src_buf = g_new0 (float,
         (stride) * (result->height + o->iterations * 2) * 4);
#ifndef INPLACE
  dst_buf = g_new0 (float,
         (stride) * (result->height + o->iterations * 2) * 4);
#endif

  {
    rect.x      -= o->iterations;
    rect.y      -= o->iterations;
    rect.width  += o->iterations*2;
    rect.height += o->iterations*2;
    gegl_buffer_get (input, &rect, 1.0, babl_format ("R'G'B'A float"),
                     src_buf, stride * 4 * 4, GEGL_ABYSS_NONE);
  }

  for (iteration = 0; iteration < o->iterations; iteration++)
    {
      noise_reduction (src_buf, stride,
#ifdef INPLACE
                       src_buf + (stride + 1) * 4,
#else
                       dst_buf,
#endif
                       result->width  + (o->iterations - 1 - iteration) * 2,
                       result->height + (o->iterations - 1 - iteration) * 2,
                       stride);
#ifndef INPLACE
      { /* swap buffers */
        float *tmp = src_buf;
        src_buf = dst_buf;
        dst_buf = tmp;
      }
#endif
    }

  gegl_buffer_set (output, result, 0, babl_format ("R'G'B'A float"),
#ifndef INPLACE
                   src_buf,
#else
                   src_buf + ((stride +1) * 4) * o->iterations,
#endif
                   stride * 4 * 4);

  g_free (src_buf);
#ifndef INPLACE
  g_free (dst_buf);
#endif

  return  TRUE;
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

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:noise-reduction",
    "categories" , "enhance",
    "description", "Anisotropic like smoothing operation",
    NULL);
}

#endif
