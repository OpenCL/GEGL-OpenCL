float4
get_pixel_color (__global const float4 *in,
                          const int     rect_width,
                          const int     rect_height,
                          const int     rect_x,
                          const int     rect_y,
                          const int     x,
                          const int     y)
{
  const int ix = clamp (x - rect_x, 0, rect_width - 1);
  const int iy = clamp (y - rect_y, 0, rect_height - 1);
  return in[iy * rect_width + ix];
}

float4
bilinear_mix (const float4 p00,
              const float4 p01,
              const float4 p10,
              const float4 p11,
              const float dx,
              const float dy)
{
  return mix (mix (p00, p10, dy), mix (p01, p11, dy), dx);
}

float
compute_phi (float xr,
             float yr)
{
  return atan2 (yr, xr);
}

#define NOMINAL_NUM_IT 100

kernel void
cl_motion_blur_circular (__global const float4 *in,
                         __global       float4 *out,
                                  const int     src_width,
                                  const int     src_height,
                                  const int     src_x,
                                  const int     src_y,
                                  const int     dst_x,
                                  const int     dst_y,
                                  const int     whole_region_width,
                                  const int     whole_region_height,
                                  const int     whole_region_x,
                                  const int     whole_region_y,
                                  const float   angle,
                                  const float   center_x,
                                  const float   center_y)
{
  const int gidx = get_global_id(0);
  const int gidy = get_global_id(1);

  const int x = gidx + dst_x;
  const int y = gidy + dst_y;

  const float xr = x - center_x;
  const float yr = y - center_y;
  const float radius = hypot (xr, yr);

  /* This is not the "real" length, a bit shorter */
  const float arc_length = radius * angle * M_SQRT2_F;

  /* ensure quality with small angles */
  int n = max ((int)ceil (arc_length), 3);

  /* performance concern */
  if (n > NOMINAL_NUM_IT)
    n = NOMINAL_NUM_IT + (int) sqrt ((float)(n - NOMINAL_NUM_IT));

  const float phi_base = compute_phi (xr, yr);
  const float phi_start = phi_base + angle / 2.0f;
  const float phi_step = angle / (float)n;

  float4 sum = (float4)0.0f;
  int count = 0;

  /* Iterate over the arc */
  for (int i = 0; i < n; i++)
    {
      float s_val, c_val;
      s_val = sincos (phi_start - i * phi_step, &c_val);

      const float fx = center_x + radius * c_val;
      const float fy = center_y + radius * s_val;
      const int ix = (int)floor (fx);
      const int iy = (int)floor (fy);

      if (ix >= whole_region_x && ix < whole_region_x + whole_region_width &&
          iy >= whole_region_y && iy < whole_region_y + whole_region_height)
        {
          /* do bilinear interpolation to get a nice smooth result */
          const float4 p00 = get_pixel_color (
            in, src_width, src_height, src_x, src_y, ix, iy);
          const float4 p01 = get_pixel_color (
            in, src_width, src_height, src_x, src_y, ix + 1, iy);
          const float4 p10 = get_pixel_color (
            in, src_width, src_height, src_x, src_y, ix, iy + 1);
          const float4 p11 = get_pixel_color (
            in, src_width, src_height, src_x, src_y, ix + 1, iy + 1);
          sum += bilinear_mix(p00, p01, p10, p11, fx - ix, fy - iy);
          count++;
        }
    }

  float4 out_v;
  if (count == 0)
    out_v = get_pixel_color (in, src_width, src_height, src_x, src_y, x, y);
  else
    out_v = sum / (float) count;
  out[gidy * get_global_size(0) + gidx] = out_v;
}
