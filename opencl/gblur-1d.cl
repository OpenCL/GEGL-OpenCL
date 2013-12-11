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
 * Copyright 2013 Téo Mazars      <teomazars@gmail.com>
 */

__kernel void fir_ver_blur(const global float4 *src_buf,
                                 global float4 *dst_buf,
                           const global float  *cmatrix,
                           const        int     clen)
{
    const int gidx          = get_global_id (0);
    const int gidy          = get_global_id (1);
    const int src_rowstride = get_global_size (0);
    const int dst_rowstride = get_global_size (0);

    const int half_clen     = clen / 2;

    const int src_offset    = gidx + (gidy + half_clen) * src_rowstride;
    const int dst_offset    = gidx +  gidy              * dst_rowstride;

    const int src_start_ind = src_offset - half_clen * src_rowstride;

    float4 v = 0.0f;

    for (int i = 0; i < clen; i++)
      {
        v += src_buf[src_start_ind + i * src_rowstride] * cmatrix[i];
      }

    dst_buf[dst_offset] = v;
}


__kernel void fir_hor_blur(const global float4 *src_buf,
                                 global float4 *dst_buf,
                           const global float  *cmatrix,
                           const        int     clen)
{
    const int gidx          = get_global_id (0);
    const int gidy          = get_global_id (1);
    const int src_rowstride = get_global_size (0) + clen - 1; /*== 2*(clen/2) */
    const int dst_rowstride = get_global_size (0);

    const int half_clen     = clen / 2;

    const int src_offset    = gidx + gidy * src_rowstride + half_clen;
    const int dst_offset    = gidx + gidy * dst_rowstride;

    const int src_start_ind = src_offset - half_clen;

    float4 v = 0.0f;

    for (int i = 0; i < clen; i++)
      {
        v += src_buf[src_start_ind + i] * cmatrix[i];
      }

    dst_buf[dst_offset] = v;
}
