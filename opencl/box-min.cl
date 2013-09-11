__kernel void kernel_min_hor (__global const float4     *in,
                              __global       float4     *aux,
                              int width, int radius)
{
  const int in_index = get_global_id(0) * (width + 2 * radius)
                       + (radius + get_global_id (1));

  const int aux_index = get_global_id(0) * width + get_global_id (1);
  int i;
  float4 min;
  float4 in_v;

  min = (float4)(FLT_MAX);

  if (get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          in_v = in[in_index + i];
          min = min > in_v ? in_v : min;
        }
        aux[aux_index] = min;
    }
}

__kernel void kernel_min_ver (__global const float4     *aux,
                              __global       float4     *out,
                              int width, int radius)
{

  const int out_index = get_global_id(0) * width + get_global_id (1);
  int aux_index = out_index;
  int i;
  float4 min;
  float4 aux_v;

  min = (float4)(FLT_MAX);

  if(get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          aux_v = aux[aux_index];
          min = min > aux_v ? aux_v : min;
          aux_index += width;
        }
        out[out_index] = min;
    }
}
