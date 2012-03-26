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
 * Copyright 2011 Jan Rüegg <rggjan@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <math.h>

#ifdef GEGL_CHANT_PROPERTIES

gegl_chant_int (iterations, _("Iterations"), 1, G_MAXINT, 10,
                _("Number of iterations"))

#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE "matting-global.c"

#include "gegl-chant.h"
#include "gegl-debug.h"

#define max(a,b) \
  ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a > _b ? _a : _b; })

#define min(a,b) \
  ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b); \
  _a > _b ? _b : _a; })

#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)       __builtin_expect((x),0)

#define ASSERT(condition) \
  if(unlikely(!(condition))) { \
  printf("Error at line %i\n", __LINE__); \
  exit(1);\
  }

// Shortcut for doing things in all three channels
#define COLOR(expr) {int c; for (c = 0; c < 3; c++) { expr; }}

// Save all important memories to output image buffer to save memory
#define FG_DISTANCE(output, index) (output[index*4+0])
#define BG_DISTANCE(output, index) (output[index*4+1])
#define FG_INDEX(output, index) (*((int*)(&output[index*4+2])))
#define BG_INDEX(output, index) (*((int*)(&output[index*4+3])))

/* We don't use the babl_format_get_n_components function for these values,
 * as literal constants can be used for stack allocation of array sizes. They
 * are double checked in matting_process.
 */
#define COMPONENTS_AUX    1
#define COMPONENTS_INPUT  3
#define COMPONENTS_OUTPUT 4

static const gchar *FORMAT_AUX    = "Y u8";
static const gchar *FORMAT_INPUT  = "R'G'B' float";
static const gchar *FORMAT_OUTPUT = "R'G'B'A float";

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

typedef float Color[3];

typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  Color color;
  Position pos;
} ColorSample;

#define SQUARE(x) ((x)*(x))
static inline float get_alpha (Color F, Color B, Color I)
{
  int c;
  float result = 0;
  float div = 0;
  for (c = 0; c < 3; c++)
    {
      result += (I[c] - B[c]) * (F[c] - B[c]);
      div += SQUARE(F[c] - B[c]);
    }
  return min(max(result / div, 0), 1);
}

static inline float get_color_cost (Color F, Color B, Color I, float alpha)
{
  int c;
  float result = 0;
  for (c = 0; c < 3; c++)
    {
      result += SQUARE(I[c] - (alpha * F[c] + (1 - alpha) * B[c]));
    }

  // TODO(rggjan): Remove sqrt to get faster code?
  // TODO(rggjan): Remove 255
  return sqrt(result) * 255;
}

static inline int get_distance_squared(ColorSample s, int x, int y)
{
  return SQUARE(s.pos.x - x) + SQUARE(s.pos.y - y);
}

static inline float get_distance (ColorSample s, int x, int y)
{
  // TODO(rggjan): Remove sqrt to get faster code?
  return sqrt(get_distance_squared(s, x, y));
}


static inline float get_distance_cost (ColorSample s, int x, int y, float *best_distance)
{
  float new_distance = get_distance(s, x, y);

  if (new_distance < *best_distance)
    *best_distance = new_distance;

  return new_distance / *best_distance;
}

static inline float get_cost (ColorSample foreground, ColorSample background, Color I, int x, int y, float *best_fg_distance, float *best_bg_distance)
{
  float cost = get_color_cost(foreground.color, background.color, I,
                              get_alpha(foreground.color, background.color, I));
  cost += get_distance_cost(foreground, x, y, best_fg_distance);
  cost += get_distance_cost(background, x, y, best_bg_distance);
  return cost;
}

static inline void do_propagate(GArray *foreground_samples, GArray *background_samples, gfloat *input, gfloat *output, guchar *trimap, int x, int y, int w, int h) {
  int index_orig = y * w + x;
  int index_new;

  if (!(trimap[index_orig] == 0 || trimap[index_orig] == 255))
    {
      int xdiff, ydiff;
      float best_cost = FLT_MAX;
      float *best_fg_distance = &output[index_orig * 4 + 0];
      float *best_bg_distance = &output[index_orig * 4 + 1];

      for (ydiff = -1; ydiff <= 1; ydiff++)
        {
          // Borders
          if (y+ydiff < 0 || y+ydiff >= h)
            continue;
          for (xdiff = -1; xdiff <= 1; xdiff++)
            {
              // Borders
              if (x+xdiff < 0 || x+xdiff >= w)
                continue;

              index_new = (y + ydiff) * w + (x + xdiff);

              if (!(trimap[index_new] == 0 || trimap[index_new] == 255))
                {
                  int fi = FG_INDEX(output, index_new);
                  int bi = BG_INDEX(output, index_new);

                  ColorSample foreground = g_array_index(foreground_samples, ColorSample, fi);
                  ColorSample background = g_array_index(background_samples, ColorSample, bi);

                  float cost = get_cost(foreground, background, &input[index_orig * 3], x, y, best_fg_distance, best_bg_distance);
                  if (cost < best_cost)
                    {
                      FG_INDEX(output, index_orig) = fi;
                      BG_INDEX(output, index_orig) = bi;
                      best_cost = cost;
                    }
                }
            }
        }
    }
}

