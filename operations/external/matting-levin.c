/* This file is an image processing operation for GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2010 Danny Robson <danny@blubinc.net>
 * Based off 2006 Anat Levin
 */

#include "config.h"
#include <glib/gi18n-lib.h>


#ifdef GEGL_CHANT_PROPERTIES
gegl_chant_int    (epsilon, _("Epsilon"),
                  -9, -1, -6,
                  _("Log of the error weighting"))
gegl_chant_int    (radius, _("Radius"),
                  1, 3, 1,
                  _("Radius of the processing window"))
gegl_chant_double (threshold, _("Threshold"),
                  0.0, 0.1, 0.02,
                  _("Alpha threshold for multilevel processing"))
gegl_chant_double (lambda, _("Lambda"),
                  0.0, 100.0, 100.0, _("Trimap influence factor"))
gegl_chant_int    (levels, _("Levels"),
                   0, 8, 4,
                   _("Number of downsampled levels to use"))
gegl_chant_int    (active_levels, _("Active Levels"),
                   0, 8, 2,
                   _("Number of levels to perform solving"))
#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "matting-levin.c"

#include "gegl-chant.h"
#include "gegl-debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* XXX: We have two options for the two common installation locations of
 * UMFPACK. Ideally this would be sorted out purely in autoconf; see
 * configure.ac for the issues.
 */
#if defined(HAVE_UMFPACK_H)
#include <umfpack.h>
#elif defined (HAVE_SUITESPARSE_UMFPACK_H)
#include <suitesparse/umfpack.h>
#endif

#include "matting-levin-cblas.h"


/* We don't use the babl_format_get_n_components function for these values,
 * as literal constants can be used for stack allocation of array sizes. They
 * are double checked in matting_process.
 */
#define COMPONENTS_AUX    2
#define COMPONENTS_INPUT  3
#define COMPONENTS_OUTPUT 1
#define COMPONENTS_COEFF  4

#define CONVOLVE_RADIUS   2
#define CONVOLVE_LEN     ((CONVOLVE_RADIUS * 2) + 1)


/* A simple structure holding a compressed column sparse matrix. Data fields
 * correspond directly to the expected format used by UMFPACK. This restricts
 * us to using square matrices.
 */
typedef struct
{
  guint    elems,
           columns,
           rows;
  glong   *col_idx,
          *row_idx;
  gdouble *values;
} sparse_t;


/* All channels use double precision. Despite it being overly precise, slower,
 * and larger; it's much more convenient:
 *   - Input R'G'B' needs to be converted into doubles later when calculating
 *     the matting laplacian, as the extra precision is actually useful here,
 *     and UMFPACK requires doubles.
 *   - AUX Y' is easier to use as a double when dealing with the matting
 *     laplacian which is already in doubles.
 */

static const gchar *FORMAT_AUX    = "Y'A double";
static const gchar *FORMAT_INPUT  = "R'G'B' double";
static const gchar *FORMAT_OUTPUT = "Y' double";

static const guint AUX_VALUE = 0;
static const guint AUX_ALPHA = 1;


/* Threshold below which we consider the Y channel to be undefined (or
 * masked). This is a binary specification, either fully masked or fully
 * defined.
 */
static const gdouble TRIMAP_ALPHA_THRESHOLD = 0.01;


/* The smallest dimension of a buffer which we allow during downsampling.
 * This must allow sufficient room for CONVOLVE_RADIUS convolutions to work
 * usefully.
 */
static const gint MIN_LEVEL_DIAMETER = 30;


/* Round upwards with performing `x / y' */
static guint
ceil_div (gint x, gint y)
{
  return (x + y - 1) / y;
}

/* Perform a floating point comparison, returning true if the values are
 * within the percentage tolerance specified in FLOAT_TOLERANCE. Note: this
 * is different to GEGL_FLOAT_EQUAL which specifies an absolute delta. This
 * won't work with very small values, however our approach can slower.
 */
static gboolean
float_cmp (gfloat a, gfloat b)
{
  static const gfloat FLOAT_TOLERANCE = 0.0001f;

  return (a - b) <= FLOAT_TOLERANCE * fabsf (a) ||
         (a - b) <= FLOAT_TOLERANCE * fabsf (b);
}

/* Return the offset for the integer coordinates (X, Y), in surface of
 * dimensions R, which has C channels. Does not take into account the channel
 * width, so should be used for indexing into properly typed arrays/pointers.
 *
 * Quite expensive without inlining (~15% runtime).
 */
static inline off_t
offset (gint                 x,
        gint                 y,
        const GeglRectangle *restrict roi,
        gint                 components)
{
  return (x + y * roi->width) * components;
}

/* Similar to `offset', inlining buys us a good speedup (though is less
 * frequently used than the general purpose `offset').
 */
static inline gboolean
trimap_masked (const gdouble       *restrict trimap,
               gint                 x,
               gint                 y,
               const GeglRectangle *restrict roi)
{
  gdouble value = trimap[offset (x, y, roi, COMPONENTS_AUX) + AUX_ALPHA];
  return  value < TRIMAP_ALPHA_THRESHOLD;
}


static const char*
matting_umf_error_to_string (guint err)
{

  switch (err)
    {
      case UMFPACK_OK:
          return "UMFPACK_OK";
      case UMFPACK_WARNING_singular_matrix:
          return "UMFPACK_WARNING_singular_matrix";
      case UMFPACK_WARNING_determinant_underflow:
          return "UMFPACK_WARNING_determinant_underflow";
      case UMFPACK_WARNING_determinant_overflow:
          return "UMFPACK_WARNING_determinant_overflow";
      case UMFPACK_ERROR_out_of_memory:
          return "UMFPACK_ERROR_out_of_memory";
      case UMFPACK_ERROR_invalid_Numeric_object:
          return "UMFPACK_ERROR_invalid_Numeric_object";
      case UMFPACK_ERROR_invalid_Symbolic_object:
          return "UMFPACK_ERROR_invalid_Symbolic_object";
      case UMFPACK_ERROR_argument_missing:
          return "UMFPACK_ERROR_argument_missing";
      case UMFPACK_ERROR_n_nonpositive:
          return "UMFPACK_ERROR_n_nonpositive";
      case UMFPACK_ERROR_invalid_matrix:
          return "UMFPACK_ERROR_invalid_matrix";
      case UMFPACK_ERROR_different_pattern:
          return "UMFPACK_ERROR_different_pattern";
      case UMFPACK_ERROR_invalid_system:
          return "UMFPACK_ERROR_invalid_system";
      case UMFPACK_ERROR_invalid_permutation:
          return "UMFPACK_ERROR_invalid_permutation";
      case UMFPACK_ERROR_internal_error:
          return "UMFPACK_ERROR_internal_error";
      case UMFPACK_ERROR_file_IO:
          return "UMFPACK_ERROR_file_IO";

      default:
          g_return_val_if_reached ("Unknown UMFPACK error");
    }
}


