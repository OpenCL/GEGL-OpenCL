#ifndef __GEGL_MATRIX_H__
#define __GEGL_MATRIX_H__

#include <glib.h>

typedef gdouble GeglMatrix3 [3][3];

void       gegl_matrix3_identity        (GeglMatrix3 matrix);
gboolean   gegl_matrix3_equal           (GeglMatrix3 matrix1,
                                         GeglMatrix3 matrix2);
gboolean   gegl_matrix3_is_identity     (GeglMatrix3 matrix);
gboolean   gegl_matrix3_is_scale        (GeglMatrix3 matrix);
gboolean   gegl_matrix3_is_translate    (GeglMatrix3 matrix);
void       gegl_matrix3_copy            (GeglMatrix3 dst,
                                         GeglMatrix3 src);
gdouble    gegl_matrix3_determinant     (GeglMatrix3 matrix);
void       gegl_matrix3_invert          (GeglMatrix3 matrix);
void       gegl_matrix3_multiply        (GeglMatrix3 left,
                                         GeglMatrix3 right,
                                         GeglMatrix3 product);
void       gegl_matrix3_originate       (GeglMatrix3 matrix,
                                         gdouble     x,
                                         gdouble     y);
void       gegl_matrix3_transform_point (GeglMatrix3 matrix,
                                         gdouble    *x,
                                         gdouble    *y);
void       gegl_matrix3_parse_string    (GeglMatrix3 matrix,
                                         const gchar *string);
gchar *    gegl_matrix3_to_string       (GeglMatrix3 matrix);

#endif
