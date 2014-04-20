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

#define RED_FACTOR    0.5133333f
#define GREEN_FACTOR  1
#define BLUE_FACTOR   0.1933333f

__kernel void cl_red_eye_removal(__global const float4 *in,
                                 __global       float4 *out,
                                                float threshold)
{
  int gid     = get_global_id(0);
  float4 in_v = in[gid];
  float adjusted_red       = in_v.x * RED_FACTOR;
  float adjusted_green     = in_v.y * GREEN_FACTOR;
  float adjusted_blue      = in_v.z * BLUE_FACTOR;
  float adjusted_threshold = (threshold - 0.4f) * 2;
  float tmp;

  if (adjusted_red >= adjusted_green - adjusted_threshold &&
      adjusted_red >= adjusted_blue  - adjusted_threshold)
    {
      tmp = (adjusted_green + adjusted_blue) / (2.0f * RED_FACTOR);
      in_v.x = clamp(tmp, 0.0f, 1.0f);
    }
  out[gid]  = in_v;
}
