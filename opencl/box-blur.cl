__kernel void kernel_blur_hor (__global const float4     *in,
                               __global       float4     *aux,
                               int width, int radius)
{
  const int in_index = get_global_id(0) * (width + 2 * radius)
                       + (radius + get_global_id (1));

  const int aux_index = get_global_id(0) * width + get_global_id (1);
  int i;
  float4 mean;

  mean = (float4)(0.0f);

  if (get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          mean += in[in_index + i];
        }
      aux[aux_index] = mean / (2 * radius + 1);
    }
}

__kernel void kernel_blur_ver (__global const float4     *aux,
                               __global       float4     *out,
                               int width, int radius)
{

  const int out_index = get_global_id(0) * width + get_global_id (1);
  int i;
  float4 mean;

  mean = (float4)(0.0f);
  int aux_index = get_global_id(0) * width + get_global_id (1);

  if(get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          mean += aux[aux_index];
          aux_index += width;
        }
      out[out_index] = mean / (2 * radius + 1);
    }
}
