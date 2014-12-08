inline int tile_index (int coordinate, int stride)      
{                                                        
  int a = (coordinate < 0);                              
  return ((coordinate + a) / stride) - a;                
}                                                        
                                                         
__kernel void kernel_checkerboard (__global float4 *out, 
                                   float4 color1,        
                                   float4 color2,        
                                   int square_width,     
                                   int square_height,    
                                   int x_offset,         
                                   int y_offset)         
{                                                        
    size_t roi_width = get_global_size(0);               
    size_t roi_x     = get_global_offset(0);             
    size_t roi_y     = get_global_offset(1);             
    size_t gidx      = get_global_id(0) - roi_x;         
    size_t gidy      = get_global_id(1) - roi_y;         
                                                         
    int x = get_global_id(0) - x_offset;                 
    int y = get_global_id(1) - y_offset;                 
                                                         
    int tilex = tile_index (x, square_width);            
    int tiley = tile_index (y, square_height);           
    out[gidx + gidy * roi_width] = (tilex + tiley) & 1 ? 
                                   color2 : color1;      
}                                                        
