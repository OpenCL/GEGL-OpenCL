#include "gegl-cl-types.h"

cl_mem
gegl_cl_texture_manager_request (cl_mem_flags flags,
                                 cl_image_format format,
                                 size_t width, size_t height);

void
gegl_cl_texture_manager_give (cl_mem data);

void
gegl_cl_texture_manager_release (cl_mem data);
