__kernel void fir_ver_blur(const global float4 *src_buf,
                              const int src_width,
                              global float4 *dst_buf,
                              constant float *cmatrix,
                              const int matrix_length,
                              const int yoff)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
    int gid  = gidx + gidy * get_global_size(0);

    int radius = matrix_length / 2;
    int src_offset = gidx + (gidy + yoff) * src_width;

    float4 v = 0.0f;

    for (int i=-radius; i <= radius; i++)
      {
        v += src_buf[src_offset + i * src_width] * cmatrix[i+radius];
      }

    dst_buf[gid] = v;
}

__kernel void fir_hor_blur(const global float4 *src_buf,
                              const int src_width,
                              global float4 *dst_buf,
                              constant float *cmatrix,
                              const int matrix_length,
                              const int xoff)
{
    int gidx = get_global_id(0);
    int gidy = get_global_id(1);
    int gid  = gidx + gidy * get_global_size(0);

    int radius = matrix_length / 2;
    int src_offset = gidy * src_width + (gidx + xoff);

    float4 v = 0.0f;

    for (int i=-radius; i <= radius; i++)
      {
        v += src_buf[src_offset + i] * cmatrix[i+radius];
      }

    dst_buf[gid] = v;
}
