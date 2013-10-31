__kernel void kernel_max_hor (__global const float4     *in,
                              __global       float4     *aux,
                              int width, int radius)
{
  const int in_index = get_global_id(0) * (width + 2 * radius)
                       + (radius + get_global_id (1));

  const int aux_index = get_global_id(0) * width + get_global_id (1);
  int i;
  float4 max;
  float4 in_v;

  max = (float4)(-FLT_MAX);

  if (get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          in_v = in[in_index + i];
          max = max < in_v ? in_v : max;
        }
        aux[aux_index] = max;
    }
}

__kernel void kernel_max_ver (__global const float4     *aux,
                              __global       float4     *out,
                              int width, int radius)
{

  const int out_index = get_global_id(0) * width + get_global_id (1);
  int aux_index = out_index;
  int i;
  float4 max;
  float4 aux_v;

  max = (float4)(-FLT_MAX);

  if(get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          aux_v = aux[aux_index];
          max = max < aux_v ? aux_v : max;
          aux_index += width;
        }
        out[out_index] = max;
    }
}
