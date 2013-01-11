__kernel void gegl_mono_mixer (__global const float4 *src_buf,
                               __global       float2 *dst_buf,
                               float                  red,
                               float                  green,
                               float                  blue)
{
  int gid = get_global_id(0);
  float4 in_v = src_buf[gid];
  dst_buf[gid].x = in_v.x * red + in_v.y * green + in_v.z * blue;
  dst_buf[gid].y = in_v.w;
}
