#ifndef __AFFINE_MATRIX_H__
#define __AFFINE_MATRIX_H__

#include <glib.h>

typedef gdouble Matrix3 [3][3];

void       matrix3_identity        (Matrix3 matrix);
gboolean   matrix3_equal           (Matrix3 matrix1,
                                    Matrix3 matrix2);
gboolean   matrix3_is_identity     (Matrix3 matrix);
gboolean   matrix3_is_scale        (Matrix3 matrix);
gboolean   matrix3_is_translate    (Matrix3 matrix);
void       matrix3_copy            (Matrix3 dst,
                                    Matrix3 src);
gdouble    matrix3_determinant     (Matrix3 matrix);
void       matrix3_invert          (Matrix3 matrix);
void       matrix3_multiply        (Matrix3 left,
                                    Matrix3 right,
                                    Matrix3 product);
void       matrix3_originate       (Matrix3 matrix,
                                    gdouble x,
                                    gdouble y);
void       matrix3_transform_point (Matrix3   matrix,
                                    gdouble *x,
                                    gdouble *y);

#endif
