__kernel void cl_texturize_canvas(__global const float * in,
                                  __global       float * out,
                                  __global       float * sdata,
                                           const int     x,
                                           const int     y,
                                           const int     xm,
                                           const int     ym,
                                           const int     offs,
                                           const float   mult,
                                           const int     components,
                                           const int     has_alpha)
{
    int col = get_global_id(0);
    int row = get_global_id(1);
    int step = components + has_alpha;
    int index = step * (row * get_global_size(0) + col);
    int canvas_index = ((x + col) & 127) * xm +
                       ((y + row) & 127) * ym + offs;
    float color;
    int i;
    float tmp = mult * sdata[canvas_index];
    for(i=0; i<components; ++i)
    {
       color = tmp + in[index];
       out[index++] = clamp(color,0.0f,1.0f);
    }
    if(has_alpha)
       out[index] = in[index];
}
