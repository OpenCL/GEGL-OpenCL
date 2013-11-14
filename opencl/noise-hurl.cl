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

__kernel void cl_noise_hurl(__global       float4    *src,
                            __global const int       *random_data,
                                           int        x_offset,
                                           int        y_offset,
                                           int        roi_width,
                                           int        whole_region_width,
                                           GeglRandom rand,
                                           float      pct_random,
                                           int        offset)
{
  int gid  = get_global_id(0);
  int gidy = gid / roi_width;
  int gidx = gid - gidy*roi_width;

  int x = gidx + x_offset;
  int y = gidy + y_offset;
  int n = 4 * (x + whole_region_width * y + offset);

  float4 src_v = src[gid];

  float pc          = gegl_cl_random_float_range (random_data,
                                                  rand, x, y, 0, n, 0, 100);
  float red_noise   = gegl_cl_random_float (random_data,
                                            rand, x, y, 0, n+1);
  float green_noise = gegl_cl_random_float (random_data,
                                            rand, x, y, 0, n+2);
  float blue_noise  = gegl_cl_random_float (random_data,
                                            rand, x, y, 0, n+3);

  if(pc <= pct_random)
    {
      src_v.x = red_noise;
      src_v.y = green_noise;
      src_v.z = blue_noise;
    }
  src[gid] = src_v;
}