static inline void do_random_search(GArray *foreground_samples, GArray *background_samples, gfloat *input, gfloat *output, int x, int y, int w) {
  int dist_f = foreground_samples->len;
  int dist_b = background_samples->len;
  int index = y * w + x;

  int best_fi = FG_INDEX(output, index);
  int best_bi = BG_INDEX(output, index);

  int start_fi = best_fi;
  int start_bi = best_bi;

  // Get current best result
  float *best_fg_distance = &FG_DISTANCE(output, index);
  float *best_bg_distance = &BG_DISTANCE(output, index);

  ColorSample foreground = g_array_index(foreground_samples, ColorSample, best_fi);
  ColorSample background = g_array_index(background_samples, ColorSample, best_bi);

  // Get cost
  float best_cost = get_cost(foreground, background, &input[index * 3], x, y, best_fg_distance, best_bg_distance);

  while (dist_f > 0 || dist_b > 0)
    {
      // Get new indices to check
      int fl = foreground_samples->len;
      int bl = background_samples->len;
      int fi = (start_fi + (rand() % (dist_f * 2 + 1)) + fl - dist_f) % fl;
      int bi = (start_bi + (rand() % (dist_b * 2 + 1)) + fl - dist_b) % bl;

      ColorSample foreground = g_array_index(foreground_samples, ColorSample, fi);
      ColorSample background = g_array_index(background_samples, ColorSample, bi);

      float cost = get_cost(foreground, background, &input[index * 3], x, y, best_fg_distance, best_bg_distance);

      if (cost < best_cost)
        {
          best_cost = cost;
          best_fi = fi;
          best_bi = bi;
        }
      dist_f /= 2;
      dist_b /= 2;
    }

  FG_INDEX(output, index) = best_fi;
  BG_INDEX(output, index) = best_bi;
}

// Compare color intensities
static gint color_compare(gconstpointer p1, gconstpointer p2)
{
  ColorSample *s1 = (ColorSample*) p1;
  ColorSample *s2 = (ColorSample*) p2;

  float sum1 = s1->color[0] + s1->color[1] + s1->color[2];
  float sum2 = s2->color[0] + s2->color[1] + s2->color[2];

  return ((sum1 > sum2) - (sum2 > sum1));
}

