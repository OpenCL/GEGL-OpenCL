__kernel void gegl_color_temperature(__global const float4     *in,
                                     __global       float4     *out,
                                     float coeff1,
                                     float coeff2,
                                     float coeff3)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;
  out_v = in_v * (float4) (coeff1, coeff2, coeff3, 1.0f);
  out[gid]  =  out_v;
}
