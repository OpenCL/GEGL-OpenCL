__kernel void gegl_video_degradation (__global const float4 *input,
                                      __global float4 *output,
                                      __global const int *pattern,
                                      const int pat_w,
                                      const int pat_h,
                                      const int additive,
                                      const int rotated)
{
  const size_t gidx   = get_global_id(0);
  const size_t gidy   = get_global_id(1);
  const size_t gid    = gidx - get_global_offset(0) +
                       (gidy - get_global_offset(1)) *
                        get_global_size(0);
  const float4 indata = input[gid];

  /* Get channel to keep in this input pixel */
  const int sel_b = pattern[rotated ? pat_w * (gidx % pat_h) + gidy % pat_w:
                                      pat_w * (gidy % pat_h) + gidx % pat_w];

  /* Mask channels according to sel_b variable */
  float4 value = select(0.f, indata, sel_b == (int4)(0, 1, 2, 3));

  /* Add original pixel if enabled */
  if (additive)
      value = fmin(value + indata, 1.0f);

  /* Maintain original alpha channel */
  value.w = indata.w;

  output[gid] = value;
}
