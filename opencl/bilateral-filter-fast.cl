/* This file is part of GEGL
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
 * Copyright 2013 Victor Oliveira (victormatheus@gmail.com)
 */

#define GRID(x,y,z) grid[x+sw*(y + z * sh)]

__kernel void bilateral_init(__global float8 *grid,
                             int sw,
                             int sh,
                             int depth)
{
  const int gid_x = get_global_id(0);
  const int gid_y = get_global_id(1);

  for (int d=0; d<depth; d++)
    {
      GRID(gid_x,gid_y,d) = (float8)(0.0f);
    }
}

__kernel void bilateral_downsample(__global const float4 *input,
                                   __global       float2 *grid,
                                   int width,
                                   int height,
                                   int sw,
                                   int sh,
                                   int   s_sigma,
                                   float r_sigma)
{
  const int gid_x = get_global_id(0);
  const int gid_y = get_global_id(1);

  for (int ry=0; ry < s_sigma; ry++)
    for (int rx=0; rx < s_sigma; rx++)
      {
        const int x = clamp(gid_x * s_sigma - s_sigma/2 + rx, 0, width -1);
        const int y = clamp(gid_y * s_sigma - s_sigma/2 + ry, 0, height-1);

        const float4 val = input[y * width + x];

        const int4 z = convert_int4(val * (1.0f/r_sigma) + 0.5f);

        grid[4*(gid_x+sw*(gid_y + z.x * sh))+0] += (float2)(val.x, 1.0f);
        grid[4*(gid_x+sw*(gid_y + z.y * sh))+1] += (float2)(val.y, 1.0f);
        grid[4*(gid_x+sw*(gid_y + z.z * sh))+2] += (float2)(val.z, 1.0f);
        grid[4*(gid_x+sw*(gid_y + z.w * sh))+3] += (float2)(val.w, 1.0f);

        barrier (CLK_GLOBAL_MEM_FENCE);
      }
}

#define LOCAL_W 16
#define LOCAL_H 16

__attribute__((reqd_work_group_size(16, 16, 1)))
__kernel void bilateral_blur(__global const float8 *grid,
                             __global       float2 *blurz_r,
                             __global       float2 *blurz_g,
                             __global       float2 *blurz_b,
                             __global       float2 *blurz_a,
                             int sw,
                             int sh,
                             int depth)
{
  __local float8 img1[LOCAL_H+2][LOCAL_W+2];
  __local float8 img2[LOCAL_H+2][LOCAL_W+2];

  const int gid_x = get_global_id(0);
  const int gid_y = get_global_id(1);

  const int lx = get_local_id(0);
  const int ly = get_local_id(1);

  float8 vpp = (float8)(0.0f);
  float8 vp  = (float8)(0.0f);
  float8 v   = (float8)(0.0f);

  float8 k;

  int x  = clamp(gid_x, 0, sw-1);
  int y  = clamp(gid_y, 0, sw-1);

  for (int d=0; d<depth; d++)
    {
      int xp = max(x-1, 0);
      int xn = min(x+1, sw-1);

      int yp = max(y-1, 0);
      int yn = min(y+1, sh-1);

      int zp = max(d-1, 0);
      int zn = min(d+1, depth-1);

      /* the corners are not going to be used, don't bother to load them */

      if (ly == 0) {
        k = GRID(x, yp, d);
        img1[0][lx+1] = k;
        img2[0][lx+1] = k;
      }

      if (ly == LOCAL_H-1) {
        k = GRID(x, yn, d);
        img1[LOCAL_H+1][lx+1] = k;
        img2[LOCAL_H+1][lx+1] = k;
      }

      if (lx == 0) {
        k = GRID(xp, y, d);
        img1[ly+1][0] = k;
        img2[ly+1][0] = k;
      }

      if (lx == LOCAL_W-1) {
        k = GRID(xn, y, d);
        img1[ly+1][LOCAL_W+1] = k;
        img2[ly+1][LOCAL_W+1] = k;
      }

      img1[ly+1][lx+1] = GRID(x, y, d);

      barrier (CLK_LOCAL_MEM_FENCE);

      /* blur x */

      img2[ly+1][lx+1] = (img1[ly+1][lx] + 2.0f * img1[ly+1][lx+1] + img1[ly+1][lx+2]) / 4.0f;

      barrier (CLK_LOCAL_MEM_FENCE);

      /* blur y */

      v = (img2[ly][lx+1] + 2.0f * img2[ly+1][lx+1] + img2[ly+2][lx+1]) / 4.0f;

      /* last three v values */

      if (d == 0)
        {
          /* this is like CLAMP */
          vpp = img1[ly+1][lx+1];
          vp  = img1[ly+1][lx+1];
        }
      else
        {
          vpp = vp;
          vp  = v;

          float8 blurred = (vpp + 2.0f * vp + v) / 4.0f;

          /* XXX: OpenCL 1.1 doesn't support writes to 3D textures */

          if (gid_x < sw && gid_y < sh)
            {
              blurz_r[x+sw*(y+sh*(d-1))] = blurred.s01;
              blurz_g[x+sw*(y+sh*(d-1))] = blurred.s23;
              blurz_b[x+sw*(y+sh*(d-1))] = blurred.s45;
              blurz_a[x+sw*(y+sh*(d-1))] = blurred.s67;
            }
        }
    }

  /* last z */

  vpp = vp;
  vp  = v;

  float8 blurred = (vpp + 2.0f * vp + v) / 4.0f;

  if (gid_x < sw && gid_y < sh)
    {
      blurz_r[x+sw*(y+sh*(depth-1))] = blurred.s01;
      blurz_g[x+sw*(y+sh*(depth-1))] = blurred.s23;
      blurz_b[x+sw*(y+sh*(depth-1))] = blurred.s45;
      blurz_a[x+sw*(y+sh*(depth-1))] = blurred.s67;
    }
}

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

__kernel void bilateral_interpolate(__global    const float4    *input,
                                    __read_only       image3d_t  blurz_r,
                                    __read_only       image3d_t  blurz_g,
                                    __read_only       image3d_t  blurz_b,
                                    __read_only       image3d_t  blurz_a,
                                    __global          float4    *smoothed,
                                    int   width,
                                    int   s_sigma,
                                    float r_sigma)
{
  const int x = get_global_id(0);
  const int y = get_global_id(1);

  const float  xf = (float)(x) / s_sigma;
  const float  yf = (float)(y) / s_sigma;
  const float4 zf = input[y * width + x] / r_sigma;

  float8 val;

  val.s04 = (read_imagef (blurz_r, sampler, (float4)(xf, yf, zf.x, 0.0f))).xy;
  val.s15 = (read_imagef (blurz_g, sampler, (float4)(xf, yf, zf.y, 0.0f))).xy;
  val.s26 = (read_imagef (blurz_b, sampler, (float4)(xf, yf, zf.z, 0.0f))).xy;
  val.s37 = (read_imagef (blurz_a, sampler, (float4)(xf, yf, zf.w, 0.0f))).xy;

  smoothed[y * width + x] = val.s0123/val.s4567;
}
