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
 * Copyright 2012 Victor Oliveira (victormatheus@gmail.com)
 */

/* This is almost a copy-paste from babl/base color conversion functions

   XXX: This code is very hard to maintain and keep in sync with BABL, we should
        think in something better
*/

/* Alpha threshold used in the reference implementation for
 * un-pre-multiplication of color data:
 *
 * 0.01 / (2^16 - 1)
 */
#define BABL_ALPHA_THRESHOLD 0.0f

float linear_to_gamma_2_2 (float value);
float gamma_2_2_to_linear (float value);

/* babl reference file: babl/base/util.h */
float linear_to_gamma_2_2 (float value)
{
  if (value > 0.003130804954f)
    return 1.055f * native_powr (value, (1.0f/2.4f)) - 0.055f;
  return 12.92f * value;
}

float gamma_2_2_to_linear (float value)
{
  if (value > 0.04045f)
    return native_powr ((value + 0.055f) / 1.055f, 2.4f);
  return value / 12.92f;
}

/* -- RGBA float/u8 -- */

/* RGBA u8 -> RGBA float */
__kernel void rgbau8_to_rgbaf (__global const uchar4 * in,
                               __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = convert_float4(in[gid]) / 255.0f;
  float4 out_v = in_v;
  out[gid] = out_v;
}

/* RGBA float -> RGBA u8 */
__kernel void rgbaf_to_rgbau8 (__global const float4 * in,
                               __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v = in_v;
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}


/* RGBA float -> RGB float */
__kernel void rgbaf_to_rgbf (__global const float4 * in,
                             __global       float  * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];

#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  vstore3 (in_v.xyz, gid, out);
#else
  out[3 * gid]     = in_v.x;
  out[3 * gid + 1] = in_v.y;
  out[3 * gid + 2] = in_v.z;
#endif
}

/* -- RaGaBaA float -- */

/* RGBA float -> RaGaBaA float */
__kernel void rgbaf_to_ragabaf (__global const float4 * in,
                                __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v = in[gid];
  float4 out_v;
  out_v   = in_v * in_v.w;
  out_v.w = in_v.w;
  out[gid] = out_v;
}


/* RaGaBaA float -> RGBA float */
__kernel void ragabaf_to_rgbaf (__global const float4 * in,
                                __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);
  out_v.w = in_v.w;
  out[gid] = out_v;
}

/* RGBA u8 -> RaGaBaA float */
__kernel void rgbau8_to_ragabaf (__global const uchar4 * in,
                                 __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = convert_float4(in[gid]) / 255.0f;
  float4 out_v;
  out_v   = in_v * in_v.w;
  out_v.w = in_v.w;
  out[gid] = out_v;
}


/* RaGaBaA float -> RGBA u8 */
__kernel void ragabaf_to_rgbau8 (__global const float4 * in,
                                 __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);
  out_v.w = in_v.w;
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* RaGaBaA float -> RGBA_GAMMA_U8 */
__kernel void ragabaf_to_rgba_gamma_u8 (__global const float4 * in,
                                        __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 tmp_v;
  tmp_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);
  tmp_v.w = in_v.w;
  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),
                   linear_to_gamma_2_2(tmp_v.y),
                   linear_to_gamma_2_2(tmp_v.z),
                   tmp_v.w);
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* RaGaBaA float -> RGB_GAMMA_U8 */
__kernel void ragabaf_to_rgb_gamma_u8 (__global const float4 * in,
                                       __global       uchar  * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 tmp_v;
  tmp_v = (in_v.w > BABL_ALPHA_THRESHOLD)? in_v / in_v.w : (float4)(0.0f);
  tmp_v.w = in_v.w;
  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),
                   linear_to_gamma_2_2(tmp_v.y),
                   linear_to_gamma_2_2(tmp_v.z),
                   tmp_v.w);
#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  vstore3 (convert_uchar3_sat_rte(255.0f * out_v.xyz), gid, out);
#else
  uchar4 sat = convert_uchar4_sat_rte (255.0f * out_v);
  out[3 * gid]     = sat.x;
  out[3 * gid + 1] = sat.y;
  out[3 * gid + 2] = sat.z;
#endif
}

