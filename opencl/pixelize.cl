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
 * Copyright 2013 Victor Oliveira <victormatheus@gmail.com>
 * Copyright 2013 Carlos Zubieta  <czubieta.dev@gmail.com>
 */

__kernel void calc_block_color(__global float4 *in,
                               __global float4 *out,
                                        int     xsize,
                                        int     ysize,
                                        int     roi_x,
                                        int     roi_y,
                                        int     total_width_x,
                                        int     total_width_y,
                                        int     line_width,
                                        int     block_count_x )
{
  int gidx = get_global_id(0);
  int gidy = get_global_id(1);

  int cx = roi_x / xsize + gidx;
  int cy = roi_y / ysize + gidy;

  int px = cx * xsize - roi_x;
  int py = cy * ysize - roi_y;

  int i, j;

  float4 col = (float4)(0.0f, 0.0f, 0.0f, 0.0f);

  int real_xsize = min (total_width_x - px - roi_x, xsize);
  int real_ysize = min (total_width_y - py - roi_y, ysize);

  float weight = 1.0f / (real_xsize * real_ysize);

  for (j = py; j < py + real_ysize; ++j)
    {
      for (i = px; i < px + real_xsize; ++i)
        {
          col += in[(j + ysize) * line_width + i + xsize];
        }
    }
  out[gidy * block_count_x + gidx] = col * weight;
}


#define NORM_MANHATTAN 0
#define NORM_EUCLIDEAN 1
#define NORM_INFINITY  2
#define SQR(x)         ((x)*(x))

__kernel void kernel_pixelize(__global float4 *in,
                              __global float4 *out,
                                       int     xsize,
                                       int     ysize,
                                       float   xratio,
                                       float   yratio,
                                       int     roi_x,
                                       int     roi_y,
                                       float4  bg_color,
                                       int     norm,
                                       int     block_count_x)
{
  int gidx = get_global_id(0);
  int gidy = get_global_id(1);

  int src_width  = get_global_size(0);
  int cx = (gidx + roi_x) / xsize - roi_x / xsize;
  int cy = (gidy + roi_y) / ysize - roi_y / ysize;

  float4 grid_color = in[cx + cy * block_count_x];
  float4 out_color = bg_color;

  int x_pos = gidx + roi_x;
  int y_pos = gidy + roi_y;

  int rect_shape_width  = ceil (xsize * xratio);
  int rect_shape_height = ceil (ysize * yratio);

  int off_shape_x = floor ((xsize - xratio * xsize) / 2.0f);
  int off_shape_y = floor ((ysize - yratio * ysize) / 2.0f);

  int start_x = (x_pos / xsize) * xsize - roi_x;
  int start_y = (y_pos / ysize) * ysize - roi_y;

  float shape_area = rect_shape_width * rect_shape_height;

  float center_x = start_x + off_shape_x + (float)(rect_shape_width)  / 2.0f;
  float center_y = start_y + off_shape_y + (float)(rect_shape_height) / 2.0f;

  if (norm == NORM_MANHATTAN &&
      (fabs (gidx - center_x) * rect_shape_height +
       fabs (gidy - center_y) * rect_shape_width
       < shape_area))
    out_color = grid_color;

  if (norm == NORM_EUCLIDEAN &&
      SQR ((gidx - center_x) / (float)rect_shape_width) +
      SQR ((gidy - center_y) / (float)rect_shape_height) <= 1.0f)
    out_color = grid_color;

  if (norm == NORM_INFINITY &&
      (gidx >= start_x + off_shape_x &&
       gidy >= start_y + off_shape_y &&
       gidx < start_x + off_shape_x + rect_shape_width &&
       gidy < start_y + off_shape_y + rect_shape_height))
      out_color = grid_color;

  out[gidx + gidy * src_width] = out_color;
}