static void
matting_prepare (GeglOperation *operation)
{
  gegl_operation_set_format (operation, "input",  babl_format (FORMAT_INPUT));
  gegl_operation_set_format (operation, "aux",    babl_format (FORMAT_AUX));
  gegl_operation_set_format (operation, "output", babl_format (FORMAT_OUTPUT));
}


static GeglRectangle
matting_get_required_for_output (GeglOperation       *operation,
                                 const gchar         *input_pad,
                                 const GeglRectangle *roi)
{
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}


static GeglRectangle
matting_get_cached_region (GeglOperation * operation,
                           const GeglRectangle * roi)
{
  return *gegl_operation_source_get_bounding_box (operation, "input");
}


/* An element-wise subtraction on two 3x3 matrices. */
static void
matting_matrix3_matrix3_sub (gdouble _in1[3][3],
                             gdouble _in2[3][3],
                             gdouble _out[3][3])
{
  const gdouble *in1 = (gdouble*)_in1,
                *in2 = (gdouble*)_in2;
        gdouble *out = (gdouble*)_out;
  guint i;

  for (i = 0; i < 3 * 3; ++i)
    out[i] = in1[i] - in2[i];
}


/* An element-wise division on one 3x3 matrix, by one scalar */
static void
matting_matrix3_scalar_div (gdouble  _in[3][3],
                            gdouble        arg,
                            gdouble _out[3][3])
{
  const gdouble *in  = (gdouble*)_in;
        gdouble *out = (gdouble*)_out;
  guint i;

  for (i = 0; i < 3 * 3; ++i)
    out[i] = in[i] / arg;
}


/* Shortcut for a 3x3 matrix inversion. Assumes the matrix is stored in row
 * major format. Parameters 'in' and 'out' must not overlap. The output
 * matrix may be overwritten on error. Returns TRUE if an inversion exists.
 *
 * If the matrix consists of column vectors, A = (v_0, v_1, v_2)
 *
 *           1   / (v_1 x v_2)' \
 * inv(A) = ___  | (v_2 x v_0)' |
 *          det  \ (v_0 x v_1)' /
 *
 */
static gboolean
matting_matrix3_inverse (gdouble  in[3][3],
                         gdouble out[3][3])
{
  gdouble determinant;

  /* Save the column vector cross products straight into the output matrix */
  out[0][0] = in[1][1] * in[2][2] - in[1][2] * in[2][1];
  out[1][0] = in[1][2] * in[2][0] - in[1][0] * in[2][2];
  out[2][0] = in[1][0] * in[2][1] - in[1][1] * in[2][0];

  out[0][1] = in[2][1] * in[0][2] - in[2][2] * in[0][1];
  out[1][1] = in[2][2] * in[0][0] - in[2][0] * in[0][2];
  out[2][1] = in[2][0] * in[0][1] - in[2][1] * in[0][0];

  out[0][2] = in[0][1] * in[1][2] - in[0][2] * in[1][1];
  out[1][2] = in[0][2] * in[1][0] - in[0][0] * in[1][2];
  out[2][2] = in[0][0] * in[1][1] - in[0][1] * in[1][0];

  /* For a 3x3 matrix, det = v_0 . (v_1 x v_2)
   * We use the cross product that was previously stored into row zero of the
   * output matrix.
   */

  determinant = in[0][0] * out[0][0] +
                in[0][1] * out[1][0] +
                in[0][2] * out[2][0];
  if (determinant == 0)
    return FALSE;

  /* Scale the output by the determinant*/
  matting_matrix3_scalar_div (out, determinant, out);
  return TRUE;
}


/* Expanded form for 4x4 matrix inversion, derived from adjugate matrix and
 * division by determinant. Extensively uses values of 2x2 submatrix
 * determinants.
 *
 * Implementation based on David Eberly's article `The Laplace Expansion
 * Theorem: Computing the Determinants and Inverses of Matrices'.
 *
 * Input and output are in row-major format. Input and output must not
 * overlap. Output will not be altered on error. Returns TRUE if the inverse
 * exists.
 */