/* -- R'G'B' float -- */

/* R'G'B' float -> RGBA float */
__kernel void rgb_gamma_f_to_rgbaf (__global const float * in,
                                    __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 out_v;

#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  float3 in_v;
  in_v = vload3 (gid, in);
  out_v = (float4)(gamma_2_2_to_linear(in_v.x),
                   gamma_2_2_to_linear(in_v.y),
                   gamma_2_2_to_linear(in_v.z),
                   1.0f);
#else
  float r = in[3 * gid + 0];
  float g = in[3 * gid + 1];
  float b = in[3 * gid + 2];
  out_v = (float4)(gamma_2_2_to_linear(r),
                   gamma_2_2_to_linear(g),
                   gamma_2_2_to_linear(b),
                   1.0f);
#endif
  out[gid] = out_v;
}

/* -- R'G'B'A float -- */

/* rgba float -> r'g'b'a float */
__kernel void rgbaf_to_rgba_gamma_f (__global const float4 * in,
                                     __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(in_v.x),
                   linear_to_gamma_2_2(in_v.y),
                   linear_to_gamma_2_2(in_v.z),
                   in_v.w);
  out[gid] = out_v;
}

/* r'g'b'a float -> rgba float */
__kernel void rgba_gamma_f_to_rgbaf (__global const float4 * in,
                                     __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (float4)(gamma_2_2_to_linear(in_v.x),
                   gamma_2_2_to_linear(in_v.y),
                   gamma_2_2_to_linear(in_v.z),
                   in_v.w);
  out[gid] = out_v;
}

/* rgba u8 -> r'g'b'a float */
__kernel void rgbau8_to_rgba_gamma_f (__global const uchar4 * in,
                                      __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = convert_float4(in[gid]) / 255.0f;
  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(in_v.x),
                   linear_to_gamma_2_2(in_v.y),
                   linear_to_gamma_2_2(in_v.z),
                   in_v.w);
  out[gid] = out_v;
}

/* r'g'b'a float -> rgba u8 */
__kernel void rgba_gamma_f_to_rgbau8 (__global const float4 * in,
                                      __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (float4)(gamma_2_2_to_linear(in_v.x),
                   gamma_2_2_to_linear(in_v.y),
                   gamma_2_2_to_linear(in_v.z),
                   in_v.w);
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* -- Y'CbCrA float -- */

/* RGBA float -> Y'CbCrA float */
__kernel void rgbaf_to_ycbcraf (__global const float4 * in,
                                __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;

#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  float3 rgb = (float3)(linear_to_gamma_2_2(in_v.x),
                        linear_to_gamma_2_2(in_v.y),
                        linear_to_gamma_2_2(in_v.z));
#else
  float4 rgb = (float4)(linear_to_gamma_2_2(in_v.x),
                        linear_to_gamma_2_2(in_v.y),
                        linear_to_gamma_2_2(in_v.z), 1.0f);
#endif

  out_v = (float4)( 0.299f    * rgb.x + 0.587f    * rgb.y + 0.114f    * rgb.z,
                   -0.168736f * rgb.x - 0.331264f * rgb.y + 0.5f      * rgb.z,
                    0.5f      * rgb.x - 0.418688f * rgb.y - 0.081312f * rgb.z,
                   in_v.w);
  out[gid] = out_v;
}

/* Y'CbCrA float -> RGBA float */
__kernel void ycbcraf_to_rgbaf (__global const float4 * in,
                                __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;

  float4 rgb = (float4)(1.0f * in_v.x + 0.0f      * in_v.y + 1.40200f    * in_v.z,
                        1.0f * in_v.x - 0.344136f * in_v.y - 0.71414136f * in_v.z,
                        1.0f * in_v.x + 1.772f    * in_v.y + 0.0f        * in_v.z,
                        0.0f);

  out_v = (float4)(gamma_2_2_to_linear(rgb.x),
                   gamma_2_2_to_linear(rgb.y),
                   gamma_2_2_to_linear(rgb.z),
                   in_v.w);
  out[gid] = out_v;
}

