#ifndef __GEGL_MATRIX_H__
#define __GEGL_MATRIX_H__


#include <glib.h>

G_BEGIN_DECLS

/***
 * GeglMatrix:
 *
 * #GeglMatrix a 3x3 matrix for GEGL represented by the structure:
 * Matrixes are currently used by #GeglPath and the affine operations,
 * they might be used more centrally in the core of GEGL later.
 *
 * typedef gdouble GeglMatrix3 [3][3];
 */

typedef gdouble GeglMatrix3 [3][3];
typedef gdouble GeglMatrix2 [2][2];

/**
 * gegl_matrix3_identity:
 * @matrix: a #GeglMatrix
 *
 * Set the provided @matrix to the identity matrix.
 */
void       gegl_matrix3_identity        (GeglMatrix3 matrix);

/**
 * gegl_matrix3_equal:
 * @matrix1: a #GeglMatrix
 * @matrix2: a #GeglMatrix
 *
 * Check if two matrices are equal.
 *
 * Returns TRUE if the matrices are equal.
 */
gboolean   gegl_matrix3_equal           (GeglMatrix3 matrix1,
                                         GeglMatrix3 matrix2);

/**
 * gegl_matrix3_is_identity:
 * @matrix: a #GeglMatrix
 *
 * Check if a matrix is the identity matrix.
 *
 * Returns TRUE if the matrix is the identity matrix.
 */
gboolean   gegl_matrix3_is_identity     (GeglMatrix3 matrix);

/**
 * gegl_matrix3_is_scale:
 * @matrix: a #GeglMatrix
 *
 * Check if a matrix only does scaling.
 *
 * Returns TRUE if the matrix only does scaling.
 */
gboolean   gegl_matrix3_is_scale        (GeglMatrix3 matrix);

/**
 * gegl_matrix3_is_translate:
 * @matrix: a #GeglMatrix
 *
 * Check if a matrix only does translation.
 *
 * Returns TRUE if the matrix only does trasnlation.
 */
gboolean   gegl_matrix3_is_translate    (GeglMatrix3 matrix);

/**
 * gegl_matrix3_copy:
 * @dst: a #GeglMatrix
 * @src: a #GeglMatrix
 *
 * Copies the matrix in @src into @dst.
 */
void       gegl_matrix3_copy            (GeglMatrix3 dst,
                                         GeglMatrix3 src);

/**
 * gegl_matrix3_determinant:
 * @matrix: a #GeglMatrix
 *
 * Returns the determinant for the matrix.
 */
gdouble    gegl_matrix3_determinant     (GeglMatrix3 matrix);

/**
 * gegl_matrix3_invert:
 * @matrix: a #GeglMatrix
 *
 * Inverts @matrix.
 */
void       gegl_matrix3_invert          (GeglMatrix3 matrix);

/**
 * gegl_matrix3_multiply:
 * @left: a #GeglMatrix
 * @right: a #GeglMatrix
 * @product: a #GeglMatrix to store the result in.
 *
 * Multiples @product = @left · @right
 */
void       gegl_matrix3_multiply        (GeglMatrix3 left,
                                         GeglMatrix3 right,
                                         GeglMatrix3 product);

/**
 * gegl_matrix3_originate:
 * @matrix: a #GeglMatrix
 * @x: x coordinate of new origin
 * @y: y coordinate of new origin.
 * 
 * Hmm not quite sure what this does.
 *
 */
void       gegl_matrix3_originate       (GeglMatrix3 matrix,
                                         gdouble     x,
                                         gdouble     y);


/**
 * gegl_matrix3_transform_point:
 * @matrix: a #GeglMatrix
 * @x: pointer to an x coordinate
 * @y: pointer to an y coordinate
 * 
 * transforms the coordinates provided in @x and @y and changes to the
 * coordinates gotten when the transformed with the matrix.
 *
 */
void       gegl_matrix3_transform_point (GeglMatrix3 matrix,
                                         gdouble    *x,
                                         gdouble    *y);

/**
 * gegl_matrix3_parse_string:
 * @matrix: a #GeglMatrix
 * @string: a string describing the matrix (right now a small subset of the
 * transform strings allowed by SVG)
 *
 * Parse a transofmation matrix from a string.
 */
void       gegl_matrix3_parse_string    (GeglMatrix3 matrix,
                                         const gchar *string);
/**
 * gegl_matrix3_to_string:
 * @matrix: a #GeglMatrix
 *
 * Serialize a #GeglMatrix to a string.
 *
 * Returns a freshly allocated string representing that #GeglMatrix, the
 * returned string should be g_free()'d.
 *
 */
gchar *    gegl_matrix3_to_string       (GeglMatrix3 matrix);

/***
 */

G_END_DECLS

#endif
