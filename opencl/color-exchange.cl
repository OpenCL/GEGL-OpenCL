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
 * Copyright 2015 Thomas Manni <thomas.manni@free.fr>
 */

__kernel void cl_color_exchange(__global const float4 *in,
                                __global       float4 *out,
                                               float3 color_diff,
                                               float3 min,
                                               float3 max)
{
  int gid     = get_global_id(0);
  float4 in_v = in[gid];
  float4 out_v;

  if(in_v.x > min.x && in_v.x < max.x &&
     in_v.y > min.y && in_v.y < max.y &&
     in_v.z > min.z && in_v.z < max.z)
    {
      out_v.x = clamp(in_v.x + color_diff.x, 0.0f, 1.0f);
      out_v.y = clamp(in_v.y + color_diff.y, 0.0f, 1.0f);
      out_v.z = clamp(in_v.z + color_diff.z, 0.0f, 1.0f);
    }
  else
    {
      out_v.xyz = in_v.xyz;
    }

  out_v.w  = in_v.w;
  out[gid] = out_v;
}
