/* This file is a test-case for GEGL
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
 * Copyright (C) 2013 Téo Mazars <teo.mazars@ensimag.fr>
 */

#include "gegl.h"

#include <string.h>
#include <math.h>

#define SUCCESS    0
#define FAILURE    -1

#define NUM_ROWS   8
#define NUM_COLS   16
#define NUM_PIXELS (NUM_ROWS * NUM_COLS)

#define SQR(x)     ((x)*(x))

typedef struct
{
  gint    wrong_pixels;
  gdouble max_diff;
  gdouble avg_diff_wrong;
  gdouble avg_diff_total;
} CompareResult;

static gboolean compare_values  (GeglNode            *comparison,
                                 const CompareResult *expected_result);
static gboolean test_comparison (const gfloat        *reference,
                                 const gfloat        *test_case,
                                 const CompareResult *expected_result);

const CompareResult
alpha_case =
  {
    2,           /* wrong pixels   */
    88.406395,   /* max diff       */
    44.703197,   /* avg_diff_wrong */
    0.698487     /* avg_diff_total */
  };

const CompareResult
rgba_r_case =
  {
    2,
    110.236877,
    55.606518,
    0.868851
  };

const CompareResult
rgba_g_case =
  {
    2,
    1.832906,
    1.013884,
    0.015842
  };

const CompareResult
rgba_b_case =
  {
    2,
    34.399933,
    17.536424,
    0.274007
  };

const CompareResult
ok_case =
  {
    0,
    0.0,
    0.0,
    0.0
  };


static gboolean
compare_values (GeglNode            *comparison,
                const CompareResult *expected_result)
{
  CompareResult actual_result;
  gint          test_result;
  /* Fetch all datas */
  gegl_node_get (comparison,
                 "max_diff",       &actual_result.max_diff,
                 "avg_diff_wrong", &actual_result.avg_diff_wrong,
                 "avg_diff_total", &actual_result.avg_diff_total,
                 "wrong_pixels",   &actual_result.wrong_pixels,
                 NULL);

  /* Check each value */
  test_result = SUCCESS;

  if (fabs (actual_result.max_diff - expected_result->max_diff)
      > GEGL_FLOAT_EPSILON)
    {
      g_printerr ("The max_diff property differs: %f instead of %f. ",
                  (gfloat) actual_result.max_diff,
                  (gfloat) expected_result->max_diff);
      test_result = FAILURE;
    }
  if (fabs (actual_result.avg_diff_wrong - expected_result->avg_diff_wrong)
      > GEGL_FLOAT_EPSILON)
    {
      g_printerr ("The avg_diff_wrong property differs: %f instead of %f. ",
                  (gfloat) actual_result.avg_diff_wrong,
                  (gfloat) expected_result->avg_diff_wrong);
      test_result = FAILURE;
    }
  if (fabs (actual_result.avg_diff_total - expected_result->avg_diff_total)
      > GEGL_FLOAT_EPSILON)
    {
      g_printerr ("The avg_diff_total property differs: %f instead of %f. ",
                  (gfloat) actual_result.avg_diff_total,
                  (gfloat) expected_result->avg_diff_total);
      test_result = FAILURE;
    }
  if (actual_result.wrong_pixels != expected_result->wrong_pixels)
    {
      g_printerr ("The wrong_pixels property differs: %d instead of %d. ",
                  actual_result.wrong_pixels,
                  expected_result->wrong_pixels);
      test_result = FAILURE;
    }

  return test_result;
}