static gboolean
matting_matrix4_inverse (gdouble  in[4][4],
                         gdouble out[4][4])
{
  gdouble s[6], c[6];
  gdouble det;

  s[0] = in[0][0] * in[1][1] - in[1][0] * in[1][0];
  s[1] = in[0][0] * in[2][1] - in[2][0] * in[0][1];
  s[2] = in[0][0] * in[3][1] - in[3][0] * in[0][1];
  s[3] = in[1][0] * in[2][1] - in[2][0] * in[1][1];
  s[4] = in[1][0] * in[3][1] - in[3][0] * in[1][1];
  s[5] = in[2][0] * in[3][1] - in[3][0] * in[2][1];

  c[5] = in[2][2] * in[3][3] - in[3][2] * in[2][3];
  c[4] = in[1][2] * in[3][3] - in[3][2] * in[1][3];
  c[3] = in[1][2] * in[2][3] - in[2][2] * in[1][3];
  c[2] = in[0][2] * in[3][3] - in[3][2] * in[0][3];
  c[1] = in[0][2] * in[2][3] - in[2][2] * in[0][3];
  c[0] = in[0][2] * in[1][3] - in[1][2] * in[0][3];

  det = s[0] * c[5] -
        s[1] * c[4] +
        s[2] * c[3] +
        s[3] * c[2] -
        s[4] * c[1] +
        s[5] * c[0];

  /* The determinant can be extremely small in real cases (eg, 1e-15). So
   * existing checks like GEGL_FLOAT_IS_ZERO are no-where near precise enough
   * in the general case.
   * Most of the time we assume there is an inverse, so the lack of precision
   * in here isn't a dealbreaker, and we just compare against an actual zero
   * to avoid divide-by-zero errors.
   */
  if (det == 0.0)
    return FALSE;
  det = 1.0 / det;

  out[0][0] = (  in[1][1] * c[5] - in[2][1] * c[4] + in[3][1] * c[3]) * det;
  out[0][1] = ( -in[1][0] * c[5] + in[2][0] * c[4] - in[3][0] * c[3]) * det;
  out[0][2] = (  in[1][3] * s[5] - in[2][3] * s[4] + in[3][3] * s[3]) * det;
  out[0][3] = ( -in[1][2] * s[5] + in[2][2] * s[4] - in[3][2] * s[3]) * det;

  out[1][0] = ( -in[0][1] * c[5] + in[2][1] * c[2] - in[3][1] * c[1]) * det;
  out[1][1] = (  in[0][0] * c[5] - in[2][0] * c[2] + in[3][0] * c[1]) * det;
  out[1][2] = ( -in[0][3] * s[5] + in[2][3] * s[2] - in[3][3] * s[1]) * det;
  out[1][3] = (  in[0][2] * s[5] - in[2][2] * s[2] + in[3][2] * s[1]) * det;

  out[2][0] = (  in[0][1] * c[4] - in[1][1] * c[2] + in[3][1] * c[0]) * det;
  out[2][1] = ( -in[0][0] * c[4] + in[1][0] * c[2] - in[3][0] * c[0]) * det;
  out[2][2] = (  in[0][3] * s[4] - in[1][3] * s[2] + in[3][3] * s[0]) * det;
  out[2][3] = ( -in[0][2] * s[4] + in[1][2] * s[2] - in[3][2] * s[0]) * det;

  out[3][0] = ( -in[0][1] * c[3] + in[1][1] * c[1] - in[2][1] * c[0]) * det;
  out[3][1] = (  in[0][0] * c[3] - in[1][0] * c[1] + in[2][0] * c[0]) * det;
  out[3][2] = ( -in[0][3] * s[3] + in[1][3] * s[1] - in[2][3] * s[0]) * det;
  out[3][3] = (  in[0][2] * s[3] - in[1][2] * s[1] + in[2][2] * s[0]) * det;

  return TRUE;
}


/* Takes a vector and multiplies by its transpose to form a matrix in row
 * major format.
 */
static void
matting_vector3_self_product (gdouble in[3],
                              gdouble out[3][3])
{
  out[0][0] = in[0] * in[0];
  out[1][0] = in[0] * in[1];
  out[2][0] = in[0] * in[2];

  out[0][1] = in[1] * in[0];
  out[1][1] = in[1] * in[1];
  out[2][1] = in[1] * in[2];

  out[0][2] = in[2] * in[0];
  out[1][2] = in[2] * in[1];
  out[2][2] = in[2] * in[2];
}


/* Perform an erosion on the last component of `pixels'. If all neighbour
 * pixels are greater than low and lesser than 1 - high, keep the pixel
 * value, otherwise set it to NAN.
 *
 * Note, the condition is NOT low < pixel < high. Setting high to negative
 * expands the non-masking range.
 * XXX: This could probably be done with seperable passes, however there are
 * more immediate performance bottlenecks.
 */
static gdouble *
matting_erode_range (const gdouble       *restrict pixels,
                     const GeglRectangle *restrict region,
                     guint                components,
                     guint                radius,
                     gdouble              low,
                     gdouble              high)
{
  gdouble *new_pixels;
  guint    x, y, i, j,
           diameter = radius * 2 + 1;

  new_pixels = g_new0 (gdouble, region->width * region->height);
  for (y = radius; y < region->height - radius; ++y)
    {
      for (x = radius; x < region->width - radius; ++x)
        {
          gdouble home = pixels[offset (x, y,
                                        region,
                                        components) + components - 1],
                  value;
          if (home == 0.0)
            continue;

          if (home < 0.0 + low || home > 1.00 - high)
            goto masked;

          for (i = 0; i < diameter; ++i)
            {
              for (j = 0; j < diameter; ++j)
                {
                  value = pixels[offset (x - radius + i,
                                         y - radius + j,
                                         region,
                                         components) + components - 1];
                  if (value < low || value > 1.0 - high)
                    goto masked;
                }
            }

          new_pixels[offset (x, y, region, 1)] = home;
          continue;
  masked:
          new_pixels[offset (x, y, region, 1)] = NAN;
        }
    }
  return new_pixels;
}


/* Fill the borders of an image with the pixels from the first row/column
 * outside of `radius'. Does not expand the image. Operates in place.
 */
static void
matting_fill_borders (gdouble             *restrict image,
                      const GeglRectangle *restrict region,
                      const gint           components,
                      const gint           radius)
{
  gint x, y, c;

  g_return_if_fail (image  != NULL);
  g_return_if_fail (region != NULL);
  g_return_if_fail (components > 0);
  g_return_if_fail (radius     > 0);

  /* Radius shouldn't be greater than the region radius. */
  g_return_if_fail (radius < region->width  / 2);
  g_return_if_fail (radius < region->height / 2);

  /* Extend the edges of the convolution outwards */
  for (y = 0; y <= radius; ++y)
    {
      /* Copy the first convolved line into the top `radius' rows */
      memcpy (&image[offset (0,      y, region, components)],
              &image[offset (0, radius + 1, region, components)],
              region->width * sizeof (image[0]) * components);
      /* Copy the last convolved line into the last `radius' rows */
      memcpy (&image[offset (0, region->height -      y - 1, region, components)],
              &image[offset (0, region->height - radius - 2, region, components)],
              region->width * sizeof (image[0]) * components);
    }

  for (y = radius; y < region->height - radius; ++y)
    {
      for (x = 0; x <= radius; ++x)
        {
          for (c = 0; c < components; ++c)
            {
              image[offset (x, y, region, components) + c] =
                  image[offset (radius + 1, y, region, components) + c];
              image[offset (region->width - x - 1, y, region, components) + c] =
                  image[offset (region->width - radius - 2, y, region, components) + c];
            }
        }
    }
}


/* Calculate the coefficients needed to upsample a previously computed output
 * alpha map. Returns a surface of 4*doubles which correspond to:
 *    red * out[0] + green * out[1] + blue * out[2] + out[3]
 */
