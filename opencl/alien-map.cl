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

__kernel void cl_alien_map(__global const float4 *in,
                           __global       float4 *out,
                                          float3 freq,
                                          float3 phaseshift,
                                          int3   keep)
{
  int gid     = get_global_id(0);
  float4 in_v = in[gid];
  float3 unit = (float3) (1.0f, 1.0f, 1.0f);
  float3 tmp  = 0.5f * (unit
                        + sin((2.0f * in_v.xyz - unit) * freq.xyz + phaseshift.xyz));
  float4 out_v;

  out_v.xyz = keep.xyz ? in_v.xyz : tmp;
  out_v.w   = in_v.w;
  out[gid]  = out_v;
}