static gboolean
test_comparison (const gfloat        *reference,
                 const gfloat        *test_case,
                 const CompareResult *expected_result)
{
  GeglBuffer    *src_ref_buffer;
  GeglBuffer    *src_aux_buffer;
  GeglRectangle  extent;
  const Babl    *input_format = babl_format ("R'G'B'A float");
  GeglNode      *graph, *source_ref, *source_aux, *comparison;
  gint           test_result;

  /* Set up all buffers */
  extent = *GEGL_RECTANGLE (0, 0, NUM_COLS, NUM_ROWS);

  src_ref_buffer = gegl_buffer_new (&extent, input_format);
  src_aux_buffer = gegl_buffer_new (&extent, input_format);

  gegl_buffer_set (src_ref_buffer, &extent, 0, input_format,
                   reference, GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_set (src_aux_buffer, &extent, 0, input_format,
                   test_case, GEGL_AUTO_ROWSTRIDE);

  /* Build the test graph */
  graph = gegl_node_new ();
  source_ref = gegl_node_new_child (graph,
                                    "operation", "gegl:buffer-source",
                                    "buffer", src_ref_buffer,
                                    NULL);
  source_aux = gegl_node_new_child (graph,
                                    "operation", "gegl:buffer-source",
                                    "buffer", src_aux_buffer,
                                    NULL);
  comparison = gegl_node_new_child (graph,
                                    "operation", "gegl:image-compare",
                                    NULL);

  gegl_node_link_many (source_ref, comparison, NULL);
  gegl_node_connect_to (source_aux, "output",  comparison, "aux");

  gegl_node_process (comparison);

  /* Compare with reference */
  test_result = compare_values (comparison, expected_result);

  /* We are done, clean and quit */
  g_object_unref (graph);
  g_object_unref (src_aux_buffer);
  g_object_unref (src_ref_buffer);

  return test_result;
}


int main (int   argc,
          char *argv[])
{
  gfloat reference    [NUM_PIXELS * 4];
  gfloat test_alpha   [NUM_PIXELS * 4];
  gfloat test_rgba_r  [NUM_PIXELS * 4];
  gfloat test_rgba_g  [NUM_PIXELS * 4];
  gfloat test_rgba_b  [NUM_PIXELS * 4];
  gfloat test_ok      [NUM_PIXELS * 4];
  gint   result;
  gint   i;

  gegl_init (&argc, &argv);

  /* Init an arbitrary image */
  for (i = 0; i < NUM_PIXELS; i++)
    {
      gfloat coef = i / (gfloat) (i + 1.0);
      reference [i * 4 + 0] = CLAMP (coef,             0.0, 1.0);
      reference [i * 4 + 1] = CLAMP (1.0 - coef,       0.0, 1.0);
      reference [i * 4 + 2] = CLAMP (SQR (1.0 - coef), 0.0, 1.0);
      reference [i * 4 + 3] = CLAMP (SQR (coef),       0.0, 1.0);
    }

  memcpy (test_alpha,  reference, NUM_PIXELS * 4 * sizeof (gfloat));
  memcpy (test_rgba_r, reference, NUM_PIXELS * 4 * sizeof (gfloat));
  memcpy (test_rgba_g, reference, NUM_PIXELS * 4 * sizeof (gfloat));
  memcpy (test_rgba_b, reference, NUM_PIXELS * 4 * sizeof (gfloat));
  memcpy (test_ok   ,  reference, NUM_PIXELS * 4 * sizeof (gfloat));

  /* Images are 8x16 RGBA, alters some pixels.
   * Try to trick image-compare's implementation
   * by setting the lowest difference first
   */
  test_rgba_r [4 * (2 * NUM_COLS + 9)  + 0] -= -0.01;
  test_rgba_r [4 * (7 * NUM_COLS + 14) + 0] = 0.1;

  test_rgba_g [4 * (1 * NUM_COLS + 9) + 1] += 0.01;
  test_rgba_g [4 * (2 * NUM_COLS + 4) + 1] = 0.1;

  test_rgba_b [4 * (4 * NUM_COLS + 0) + 2] += 0.01;
  test_rgba_b [4 * (5 * NUM_COLS + 3) + 2] = 0.3;

  test_alpha [4 * (3 * NUM_COLS + 6)  + 3] -= 0.01;
  test_alpha [4 * (7 * NUM_COLS + 12) + 3] = 0.1;

  /* Perform all 5 tests cases */
  result  = SUCCESS;

  if (test_comparison (reference, test_alpha, &alpha_case))
    {
      g_printerr ("-> Comparison failed for the alpha channel case\n");
      result = FAILURE;
    }
  if (test_comparison (reference, test_rgba_r, &rgba_r_case))
    {
      g_printerr ("-> Comparison failed for the red channel case\n");
      result = FAILURE;
    }
  if (test_comparison (reference, test_rgba_g, &rgba_g_case))
    {
      g_printerr ("-> Comparison failed for the green channel case\n");
      result = FAILURE;
    }
  if (test_comparison (reference, test_rgba_b, &rgba_b_case))
    {
      g_printerr ("-> Comparison failed for the blue channel case\n");
      result = FAILURE;
    }
  if (test_comparison (reference, test_ok, &ok_case))
    {
      g_printerr ("-> Comparison failed for the reference case\n");
      result = FAILURE;
    }

  gegl_exit ();

  return result;
}