static gdouble *
matting_get_linear_coefficients (const gdouble       *restrict image,
                                 const gdouble       *restrict alpha,
                                 const GeglRectangle *restrict rect,
                                 const gdouble        epsilon,
                                 const gint           radius)
{
  gint     diameter     = radius * 2 + 1,
           window_elems = diameter * diameter,
           image_elems  = rect->width * rect->height;
  gdouble *coeffs       = g_new0 (gdouble, image_elems * (COMPONENTS_INPUT + 1));
  gint     x, y, i, j;

  gdouble window  [window_elems + COMPONENTS_INPUT][COMPONENTS_INPUT + 1],
          winprod [COMPONENTS_INPUT + 1][COMPONENTS_INPUT + 1],
          inverse [COMPONENTS_INPUT + 1][COMPONENTS_INPUT + 1],
          invprod [COMPONENTS_INPUT + 1][window_elems + COMPONENTS_INPUT],
          alphmat [window_elems + COMPONENTS_INPUT][1];

  g_return_val_if_fail (image, NULL);
  g_return_val_if_fail (alpha, NULL);
  g_return_val_if_fail (rect,  NULL);

  g_return_val_if_fail (epsilon != 0.0, NULL);
  g_return_val_if_fail (radius   > 0,   NULL);

  g_return_val_if_fail (COMPONENTS_INPUT + 1 == COMPONENTS_COEFF, NULL);

  /* Zero out the main window matrix, and pre-set the lower window identity
   * matrix, ones, and zeroes.
   */
  memset (window,  0, sizeof (window));
  memset (alphmat, 0, sizeof (alphmat));
  for (i = 0; i < COMPONENTS_INPUT; ++i)
    window[window_elems + i][i] = sqrtf (epsilon);
  for (i = 0; i < window_elems; ++i)
    window[i][COMPONENTS_INPUT] = 1.0;

  /* Calculate window's coefficients */
  for (x = radius; x < rect->width - radius; ++x)
    {
      for (y = radius; y < rect->height - radius; ++y)
        {
          /*          / I_r, I_g, I_b, 1 \
           *          | ...  ...  ...  1 |
           * window = | eps   0    0   0 |
           *          |  0   eps   0   0 |
           *          \  0    0   eps  0 /
           */
          for (j = 0; j < diameter; ++j)
            for (i = 0; i < diameter; ++i)
              {
                guint image_offset  =  x - radius + i;
                      image_offset += (y - radius + j) * rect->width;
                      image_offset *= COMPONENTS_INPUT;

                window[i + j * diameter][0] = image[image_offset + 0];
                window[i + j * diameter][1] = image[image_offset + 1];
                window[i + j * diameter][2] = image[image_offset + 2];
              }

          /* window' x window */
          cblas_dgemm (CblasRowMajor, CblasTrans, CblasNoTrans,
                       COMPONENTS_INPUT + 1,
                       COMPONENTS_INPUT + 1,
                       window_elems + COMPONENTS_INPUT, 1.0,
                       (gdouble *)window, COMPONENTS_INPUT + 1,
                       (gdouble *)window, COMPONENTS_INPUT + 1,
                       0.0, (gdouble *)winprod, COMPONENTS_INPUT + 1);

          /* inv ($_) */
          matting_matrix4_inverse (winprod, inverse);

          /* $_ x window' */
          cblas_dgemm (CblasRowMajor, CblasNoTrans, CblasTrans,
                       COMPONENTS_INPUT + 1,
                       window_elems + COMPONENTS_INPUT,
                       COMPONENTS_INPUT + 1, 1.0,
                       (gdouble *)inverse, COMPONENTS_INPUT + 1,
                       (gdouble *)window,  COMPONENTS_INPUT + 1,
                       0.0, (gdouble*)invprod, window_elems + COMPONENTS_INPUT);

          /* alphmat = | a[x,y], .., a[x+d,y+d], 0, 0, 0, 0 | */
          for (j = 0; j < diameter; ++j)
            {
              for (i = 0; i < diameter; ++i)
                {
                  alphmat[i + j * diameter][0] = alpha[offset (x - radius + i,
                                                               y - radius + j,
                                                               rect, 1)];
                }
            }

          /* $_ x alphmat = | w, x, y, z |  */
          cblas_dgemm (CblasRowMajor, CblasNoTrans, CblasNoTrans,
                       COMPONENTS_INPUT + 1, 1,
                       window_elems + COMPONENTS_INPUT, 1.0,
                       (gdouble *)invprod, window_elems + COMPONENTS_INPUT,
                       (gdouble *)alphmat, 1,
                       0.0, coeffs + offset (x, y, rect, COMPONENTS_INPUT + 1), 1);
        }
    }

  matting_fill_borders (coeffs, rect, COMPONENTS_COEFF, radius);
  return coeffs;
}


/*
 * Convolves with a seperable 5 element kernel. Modifies the input data in
 * place.
 */
static void
matting_convolve5 (gdouble             *restrict pixels,
                   const GeglRectangle *restrict region,
                   guint                components,
                   const gdouble        kernel[CONVOLVE_LEN])
{
  gint     x, y, i;
  guint    c;
  gdouble *temp = g_new0 (gdouble, region->width * region->height * components);

  g_return_if_fail (CONVOLVE_LEN % 2 == 1);
  /* Horizontal convolution */
  for (y = 0; y < region->height; ++y)
    {
      for (x = CONVOLVE_RADIUS; x < region->width - CONVOLVE_RADIUS; ++x)
        {
          for (i = 0; i < CONVOLVE_LEN; ++i)
            {
              for (c = 0; c < components; ++c)
                {
                  temp  [offset (                      x, y, region, components) + c] +=
                  pixels[offset (x + i - CONVOLVE_RADIUS, y, region, components) + c] * kernel[i];
                }
            }
        }
    }

  /* Vertical convolution */
  memset (pixels, 0, (sizeof (pixels[0]) *
                      region->width      *
                      region->height     *
                      components));

  for (y = CONVOLVE_RADIUS; y < region->height - CONVOLVE_RADIUS; ++y)
    {
      for (x = 0; x < region->width; ++x)
        {
          for (i = 0; i < CONVOLVE_LEN; ++i)
            {
              for (c = 0; c < components; ++c)
                {
                  pixels[offset (x, y     - CONVOLVE_RADIUS, region, components) + c] +=
                  temp  [offset (x, y + i - CONVOLVE_RADIUS, region, components) + c] * kernel[i];
                }
            }
        }
    }

  g_free (temp);

  matting_fill_borders (pixels, region, components, CONVOLVE_RADIUS + 1);
}


