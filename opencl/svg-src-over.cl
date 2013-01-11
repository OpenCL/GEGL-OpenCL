__kernel void svg_src_over (__global const float4 *in,
                            __global const float4 *aux,
                            __global       float4 *out)
{
  int gid = get_global_id(0);
  float4 in_v  = in [gid];
  float4 aux_v = aux[gid];
  float4 out_v;
  out_v.xyz = aux_v.xyz + in_v.xyz * (1.0f - aux_v.w);
  out_v.w   = aux_v.w + in_v.w - aux_v.w * in_v.w;
  out[gid]  = out_v;
}
