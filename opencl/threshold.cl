__kernel void gegl_threshold (__global const float2     *in,
                              __global const float      *aux,
                              __global       float2     *out,
                             float value)
{
  int gid = get_global_id(0);
  float2 in_v  = in [gid];
  float  aux_v = (aux)? aux[gid] : value;
  float2 out_v;
  out_v.x = (in_v.x > aux_v)? 1.0f : 0.0f;
  out_v.y = in_v.y;
  out[gid]  =  out_v;
}