static gdouble *
matting_downsample (gdouble             *restrict pixels,
                    const GeglRectangle *restrict input,
                    GeglRectangle       *restrict output,
                    guint                components)
{
  /* Downsamples a buffer by a factor of two, and performs a gaussian blur.
   * Returns the output size via the provided pointer; this is not respected as
   * an input parameter.
   */
  static const gdouble DOWNSAMPLE_KERNEL[] =
    { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

  gint     x, y;
  guint    c;
  gdouble *down,
          *copy;

  g_return_val_if_fail (input->x == 0 && input->y == 0, NULL);
  output->x      = input->x;
  output->y      = input->y;
  output->width  = ceil_div (input->width,  2);
  output->height = ceil_div (input->height, 2);

  /* convolve a copy of the pixels */
  copy = g_new (gdouble, input->width  * input->height  * components);
  memcpy (copy, pixels, sizeof (pixels[0]) *
                        input->width       *
                        input->height      *
                        components);
  matting_convolve5 (copy, input, components, DOWNSAMPLE_KERNEL);

  /* downscale the copy into a new buffer */
  down = g_new (gdouble, output->width * output->height * components);
  for (x = 0; x < input->width; x += 2)
    {
      for (y = 0; y < input->height; y += 2)
        {
          guint down_offset = (offset (x / 2 , y / 2, output, components)),
                copy_offset = (offset (x     ,     y,  input, components));

          for (c = 0; c < components; ++c)
            down[down_offset + c] = copy[copy_offset + c];
        }
    }

  g_free (copy);
  return down;
}


static gdouble *
matting_upsample (const gdouble       *restrict pixels,
                  const GeglRectangle *restrict input,
                  const GeglRectangle *restrict output,
                  guint                         components)
{
  /* Upsample to the size given in output, which must equate to a factor of ~2.
   * Copies in input pixels into the corresponding output locations, leaving
   * the gaps black. Then performs a gaussian blur with a double weighted
   * kernel.
   */
  static const gdouble  UPSAMPLE_KERNEL[] =
    { 0.125, 0.5, 0.75, 0.5, 0.125 };

  gint     x_start, x_end,
           y_start, y_end;
  gint     x, y;
  guint    c;
  gdouble *newpix = NULL;

  g_return_val_if_fail (pixels, NULL);
  g_return_val_if_fail (input,  NULL);
  g_return_val_if_fail (output, NULL);
  g_return_val_if_fail (input->width < output->width &&
                        input->height < output->height, NULL);
  g_return_val_if_fail (abs (output->width  - 2 * input->width ) <= 1, NULL);
  g_return_val_if_fail (abs (output->height - 2 * input->height) <= 1, NULL);

  x_start = 1;
  y_start = 1;
  x_end   = output->width  - output->width  % 2;
  y_end   = output->height - output->height % 2;

  newpix  = g_new0 (gdouble, output->width * output->height * components);

  for (y = y_start; y < output->height; y += 2)
    {
      for (x = x_start; x < output->width; x += 2)
        {
          guint newoff = (x     +  y      * output->width) * components,
                oldoff = (x / 2 + (y / 2) * input->width ) * components;

          for (c = 0; c < components; ++c)
            newpix[newoff + c] = pixels[oldoff + c];
        }
    }

  matting_convolve5 (newpix, output, components, UPSAMPLE_KERNEL);
  return newpix;
}


/* Upsample a previously computed alpha mat, using linear coefficients taken
 * from the source image. Resizes from small_rect to large_rect, and assumes
 * the factor is 2x +/- 1pixel.
 */
static gdouble *
matting_upsample_alpha (const gdouble       *restrict small_pixels,
                        const gdouble       *restrict large_pixels,
                        const gdouble       *restrict small_alpha,
                        const GeglRectangle *restrict small_rect,
                        const GeglRectangle *restrict large_rect,
                        gdouble              epsilon,
                        guint                radius)
{
  gdouble *small_coeff = NULL,
          *large_coeff = NULL,
          *new_alpha = NULL;
  gint     i;

  small_coeff = matting_get_linear_coefficients (small_pixels, small_alpha,
                                                 small_rect, epsilon,
                                                 radius);
  if (!small_coeff)
    goto cleanup;

  large_coeff = matting_upsample (small_coeff, small_rect, large_rect, COMPONENTS_COEFF);
  if (!large_coeff)
    goto cleanup;

  new_alpha = g_new (gdouble, large_rect->width * large_rect->height);
  for (i = 0; i < large_rect->width * large_rect->height; ++i)
    {
      new_alpha[i]  = large_coeff[i * COMPONENTS_COEFF + 3];
      new_alpha[i] += large_coeff[i * COMPONENTS_COEFF + 0] * large_pixels[i * COMPONENTS_INPUT + 0];
      new_alpha[i] += large_coeff[i * COMPONENTS_COEFF + 1] * large_pixels[i * COMPONENTS_INPUT + 1];
      new_alpha[i] += large_coeff[i * COMPONENTS_COEFF + 2] * large_pixels[i * COMPONENTS_INPUT + 2];
    }

cleanup:
  g_free (small_coeff);
  g_free (large_coeff);
  return new_alpha;
}


static sparse_t *
matting_sparse_new (guint cols, guint rows, guint elems)
{
  sparse_t *s = g_new (sparse_t, 1);
  s->columns  = cols;
  s->rows     = rows;
  s->col_idx  = g_new  (UF_long, cols + 1);
  s->row_idx  = g_new  (UF_long, elems);
  s->values   = g_new0 (gdouble, elems);

  return s;
}


static void
matting_sparse_free (sparse_t *s)
{
  if (!s)
      return;

  g_free (s->row_idx);
  g_free (s->col_idx);
  g_free (s->values);
  g_free (s);
}


static guint
matting_sparse_elems (const sparse_t *s)
{
  return s->col_idx[s->columns];
}


/* Debugging function which ensures the sparse matrix fields are consistent
 * with what UMFPACK, and the matting algorithm, would expect.
 *
 * Returns FALSE, using glib debugging routines, if there is an error. Else,
 * returns TRUE.
 */
static gboolean
matting_verify (const sparse_t *s)
{
  guint    i, j;
  gboolean rows[s->rows];

  /* Must be a square matrix */
  g_return_val_if_fail (s->columns == s->rows, FALSE);
  g_return_val_if_fail (s->col_idx[0] == 0,    FALSE);

  for (i = 1; i < s->columns; ++i)
    {
      /* Strictly ascending column indices */
      guint col = s->col_idx[i];
      g_return_val_if_fail (s->col_idx[i - 1] <= col, FALSE);

      for (j = s->col_idx[i] + 1; j < s->col_idx[i + 1]; ++j)
        {
          /* Strictly ascending row indices, within a column */
          guint row = s->row_idx[j];
          g_return_val_if_fail (s->row_idx[j - 1] < row, FALSE);
          g_return_val_if_fail (row < s->rows,           FALSE);
        }
    }

  /* We expect to have entries for each column in the matrix. Note: this is
   * not a requirement of the UMFPACK format; rather, something we expect of
   * the matrix from the matting algorithm.
   */
  for (i = 0; i < s->rows; ++i)
    rows [i] = FALSE;
  for (i = 0; i < matting_sparse_elems (s); ++i)
    {
      guint row = s->row_idx[i];
      g_return_val_if_fail (row < s->rows, FALSE);
      rows[row] = TRUE;
    }
  for (i = 0; i < s->rows; ++i)
    g_return_val_if_fail (rows[i], FALSE);

  return TRUE;
}


/* Calculate the matting laplacian for an image, given a user trimap.
 * We accumulate entries in a sparse banded matrix, for a radius around each
 * pixel in the image.
 *
 * We construct a triplet form of the matrix initially, then transform to
 * compressed column. This is much simpler than directly constructing the
 * compressed column form, and does not appear to cause a performance
 * bottleneck (though does consume additional memory).
 */
static sparse_t*
matting_get_laplacian (const gdouble       *restrict image,
                       const gdouble       *restrict trimap,
                       const GeglRectangle *restrict roi,
                       const gint           radius,
                       const gdouble        epsilon,
                       const gdouble        lambda)
{
  gint      diameter     = radius * 2 + 1,
            window_elems = diameter * diameter,
            image_elems  = roi->width * roi->height,
            i, j, k, x, y,
            status;
  UF_long  *trip_col,
           *trip_row;
  glong     trip_nz = 0,
            trip_cursor = 0,
            trip_masked = 0;
  gdouble  *trip_val;
  sparse_t *laplacian;

  gdouble       mean[COMPONENTS_INPUT],
         mean_matrix[COMPONENTS_INPUT][COMPONENTS_INPUT],
          covariance[COMPONENTS_INPUT][COMPONENTS_INPUT],
             inverse[COMPONENTS_INPUT][COMPONENTS_INPUT],
              window[COMPONENTS_INPUT][window_elems],
             winxinv[COMPONENTS_INPUT][window_elems],
                  values[window_elems][window_elems];

  g_return_val_if_fail (radius > 0, NULL);
  g_return_val_if_fail (COMPONENTS_INPUT == 3, NULL);

  for (j = radius; j < roi->height - radius; ++j)
    {
      for (i = radius; i < roi->width - radius; ++i)
        {
          if (trimap_masked (trimap, i, j, roi))
            trip_masked++;
        }
    }

  trip_nz   = trip_masked * window_elems * window_elems;
  trip_nz  += image_elems; // Sparse diagonal and row summing at conclusion

  trip_col  = g_new  (UF_long, trip_nz);
  trip_row  = g_new  (UF_long, trip_nz);
  trip_val  = g_new0 (gdouble, trip_nz);

  /* Compute the contribution of each pixel in the image to the laplacian */
  for (i = radius; i < roi->width - radius; ++i)
    {
      for (j = radius; j < roi->height - radius; ++j)
        {
          /* Skip if the pixel is valid in the the trimap */
          if (!trimap_masked (trimap, i, j, roi))
            continue;
          trip_masked--;
          g_return_val_if_fail (trip_masked >= 0, FALSE);

          /* Calculate window's component means, and their vector product
           * (which we will use later to calculate the covariance matrix).
           * Store the values into the window matrix as we go.
           */
          mean[0] = mean[1] = mean[2] = 0.0;
          k = 0;
          for (y = j - radius; y <= j + radius; ++y)
            for (x = i - radius; x <= i + radius; ++x)
              {
                mean[0] += window[0][k] = image[(x + y * roi->width) * COMPONENTS_INPUT + 0];
                mean[1] += window[1][k] = image[(x + y * roi->width) * COMPONENTS_INPUT + 1];
                mean[2] += window[2][k] = image[(x + y * roi->width) * COMPONENTS_INPUT + 2];
                ++k;
              }

          mean[0] /= window_elems;
          mean[1] /= window_elems;
          mean[2] /= window_elems;

          matting_vector3_self_product (mean, mean_matrix);

          /*
           * Calculate inverse covariance matrix.
           */

          /* Multiply the 'component x window' matrix with its transpose to
           * form a 3x3 matrix which is the first component of the covariance
           * matrix.
           */
          cblas_dgemm (CblasRowMajor, CblasNoTrans, CblasTrans,
                       COMPONENTS_INPUT, COMPONENTS_INPUT, window_elems,
                       1.0 / window_elems,
                       (gdouble *)window, window_elems,
                       (gdouble *)window, window_elems,
                       0.0,  (gdouble *)covariance, COMPONENTS_INPUT);

          /* Subtract the mean to create the covariance matrix, then add the
           * epsilon term and invert.
           */
          matting_matrix3_matrix3_sub (covariance, mean_matrix, covariance);
          covariance[0][0] += epsilon / window_elems;
          covariance[1][1] += epsilon / window_elems;
          covariance[2][2] += epsilon / window_elems;
          matting_matrix3_inverse     (covariance, inverse);

          /* Subtract each component's mean from the pixels */
          for (k = 0; k < window_elems; ++k)
            {
              window[0][k] -= mean[0];
              window[1][k] -= mean[1];
              window[2][k] -= mean[2];
            }

          /* Calculate the values for the matting matrix */
          cblas_dgemm (CblasRowMajor, CblasNoTrans, CblasNoTrans,
                       COMPONENTS_INPUT, window_elems, COMPONENTS_INPUT,
                       1.0,
                       (gdouble *)inverse, COMPONENTS_INPUT,
                       (gdouble *) window, window_elems,
                       0.0, (gdouble *)winxinv, window_elems);

          cblas_dgemm (CblasRowMajor, CblasTrans, CblasNoTrans,
                       window_elems, window_elems, COMPONENTS_INPUT,
                       1.0,
                       (gdouble *) window, window_elems,
                       (gdouble *)winxinv, window_elems,
                       0.0, (gdouble *)values, window_elems);

          /* Store the values and coordinates */
          for (y = 0; y < window_elems; ++y)
            for (x = 0; x < window_elems; ++x)
              {
                UF_long yx = y % diameter,
                        yy = y / diameter,
                        xx = x % diameter,
                        xy = x / diameter;

                g_return_val_if_fail (trip_cursor < trip_nz, FALSE);
                trip_col[trip_cursor] = (i - radius + yx) + (j - radius + yy) * roi->width,
                trip_row[trip_cursor] = (i - radius + xx) + (j - radius + xy) * roi->width,
                trip_val[trip_cursor] = (1.0 + values[y][x]) / window_elems;
                ++trip_cursor;
              }
        }
    }

  {
    gdouble row_sum[image_elems];

    /* Calculate the sum of all the elements in each row */
    for (i = 0; i < image_elems; ++i)
      row_sum[i] = 0.0;
    for (i = 0; i < trip_cursor; ++i)
      row_sum[trip_row[i]]  += trip_val[i];

    /* Negate each entry of the matrix. This partially implements a
     * subtraction from the diagonal matrix:
     *    [lambda + sum, lambda + sum, ..., lambda + sum]
     */
    for (i = 0; i < trip_cursor; ++i)
      trip_val[i] = -trip_val[i];

    /* Set the diagonal such that the sum of the row equals `lambda' if the
     * trimap entry is valid
     */
    for (i = 0; i < image_elems; ++i)
      {
        trip_col[trip_cursor] = i;
        trip_row[trip_cursor] = i;
        trip_val[trip_cursor] = row_sum[i];

        if (!trimap_masked (trimap, i, 0, roi))
          trip_val[trip_cursor] += lambda;
        trip_cursor++;
      }

    /* Double check that each row equals either 0.0 or lambda */
    for (i = 0; i < image_elems; ++i)
      row_sum[i] = 0.0;
    for (i = 0; i < trip_cursor; ++i)
      row_sum[trip_row[i]]  += trip_val[i];
    for (i = 0; i < image_elems; ++i)
      {
        g_warn_if_fail (float_cmp (row_sum [i], 0.0) ||
                        float_cmp (row_sum [i], lambda));
      }
  }

  g_warn_if_fail (trip_cursor == trip_nz);

  /* Convert to the compressed column format expected by UMFPACK */
  laplacian = matting_sparse_new (image_elems, image_elems, trip_cursor);
  status    = umfpack_dl_triplet_to_col (laplacian->rows,
                                         laplacian->columns,
                                         trip_cursor,
                                         trip_row, trip_col, trip_val,
                                         laplacian->col_idx,
                                         laplacian->row_idx,
                                         laplacian->values,
                                         NULL);

  g_free (trip_col);
  g_free (trip_row);
  g_free (trip_val);

  g_return_val_if_fail (status == UMFPACK_OK, FALSE);
  return laplacian;
}


static gboolean
matting_solve_laplacian (gdouble             *restrict trimap,
                         sparse_t            *restrict laplacian,
                         gdouble             *restrict solution,
                         const GeglRectangle *restrict roi,
                         gdouble              lambda)
{
  void    *symbolic = NULL,
          *numeric  = NULL;
  gint     status;
  guint    image_elems, i;
  gboolean success = FALSE;
  gdouble  umfcontrol[UMFPACK_CONTROL],
           umfinfo[UMFPACK_INFO];

  g_return_val_if_fail (trimap,    FALSE);
  g_return_val_if_fail (laplacian, FALSE);
  g_return_val_if_fail (solution,  FALSE);

  g_return_val_if_fail (roi,       FALSE);
  g_return_val_if_fail (!gegl_rectangle_is_empty (roi), FALSE);
  image_elems = roi->width * roi->height;

  g_return_val_if_fail (laplacian->columns == image_elems, FALSE);
  g_return_val_if_fail (laplacian->rows    == image_elems, FALSE);

  matting_verify (laplacian);

  umfpack_di_defaults (umfcontrol);
  /* Pre-process the matrix */
  if ((status = umfpack_dl_symbolic (laplacian->rows,
                                     laplacian->columns,
                                     laplacian->col_idx,
                                     laplacian->row_idx,
                                     laplacian->values,
                                     &symbolic,
                                     umfcontrol, umfinfo)) < 0)
    {
      symbolic = NULL;
      goto cleanup;
    }

  if ((status = umfpack_dl_numeric (laplacian->col_idx,
                                    laplacian->row_idx,
                                    laplacian->values,
                                    symbolic, &numeric,
                                    umfcontrol, umfinfo)) < 0)
    {
      numeric = NULL;
      goto cleanup;
    }

  /* Solve and exit */
  {
    gdouble *residual = g_new (gdouble, image_elems);
    for (i = 0; i < image_elems; ++i) 
      {
        if (trimap_masked (trimap, i, 0, roi))
          residual[i] = 0;
        else
          residual[i] = lambda * trimap[i * COMPONENTS_AUX + AUX_VALUE];
      }

    status = umfpack_dl_solve (UMFPACK_A,
                               laplacian->col_idx,
                               laplacian->row_idx,
                               laplacian->values,
                               solution,
                               residual,
                               numeric,
                               umfcontrol, umfinfo);

    /* Positive numbers are warnings. We don't care if the matrix is
     * singular, as the computed result is still usable, so just check for
     * errors.
     */
    g_free (residual);
    if (status < 0)
      goto cleanup;
  }

  /* Courtesy clamping of the solution to normal alpha range */
  for (i = 0; i < image_elems; ++i)
    solution[i] = CLAMP (solution[i], 0.0, 1.0);

  success = TRUE;
cleanup:
  /* Singular matrices appear to work correctly, provided that we clamp the
   * results (which needs to be done regardless). I'm not sure if this is a
   * result of an incorrect implementation of the algorithm, or if it's
   * inherent to the design; either way it seems to work.
   */
  if (status != UMFPACK_OK && status != UMFPACK_WARNING_singular_matrix)
    g_warning ("%s", matting_umf_error_to_string (status));

  if (numeric)
    umfpack_dl_free_numeric  (&numeric);
  if (symbolic)
    umfpack_dl_free_symbolic (&symbolic);

  return success;
}


/* Recursively downsample, solve, then upsample the matting laplacian.
 * Perform up to `levels' recursions (provided the image remains large
 * enough), with up to `active_levels' number of full laplacian solves (not
 * just extrapolation).
 */
static gdouble *
matting_solve_level (gdouble             *restrict pixels,
                     gdouble             *restrict trimap,
                     const GeglRectangle *restrict region,
                     guint                active_levels,
                     guint                levels,
                     guint                radius,
                     gdouble              epsilon,
                     gdouble              lambda,
                     gdouble              threshold)
{
  gint     i;
  gdouble *new_alpha    = NULL,
          *eroded_alpha = NULL;

  if (region->width  < MIN_LEVEL_DIAMETER ||
      region->height < MIN_LEVEL_DIAMETER)
    {
      GEGL_NOTE (GEGL_DEBUG_PROCESS,
                 "skipping subdivision with level %dx%d\n",
                region->width, region->height);
      levels = 0;
    }

  if (levels > 0)
    {
      GeglRectangle  small_region;
      gdouble       *small_pixels,
                    *small_trimap;
      gdouble       *small_alpha;

      /* Downsample, solve, then upsample again */
      small_pixels = matting_downsample (pixels, region, &small_region,
                                         COMPONENTS_INPUT);
      small_trimap = matting_downsample (trimap, region, &small_region,
                                         COMPONENTS_AUX);
      for (i = 0; i < small_region.width  *
                      small_region.height *
                      COMPONENTS_AUX; ++i)
        {
          small_trimap[i] = roundf (small_trimap[i]);
        }

      small_alpha = matting_solve_level (small_pixels, small_trimap,
                                         &small_region, active_levels,
                                         levels - 1, radius, epsilon,
                                         lambda, threshold);

      new_alpha = matting_upsample_alpha (small_pixels, pixels, small_alpha,
                                          &small_region, region, epsilon,
                                          radius);

      /* Erode the result:
       * If the trimap alpha has not been set high (ie, valid), update the
       * trimap value with our computed result.
       * If it was eroded, then set the trimap pixel as valid by setting
       * alpha high.
       * Set all trimap values as either high or low.
       */
      eroded_alpha = matting_erode_range (new_alpha, region, 1, radius,
                                          threshold, threshold);

      g_free (small_pixels);
      g_free (small_trimap);
      g_free (small_alpha);

      for (i = 0; i < region->width * region->height; ++i)
        {
          if (trimap[i * COMPONENTS_AUX + AUX_ALPHA] < TRIMAP_ALPHA_THRESHOLD)
            trimap[i * COMPONENTS_AUX + AUX_VALUE] = new_alpha[i];

          if (isnan (eroded_alpha[i]))
            trimap[i * COMPONENTS_AUX + AUX_ALPHA] = 1.0;

          trimap[i * COMPONENTS_AUX + AUX_VALUE] *= roundf (trimap[i * COMPONENTS_AUX + AUX_VALUE]) *
                                                    trimap[i * COMPONENTS_AUX + AUX_ALPHA];
        }
      g_free (eroded_alpha);
    }

  /* Ordinary solution of the matting laplacian */
  if (active_levels >= levels || levels == 0)
    {
      sparse_t *laplacian;
      g_free (new_alpha);

      if (!(laplacian = matting_get_laplacian (pixels, trimap, region,
              radius, epsilon, lambda)))
        {
          g_warning ("unable to construct laplacian matrix");
          return NULL;
        }

      new_alpha = g_new (gdouble, region->width * region->height);
      matting_solve_laplacian (trimap, laplacian, new_alpha, region, lambda);
      matting_sparse_free (laplacian);
    }

  g_return_val_if_fail (new_alpha != NULL, NULL);
  return new_alpha;
}


/* Simple wrapper around matting_solve_level, which extracts the relevant
 * pixel data and writes the solution to output.
 */
static gboolean
matting_process (GeglOperation       *operation,
                 GeglBuffer          *input_buf,
                 GeglBuffer          *aux_buf,
                 GeglBuffer          *output_buf,
                 const GeglRectangle *result,
                 gint                 level)
{
  const GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gdouble          *input   = NULL,
                   *trimap  = NULL;
  gdouble          *output  = NULL;
  gboolean          success = FALSE;

  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_INPUT )) == COMPONENTS_INPUT,  FALSE);
  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_AUX   )) == COMPONENTS_AUX,    FALSE);
  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_OUTPUT)) == COMPONENTS_OUTPUT, FALSE);

  g_return_val_if_fail (operation,  FALSE);
  g_return_val_if_fail (input_buf,  FALSE);
  g_return_val_if_fail (aux_buf,    FALSE);
  g_return_val_if_fail (output_buf, FALSE);
  g_return_val_if_fail (result,     FALSE);

  input  = g_new (gdouble, result->width * result->height * COMPONENTS_INPUT);
  trimap = g_new (gdouble, result->width * result->height * COMPONENTS_AUX);

  gegl_buffer_get (input_buf, result, 1.0, babl_format (FORMAT_INPUT), input, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (  aux_buf, result, 1.0, babl_format (FORMAT_AUX),  trimap, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  output = matting_solve_level (input, trimap, result,
                                MIN (o->active_levels, o->levels), o->levels,
                                o->radius, powf (10, o->epsilon), o->lambda,
                                o->threshold);
  gegl_buffer_set (output_buf, result, 0, babl_format (FORMAT_OUTPUT), output,
                   GEGL_AUTO_ROWSTRIDE);

  success = TRUE;

  g_free (input);
  g_free (trimap);
  g_free (output);

  return success;
}


static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  composer_class->process = matting_process;

  operation_class->prepare                 = matting_prepare;
  operation_class->get_required_for_output = matting_get_required_for_output;
  operation_class->get_cached_region       = matting_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
  "name"        , "gegl:matting-levin",
  "categories"  , "misc",
  "description" ,
        _("Given a sparse user supplied tri-map and an input image, create a "
          "foreground alpha mat. Set white as selected, black as unselected, "
          "for the tri-map."),
        NULL);
}


#endif