/* RGBA u8 -> Y'CbCrA float */
__kernel void rgbau8_to_ycbcraf (__global const uchar4 * in,
                                 __global       float4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = convert_float4(in[gid]) / 255.0f;
  float4 out_v;

  float4 rgb = (float4)(linear_to_gamma_2_2(in_v.x),
                        linear_to_gamma_2_2(in_v.y),
                        linear_to_gamma_2_2(in_v.z),
                        0.0f);

  out_v = (float4)( 0.299f    * rgb.x + 0.587f    * rgb.y + 0.114f    * rgb.z,
                   -0.168736f * rgb.x - 0.331264f * rgb.y + 0.5f      * rgb.z,
                    0.5f      * rgb.x - 0.418688f * rgb.y - 0.081312f * rgb.z,
                   in_v.w);
  out[gid] = out_v;
}

/* Y'CbCrA float -> RGBA u8 */
__kernel void ycbcraf_to_rgbau8 (__global const float4 * in,
                                 __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;

  float4 rgb = (float4)(1.0f * in_v.x + 0.0f      * in_v.y + 1.40200f    * in_v.z,
                        1.0f * in_v.x - 0.344136f * in_v.y - 0.71414136f * in_v.z,
                        1.0f * in_v.x + 1.772f    * in_v.y + 0.0f        * in_v.z,
                        0.0f);

  out_v = (float4)(gamma_2_2_to_linear(rgb.x),
                   gamma_2_2_to_linear(rgb.y),
                   gamma_2_2_to_linear(rgb.z),
                   in_v.w);
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* -- RGB u8 -- */

/* RGB u8 -> RGBA float */
__kernel void rgbu8_to_rgbaf (__global const uchar  * in,
                              __global       float4 * out)
{
  int gid = get_global_id(0);
#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  uchar3 in_v;
  float4 out_v;
  in_v = vload3 (gid, in);
  out_v.xyz = convert_float3(in_v) / 255.0f;
  out_v.w   = 1.0f;
#else
  float4 in_v = (float4) (in[3 * gid], in[3 * gid + 1], in[3 * gid + 2], 255.0f);
  float4 out_v = in_v / (float4) (255.0f);
#endif
  out[gid]  = out_v;
}

/* RGBA float -> RGB u8 */
__kernel void rgbaf_to_rgbu8 (__global const float4 * in,
                              __global       uchar  * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  uchar3 out_v = convert_uchar3_sat_rte(255.0f * in_v.xyz);
  vstore3 (out_v, gid, out);
#else
  uchar4 out_v = convert_uchar4_sat_rte(255.0f * in_v);
  out[3 * gid]     = out_v.x;
  out[3 * gid + 1] = out_v.y;
  out[3 * gid + 2] = out_v.z;
#endif
}

/* -- Y u8 -- */

/* Y u8 -> Y float */
__kernel void yu8_to_yf (__global const uchar * in,
                         __global       float * out)
{
  int gid = get_global_id(0);
  float in_v  = convert_float (in[gid]) / 255.0f;
  float out_v;
  out_v = in_v;
  out[gid] = out_v;
}

/* -- Y float -- */

/* Y float -> RaGaBaA float */
__kernel void yf_to_ragabaf (__global const float * in,
                              __global       float4 * out)
{
  int gid = get_global_id(0);
  float  y  = in[gid];
  float4 out_v = (float4) (y, y, y, 1.0f);

  out[gid] = out_v;
}

/* -- YA float -- */

/* babl reference file: babl/base/rgb-constants.h */
#define RGB_LUMINANCE_RED    (0.222491f)
#define RGB_LUMINANCE_GREEN  (0.716888f)
#define RGB_LUMINANCE_BLUE   (0.060621f)

/* RGBA float -> YA float */
__kernel void rgbaf_to_yaf (__global const float4 * in,
                            __global       float2 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float2 out_v;

  float luminance = in_v.x * RGB_LUMINANCE_RED +
                    in_v.y * RGB_LUMINANCE_GREEN +
                    in_v.z * RGB_LUMINANCE_BLUE;

  out_v.x = luminance;
  out_v.y = in_v.w;

  out[gid] = out_v;
}

/* RaGaBaA float -> YA float */
__kernel void ragabaf_to_yaf (__global const float4 * in,
                              __global       float2 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 tmp_v = in_v / (float4) (in_v.w);

  float luminance = tmp_v.x * RGB_LUMINANCE_RED +
                    tmp_v.y * RGB_LUMINANCE_GREEN +
                    tmp_v.z * RGB_LUMINANCE_BLUE;

  out[gid] = (float2) (luminance, in_v.w);
}

/* YA float -> RGBA float */
__kernel void yaf_to_rgbaf (__global const float2 * in,
                            __global       float4 * out)
{
  int gid = get_global_id(0);
  float2 in_v  = in[gid];
  float4 out_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);

  out[gid] = out_v;
}

