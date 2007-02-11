#ifndef __AFFINE_NEAREST_H__
#define __AFFINE_NEAREST_H__

#include <gegl-plugin.h>
#include "matrix.h"

void       affine_nearest          (GeglBuffer *input,
                                    GeglBuffer *output,
                                    Matrix3     matrix);
void       scale_nearest           (GeglBuffer *input,
                                    GeglBuffer *output,
                                    Matrix3     matrix);

#endif
