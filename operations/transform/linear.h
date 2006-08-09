#ifndef __AFFINE_LINEAR_H__
#define __AFFINE_LINEAR_H__

#include <gegl.h>
#include "matrix.h"

void       affine_linear           (GeglBuffer *input,
                                    GeglBuffer *output,
                                    Matrix3     matrix,
                                    gboolean    hard_edges);
void       scale_linear            (GeglBuffer *input,
                                    GeglBuffer *output,
                                    Matrix3     matrix,
                                    gboolean    hard_edges);

#endif
