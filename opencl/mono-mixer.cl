__kernel void gegl_mono_mixer (__global const float4 *src_buf,
                               __global       float2 *dst_buf,
                               const int              preserve_luminocity,
                               float                  red,
                               float                  green,
                               float                  blue)
{
  int gid = get_global_id(0);
  float4 in_v = src_buf[gid];
  float norm_factor = 1.0f;

  if (preserve_luminocity)
    {
      float sum = red + green + blue;
      if (sum == 0.0)
        norm_factor = 1.0f;
      else
        norm_factor = fabs (1.0f / sum);
    }

  dst_buf[gid].x = (in_v.x * red + in_v.y * green + in_v.z * blue) * norm_factor;
  dst_buf[gid].y = in_v.w;
}
