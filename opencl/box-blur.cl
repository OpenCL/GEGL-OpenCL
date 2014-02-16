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
      aux[aux_index] = mean / (float)(2 * radius + 1);
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

  if (get_global_id(1) < width)
    {
      for (i=-radius; i <= radius; i++)
        {
          mean += aux[aux_index];
          aux_index += width;
        }
      out[out_index] = mean / (float)(2 * radius + 1);
    }
}

__kernel
__attribute__((reqd_work_group_size(256,1,1)))
void kernel_box_blur_fast (__global const float4 *in,
                           __global       float4 *out,
                           __local        float4 *column_sum,
                                    const int     width,
                                    const int     height,
                                    const int     radius,
                                    const int     size)
{
  const int local_id0 = get_local_id(0);
  const int twice_radius = 2 * radius;
  const int in_width = twice_radius + width;
  const int in_height = twice_radius + height;
  const float4 area = (float4)( (twice_radius + 1) * (twice_radius + 1) );
  int column_index_start,column_index_end;
  int y = get_global_id(1) * size;
  const int out_x = get_group_id(0)
                    * ( get_local_size(0) - twice_radius ) + local_id0 - radius;
  const int in_x = out_x + radius;
  int tmp_size = size;
  int tmp_index = 0;
  float4 tmp_sum = (float4)0.0f;
  float4 total_sum = (float4)0.0f;

  if (in_x < in_width)
    {
      column_index_start = y;
      column_index_end = y + twice_radius;

      for (int i = 0; i < twice_radius + 1; ++i)
        {
          tmp_sum += in[(y + i) * in_width + in_x];
        }

      column_sum[local_id0] = tmp_sum;
    }

  barrier(CLK_LOCAL_MEM_FENCE);

  while (1)
    {
      if (out_x < width)
        {
          if (local_id0 >= radius && local_id0 < get_local_size(0) - radius)
            {
                total_sum = (float4)0.0f;

                for (int i = 0; i < twice_radius + 1; ++i)
                  {
                    total_sum += column_sum[local_id0 - radius + i];
                  }

                out[y * width + out_x] = total_sum / area;
            }
        }

    if (--tmp_size == 0 || y == height - 1)
      break;

    barrier(CLK_LOCAL_MEM_FENCE);

    ++y;

    if (in_x < in_width)
      {
        tmp_sum = column_sum[local_id0];
        tmp_sum -= in[(column_index_start)   * in_width + in_x];
        tmp_sum += in[(column_index_end + 1) * in_width + in_x];
        ++column_index_start;
        ++column_index_end;
        column_sum[local_id0] = tmp_sum;
      }

    barrier(CLK_LOCAL_MEM_FENCE);
  }
}
