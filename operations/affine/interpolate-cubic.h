#ifndef __AFFINE_CUBIC_H__
#define __AFFINE_CUBIC_H__

#include <gegl.h>
#include "matrix.h"

void affine_cubic (GeglBuffer *input,
                   GeglBuffer *output,
                   Matrix3     matrix);
void scale_cubic  (GeglBuffer *input,
                   GeglBuffer *output,
                   Matrix3     matrix);

#endif
