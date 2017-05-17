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

__kernel void cl_contrast_curve(__global const float2 *in,
                                __global       float2 *out,
                                __global       float  *curve,
                                               int     num_sampling_points)
{
  int gid     = get_global_id(0);
  float2 in_v = in[gid];

  int idx = (int) fmin(num_sampling_points - 1.0f,
                       fmax(0.0f,
                            in_v.x * num_sampling_points));

  out[gid] = (float2) (curve[idx], in_v.y);
}

