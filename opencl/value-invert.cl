__kernel void gegl_value_invert (__global const float4     *in,
                                 __global       float4     *out)
{
  int gid = get_global_id(0);
  float4 in_v  = in[gid];
  float4 out_v;

  float value = fmax (in_v.x, fmax (in_v.y, in_v.z));
  float minv  = fmin (in_v.x, fmin (in_v.y, in_v.z));
  float delta = value - minv;

  if (value == 0.0f || delta == 0.0f)
    {
      out_v = (float4) ((1.0f - value),
                        (1.0f - value),
                        (1.0f - value),
                        in_v.w);
    }
  else
    {
      out_v = (float4) ((1.0f - value) * in_v.x / value,
                        (1.0f - value) * in_v.y / value,
                        (1.0f - value) * in_v.z / value,
                        in_v.w);
    }

  out_v.w   =  in_v.w;
  out[gid]  =  out_v;
}