static gboolean
matting_process (GeglOperation       *operation,
                 GeglBuffer          *input_buf,
                 GeglBuffer          *aux_buf,
                 GeglBuffer          *output_buf,
                 const GeglRectangle *result,
                 int                  level)
{

  const GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
  gfloat           *input   = NULL;
  guchar           *trimap  = NULL;
  gfloat           *output  = NULL;

  gboolean          success = FALSE;
  int               w, h, i, x, y, xdiff, ydiff, neighbour_mask;

  GArray           *foreground_samples, *background_samples;
  GArray           *unknown_positions;

  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_INPUT )) == COMPONENTS_INPUT,  FALSE);
  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_AUX   )) == COMPONENTS_AUX,    FALSE);
  g_return_val_if_fail (babl_format_get_n_components (babl_format (FORMAT_OUTPUT)) == COMPONENTS_OUTPUT, FALSE);

  g_return_val_if_fail (operation,  FALSE);
  g_return_val_if_fail (input_buf,  FALSE);
  g_return_val_if_fail (aux_buf,    FALSE);
  g_return_val_if_fail (output_buf, FALSE);
  g_return_val_if_fail (result,     FALSE);

  w = result->width;
  h = result->height;

  input  = g_new (gfloat, w * h * COMPONENTS_INPUT);
  trimap = g_new (guchar, w * h * COMPONENTS_AUX);
  output = g_new0 (gfloat, w * h * COMPONENTS_OUTPUT);

  gegl_buffer_get (input_buf, result, 1.0, babl_format (FORMAT_INPUT), input, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  gegl_buffer_get (  aux_buf, result, 1.0, babl_format (FORMAT_AUX),  trimap, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

  foreground_samples = g_array_new(FALSE, FALSE, sizeof(ColorSample));
  background_samples = g_array_new(FALSE, FALSE, sizeof(ColorSample));
  unknown_positions = g_array_new(FALSE, FALSE, sizeof(Position));

  // Get mask
  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          int mask = trimap[y * w + x];
          for (ydiff = -1; ydiff <= 1; ydiff++)
            {
              // Borders
              if (y+ydiff < 0 || y+ydiff >= h)
                continue;

              for (xdiff = -1; xdiff <= 1; xdiff++)
                {
                  // Borders
                  if (x+xdiff < 0 || x+xdiff >= w)
                    continue;

                  neighbour_mask = trimap[(y + ydiff) * w + x + xdiff];
                  if (neighbour_mask != mask && (mask == 0 || mask == 255))
                    {
                      int index = y*w+x;
                      ColorSample s;
                      s.pos.x = x;
                      s.pos.y = y;
                      COLOR(s.color[c] = input[index*3 + c]);

                      if (mask == 255)
                        {
                          g_array_append_val(foreground_samples, s);
                          FG_DISTANCE(output, index) = 0;
                          BG_DISTANCE(output, index) = FLT_MAX;
                        }
                      else
                        {
                          g_array_append_val(background_samples, s);
                          FG_DISTANCE(output, index) = 0;
                          BG_DISTANCE(output, index) = FLT_MAX;
                        }

                      // Go to next pixel
                      xdiff = 1;
                      ydiff = 1;
                    }
                }
            }
        }
    }

  // Initialize unknowns
  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          int index = y * w + x;

          if (trimap[index] != 0 && trimap[index] != 255)
            {
              Position p;
              p.x = x;
              p.y = y;
              g_array_append_val(unknown_positions, p);
              FG_DISTANCE(output, index) = FLT_MAX;
              BG_DISTANCE(output, index) = FLT_MAX;
              FG_INDEX(output, index) = rand() % foreground_samples->len;
              BG_INDEX(output, index) = rand() % background_samples->len;
            }
        }
    }

  g_array_sort(foreground_samples, color_compare);
  g_array_sort(background_samples, color_compare);

  // Do real iterations
  for (i = 0; i < o->iterations; i++)
    {
      unsigned j;

      GEGL_NOTE (GEGL_DEBUG_PROCESS, "Iteration %i", i);

      for (j=0; j<unknown_positions->len; j++)
        {
          Position p = g_array_index(unknown_positions, Position, j);
          do_random_search(foreground_samples, background_samples, input, output, p.x, p.y, w);
        }

      for (j=0; j<unknown_positions->len; j++)
        {
          Position p = g_array_index(unknown_positions, Position, j);
          do_propagate(foreground_samples, background_samples, input, output, trimap, p.x, p.y, w, h);
        }
    }

  // Fill results in
  for (y = 0; y < h; y++)
    {
      for (x = 0; x < w; x++)
        {
          int index = y * w + x;
          if (trimap[index] == 0 || trimap[index] == 255)
            {
              // Use known values
              output[index * 4 + 0] = input[index * 3 + 0];
              output[index * 4 + 1] = input[index * 3 + 1];
              output[index * 4 + 2] = input[index * 3 + 2];

              if (trimap[index] == 0)
                {
                  output[index * 4 + 3] = 0;
                }
              else if (trimap[index] == 255)
                {
                  output[index * 4 + 3] = 1;
                }
            }
          else
            {
              ColorSample background, foreground;
              foreground = g_array_index(foreground_samples, ColorSample, FG_INDEX(output, index));
              background = g_array_index(background_samples, ColorSample, BG_INDEX(output, index));

              output[index * 4 + 3] = get_alpha(foreground.color, background.color, &input[index * 3]);

              COLOR(output[index * 4 + c] = foreground.color[c]);
            }
        }
    }

  // Save to buffer
  gegl_buffer_set (output_buf, result, 0, babl_format (FORMAT_OUTPUT), output,
                   GEGL_AUTO_ROWSTRIDE);
  success = TRUE;

  // Free memory
  g_free (input);
  g_free (trimap);
  g_free (output);
  g_array_free(foreground_samples, FALSE);
  g_array_free(background_samples, FALSE);
  g_array_free(unknown_positions, FALSE);

  return success;
}

static void gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);
  composer_class->process = matting_process;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->prepare                 = matting_prepare;
  operation_class->get_required_for_output = matting_get_required_for_output;
  operation_class->get_cached_region       = matting_get_cached_region;

  gegl_operation_class_set_keys (operation_class,
    "name"       , "gegl:matting-global",
    "categories" , "misc",
    "description",
   _("Given a sparse user supplied tri-map and an input image, create a "
     "foreground alpha matte. Set white as foreground, black as background "
     "for the tri-map. Everything else will be treated as unknown and filled in."),
    NULL);
}
#endif
