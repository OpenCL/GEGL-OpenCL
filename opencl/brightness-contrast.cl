__kernel void gegl_brightness_contrast(__global const float4     *in,
                                       __global       float4     *out,
                                       float contrast,
                                       float brightness)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v.xyz = (in_v.xyz - 0.5f) * contrast + brightness + 0.5f;
  out_v.w   =  in_v.w;
  out[gid]  =  out_v;
}