/* YA float -> RaGaBaA float */
__kernel void yaf_to_ragabaf (__global const float2 * in,
                              __global       float4 * out)
{
  int gid = get_global_id(0);
  float2 in_v  = in[gid];
  float4 out_v = (float4) (in_v.x * in_v.y,
                           in_v.x * in_v.y,
                           in_v.x * in_v.y,
                           in_v.y);

  out[gid] = out_v;
}

/* RGBA u8 -> YA float */
__kernel void rgbau8_to_yaf (__global const uchar4 * in,
                             __global       float2 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = convert_float4(in[gid]) / 255.0f;
  float2 out_v;

  float luminance = in_v.x * RGB_LUMINANCE_RED +
                    in_v.y * RGB_LUMINANCE_GREEN +
                    in_v.z * RGB_LUMINANCE_BLUE;

  out_v.x = luminance;
  out_v.y = in_v.w;

  out[gid] = out_v;
}

/* YA float -> RGBA u8 */
__kernel void yaf_to_rgbau8 (__global const float2 * in,
                             __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float2 in_v  = in[gid];
  float4 out_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);

  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* YA float -> R'G'B'A u8 */
__kernel void yaf_to_rgba_gamma_u8 (__global const float2 * in,
                                    __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float2 in_v  = in[gid];
  float4 tmp_v = (float4) (in_v.x, in_v.x, in_v.x, in_v.y);

  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(tmp_v.x),
                   linear_to_gamma_2_2(tmp_v.y),
                   linear_to_gamma_2_2(tmp_v.z),
                   tmp_v.w);
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* YA float -> R'G'B' u8 */
__kernel void yaf_to_rgb_gamma_u8 (__global const float2 * in,
                                   __global       uchar  * out)
{
  int gid = get_global_id(0);
  float2 in_v  = in[gid];
  uchar  tmp = convert_uchar_sat_rte (255.0f * linear_to_gamma_2_2 (in_v.x));

#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  vstore3 ((uchar3)tmp, gid, out);
#else
  out[3 * gid] = out[3 * gid + 1] = out[3 * gid + 2] = tmp;
#endif
}

/* R'G'B'A u8 */

/* rgba float -> r'g'b'a u8 */
__kernel void rgbaf_to_rgba_gamma_u8 (__global const float4 * in,
                                      __global       uchar4 * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = (float4)(linear_to_gamma_2_2(in_v.x),
                   linear_to_gamma_2_2(in_v.y),
                   linear_to_gamma_2_2(in_v.z),
                   in_v.w);
  out[gid] = convert_uchar4_sat_rte(255.0f * out_v);
}

/* R'G'B' u8 */

/* rgba float -> r'g'b' u8 */
__kernel void rgbaf_to_rgb_gamma_u8 (__global const float4 * in,
                                     __global       uchar  * out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 tmp_v;
  tmp_v = (float4)(linear_to_gamma_2_2(in_v.x),
                   linear_to_gamma_2_2(in_v.y),
                   linear_to_gamma_2_2(in_v.z),
                   in_v.w);
#if (__OPENCL_VERSION__ != CL_VERSION_1_0)
  uchar3 out_v;
  out_v = convert_uchar3_sat_rte(255.0f * tmp_v.xyz);
  vstore3 (out_v, gid, out);
#else
  uchar4 out_v = convert_uchar4_sat_rte (255.0f * tmp_v);
  out[3 * gid]     = out_v.x;
  out[3 * gid + 1] = out_v.y;
  out[3 * gid + 2] = out_v.z;
#endif
}
