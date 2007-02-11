#ifndef __AFFINE_LANCZOS_H__
#define __AFFINE_LANCZOS_H__

#include <gegl.h>
#include "matrix.h"

void affine_lanczos (GeglBuffer *input,
                     GeglBuffer *output,
                     Matrix3     matrix,
                     gint        lanczos_width);
void scale_lanczos  (GeglBuffer *input,
                     GeglBuffer *output,
                     Matrix3     matrix,
                     gint        lanczos_width);

#endif
