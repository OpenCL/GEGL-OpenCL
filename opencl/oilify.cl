/* two small different kernels are better than one big */

#define NUM_INTENSITIES 256

kernel void kernel_oilify(global float4 *in,
                             global float4 *out,
                             const int mask_radius,
                             const int intensities,
                             const float exponent)
{
  int gidx = get_global_id(0);
  int gidy = get_global_id(1);
  int x = gidx + mask_radius;
  int y = gidy + mask_radius;
  int dst_width = get_global_size(0);
  int src_width = dst_width + mask_radius * 2;
  float4 hist[NUM_INTENSITIES];
  float4 hist_max = 1.0;
  int i, j, intensity;
  int radius_sq = mask_radius * mask_radius;
  float4 temp_pixel;
  for (i = 0; i < intensities; i++)
    hist[i] = 0.0;

  for (i = -mask_radius; i <= mask_radius; i++)
  {
    for (j = -mask_radius; j <= mask_radius; j++)
      {
        if (i*i + j*j <= radius_sq)
          {
            temp_pixel = in[x + i + (y + j) * src_width];
            hist[(int)(temp_pixel.x * (intensities - 1))].x+=1;
            hist[(int)(temp_pixel.y * (intensities - 1))].y+=1;
            hist[(int)(temp_pixel.z * (intensities - 1))].z+=1;
            hist[(int)(temp_pixel.w * (intensities - 1))].w+=1;
          }
      }
  }

  for (i = 0; i < intensities; i++) {
    if(hist_max.x < hist[i].x)
      hist_max.x = hist[i].x;
    if(hist_max.y < hist[i].y)
      hist_max.y = hist[i].y;
    if(hist_max.z < hist[i].z)
      hist_max.z = hist[i].z;
    if(hist_max.w < hist[i].w)
      hist_max.w = hist[i].w;
  }
  float4 div = 0.0;
  float4 sum = 0.0;
  float4 ratio, weight;
  for (i = 0; i < intensities; i++)
  {
    ratio = hist[i] / hist_max;
    weight = pow(ratio, (float4)exponent);
    sum += weight * (float4)i;
    div += weight;
  }
  out[gidx + gidy * dst_width] = sum / div / (float)(intensities - 1);
}

kernel void kernel_oilify_inten(global float4 *in,
                             global float4 *out,
                             const int mask_radius,
                             const int intensities,
                             const float exponent)
{
  int gidx = get_global_id(0);
  int gidy = get_global_id(1);
  int x = gidx + mask_radius;
  int y = gidy + mask_radius;
  int dst_width = get_global_size(0);
  int src_width = dst_width + mask_radius * 2;
  float4 cumulative_rgb[NUM_INTENSITIES];
  int hist_inten[NUM_INTENSITIES], inten_max;
  int i, j, intensity;
  int radius_sq = mask_radius * mask_radius;
  float4 temp_pixel;
  for (i = 0; i < intensities; i++)
  {
    hist_inten[i] = 0;
    cumulative_rgb[i] = 0.0;
  }
  for (i = -mask_radius; i <= mask_radius; i++)
  {
    for (j = -mask_radius; j <= mask_radius; j++)
      {
        if (i*i + j*j <= radius_sq)
          {
            temp_pixel = in[x + i + (y + j) * src_width];
            /*Calculate intensity on the fly, GPU does it fast*/
            intensity = (int)((0.299 * temp_pixel.x
                      +0.587 * temp_pixel.y
                      +0.114 * temp_pixel.z) * (float)(intensities-1));
            hist_inten[intensity] += 1;
            cumulative_rgb[intensity] += temp_pixel;
          }
      }
  }
  inten_max = 1;

  /* calculated maximums */
  for (i = 0; i < intensities; i++) {
    if(hist_inten[i] > inten_max)
      inten_max = hist_inten[i];
  }
  float div = 0.0;
  float ratio, weight, mult_inten;

  float4 color = 0.0;
  for (i = 0; i < intensities; i++)
  {
    if (hist_inten[i] > 0)
    {
      ratio = (float)(hist_inten[i]) / (float)(inten_max);
      weight = pow(ratio, exponent);
      mult_inten = weight / (float)(hist_inten[i]);

      div += weight;
      color += mult_inten * cumulative_rgb[i];
    }
  }
  out[gidx + gidy * dst_width] = color/div;
}
