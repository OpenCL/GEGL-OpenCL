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
 * Copyright 2013 Carlos Zubieta <czubieta.dev@gmail.com>
 */

__kernel void cl_calc_block_colors (__global const float4 *in,
                                    __global       float4 *block_colors,
                                                   int     cx0,
                                                   int     cy0,
                                                   int     size,
                                                   float   weight,
                                                   int     roi_x,
                                                   int     roi_y,
                                                   int     line_width)
{
  int    cx   = cx0 + get_global_id(0);
  int    cy   = cy0 + get_global_id(1);
  int    px   = (cx * size) - roi_x + size;
  int    py   = (cy * size) - roi_y + size;
  float4 mean = (float4)(0.0f);
  float4 tmp;
  int    i, j;

  for( j = py; j < py+size; ++j)
    {
      for (i = px; i < px+size; ++i)
        {
          mean += in[j * line_width + i];
        }
    }
  block_colors[(cx-cx0) + get_global_size(0) * (cy-cy0)] = mean*weight;
}

__kernel void cl_dot (__global const float4 *block_colors,
                      __global       float4 *out,
                                     int     cx0,
                                     int     cy0,
                                     int     size,
                                     float   radius2,
                                     int     roi_x,
                                     int     roi_y,
                                     int     block_count_x)
{
  int    gidx  = get_global_id(0);
  int    gidy  = get_global_id(1);
  int    x     = gidx + roi_x;
  int    y     = gidy + roi_y;
  int    cy    = y/size;
  int    cx    = x/size;
  float  cellx = convert_float(x - cx * size) - convert_float(size) / 2.0;
  float  celly = convert_float(y - cy * size) - convert_float(size) / 2.0;
  float4 tmp  = (float4)(0.0);

  if((cellx * cellx + celly * celly) <= radius2)
    tmp = block_colors[(cx-cx0) + block_count_x * (cy-cy0)];

  out[gidx + get_global_size(0) * gidy] = tmp;
}

