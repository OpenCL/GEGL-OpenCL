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
#define GEGL_CHANT_C_FILE       "matting-global.c"

#include "gegl-chant.h"
#include "gegl-debug.h"
#include <stdlib.h>

#include <stdio.h>
#include <math.h>

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

#define COLOR(expr) {int c; for (c = 0; c < 3; c++) { expr; }}

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
matting_prepare (GeglOperation *operation) {
  gegl_operation_set_format (operation, "input",  babl_format (FORMAT_INPUT));
  gegl_operation_set_format (operation, "aux",    babl_format (FORMAT_AUX));
  gegl_operation_set_format (operation, "output", babl_format (FORMAT_OUTPUT));
}

static GeglRectangle
matting_get_required_for_output (GeglOperation       *operation,
                                 const gchar         *input_pad,
                                 const GeglRectangle *roi) {
  GeglRectangle result = *gegl_operation_source_get_bounding_box (operation,
                                                                  "input");
  return result;
}

static GeglRectangle
matting_get_cached_region (GeglOperation * operation,
                           const GeglRectangle * roi) {
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
static inline float get_alpha (Color F, Color B, Color I) {
  int c;
  float result = 0;
  float div = 0;
  for (c = 0; c < 3; c++) {
    result += (I[c] - B[c]) * (F[c] - B[c]);
    div += SQUARE(F[c] - B[c]);
  }
  return min(max(result / div, 0), 1);
}

static inline float get_color_cost (Color F, Color B, Color I, float alpha) {
  int c;
  float result = 0;
  for (c = 0; c < 3; c++) {
    result += SQUARE(I[c] - (alpha * F[c] + (1 - alpha) * B[c]));
  }

  // TODO(rggjan): Remove sqrt to get faster code?
  // TODO(rggjan): Remove 255
  return sqrt(result) * 255;
}

static inline int get_distance_squared(ColorSample s, int x, int y) {
  return SQUARE(s.pos.x - x) + SQUARE(s.pos.y - y);
}

static inline float get_distance (ColorSample s, int x, int y) {
  // TODO(rggjan): Remove sqrt to get faster code?
  return sqrt(get_distance_squared(s, x, y));
}


static inline float get_distance_cost (ColorSample s, int x, int y, float *best_distance) {
  float new_distance = get_distance(s, x, y);

  if (new_distance < *best_distance)
    *best_distance = new_distance;

  return new_distance / *best_distance;
}

static inline float get_cost (ColorSample foreground, ColorSample background, Color I, int x, int y, float *best_fg_distance, float *best_bg_distance, Color avgf, Color avgb, float devf, float devb) {
  float cost = 0;
  //printf("I: %f/%f/%f at %i,%i\n", I[0], I[1], I[2], x, y);
  //printf("best: %f/%f\n", best_fg_distance, best_bg_distance);
  //printf("F: %f/%f/%f at %i,%i\n", foreground.color[0], foreground.color[1], foreground.color[2], foreground.pos[0], foreground.pos[1]);
  //printf("B: %f/%f/%f at %i,%i\n", background.color[0], background.color[1], background.color[2], background.pos[0], background.pos[1]);

//  COLOR(cost += SQUARE(foreground.color[c] - avgf[c]) / devf);
//  COLOR(cost += SQUARE(background.color[c] - avgb[c]) / devb);
  //cost *= 255;

  cost += get_color_cost(foreground.color, background.color, I,
                         get_alpha(foreground.color, background.color, I));
  //printf("cost1: %f\n", cost);
  cost += get_distance_cost(foreground, x, y, best_fg_distance);
  //printf("costf: %f\n", get_distance_cost(foreground, x, y, best_fg_distance));
  cost += get_distance_cost(background, x, y, best_bg_distance);
  //printf("costb: %f\n", get_distance_cost(foreground, x, y, best_bg_distance));
  return cost;
}

static inline void get_average (GArray *foreground_samples, GArray *background_samples, gfloat *input, gfloat *output, int x, int y, int w, int h, float *avgf, float *avgb, float *devf, float *devb) {
  int xdiff, ydiff;
  int fg_count = 0;
  int bg_count = 0;

/*  COLOR(avgf[c] = 0);
  COLOR(avgb[c] = 0);
  *devf = 0;
  *devb = 0;*/

  // Get average
  for (ydiff = -1; ydiff <= 1; ydiff++) {
    // Borders
    if (y+ydiff < 0 || y+ydiff >= h)
      continue;
    for (xdiff = -1; xdiff <= 1; xdiff++) {
      int i = (y + ydiff) * w + x + xdiff;
      // Borders
      if (x+xdiff < 0 || x+xdiff >= w)
        continue;

      // Distance to background != 0
      if (output[i * 4 + 1] != 0) {
        ColorSample foreground = g_array_index(foreground_samples, ColorSample, FG_INDEX(output, i));
        COLOR(avgf[c] += foreground.color[c]);
        fg_count++;
      }

      // Distance to foreground != 0
      if (output[i * 4 + 0] != 0) {
        ColorSample background = g_array_index(background_samples, ColorSample, BG_INDEX(output, i));
        COLOR(avgb[c] += background.color[c]);
        bg_count++;
      }
    }
  }

  ASSERT(fg_count > 0);
  ASSERT(bg_count > 0);

  COLOR(avgf[c] /= fg_count);
  COLOR(avgb[c] /= bg_count);

//  COLOR(printf("%f ", avgf[c]));
//  printf("\n\n");

  // get deviation
  for (ydiff = -1; ydiff <= 1; ydiff++) {
    // Borders
    if (y+ydiff < 0 || y+ydiff >= h)
      continue;
    for (xdiff = -1; xdiff <= 1; xdiff++) {
      int i = (y + ydiff) * w + x + xdiff;
      // Borders
      if (x+xdiff < 0 || x+xdiff >= w)
        continue;

      // Distance to background != 0
      if (output[i * 4 + 1] != 0) {
        ColorSample foreground = g_array_index(foreground_samples, ColorSample, FG_INDEX(output, i));
        COLOR(*devf += SQUARE(avgf[c] - foreground.color[c]) / fg_count);
      }

      // Distance to foreground != 0
      if (output[i * 4 + 0] != 0) {
        ColorSample background = g_array_index(background_samples, ColorSample, BG_INDEX(output, i));
        COLOR(*devb += SQUARE(avgb[c] - background.color[c]) / bg_count);
      }
    }
  }
}

static inline void do_propagate(GArray *foreground_samples, GArray *background_samples, gfloat *input, gfloat *output, guchar *trimap, int x, int y, int w, int h) {
  int index_orig = y * w + x;
  int index_new;

  Color avgf = {0, 0, 0};
  Color avgb = {0, 0, 0};

  float devf = 0;
  float devb = 0;

  get_average(foreground_samples, background_samples, input, output, x, y, w, h, avgf, avgb, &devf, &devb);

  if (!(trimap[index_orig] == 0 || trimap[index_orig] == 255)) {
    int xdiff, ydiff;
    float best_cost = FLT_MAX;
    float *best_fg_distance = &output[index_orig * 4 + 0];
    float *best_bg_distance = &output[index_orig * 4 + 1];

    for (ydiff = -1; ydiff <= 1; ydiff++) {
      // Borders
      if (y+ydiff < 0 || y+ydiff >= h)
        continue;
      for (xdiff = -1; xdiff <= 1; xdiff++) {
        // Borders
        if (x+xdiff < 0 || x+xdiff >= w)
          continue;

        index_new = (y + ydiff) * w + (x + xdiff);

        if (!(trimap[index_new] == 0 || trimap[index_new] == 255)) {
          int fi = FG_INDEX(output, index_new);
          int bi = BG_INDEX(output, index_new);

          ColorSample foreground = g_array_index(foreground_samples, ColorSample, fi);
          ColorSample background = g_array_index(background_samples, ColorSample, bi);

          float cost = get_cost(foreground, background, &input[index_orig * 3], x, y, best_fg_distance, best_bg_distance, avgf, avgb, devf, devb);
          if (cost < best_cost) {
            FG_INDEX(output, index_orig) = fi;
            BG_INDEX(output, index_orig) = bi;
            best_cost = cost;
          }
        }
      }
    }
  }
}

static inline void do_random_search(GArray *foreground_samples, GArray *background_samples, gfloat *input, gfloat *output, int x, int y, int w, int h) {
  int dist_f = foreground_samples->len;
  int dist_b = background_samples->len;
  int index = y * w + x;

  int best_fi = FG_INDEX(output, index);
  int best_bi = BG_INDEX(output, index);

  int start_fi = best_fi;
  int start_bi = best_bi;

  Color avgf = {0, 0, 0};
  Color avgb = {0, 0, 0};

  float devf = 0;
  float devb = 0;

  // Get current best result
  float *best_fg_distance = &output[index * 4 + 0];
  float *best_bg_distance = &output[index * 4 + 1];

  ColorSample foreground = g_array_index(foreground_samples, ColorSample, best_fi);
  ColorSample background = g_array_index(background_samples, ColorSample, best_bi);

  float best_cost;

  // Get average cost
  get_average(foreground_samples, background_samples, input, output, x, y, w, h, avgf, avgb, &devf, &devb);
  best_cost = get_cost(foreground, background, &input[index * 3], x, y, best_fg_distance, best_bg_distance, avgf, avgb, devf, devb);

  //printf("Got best cost: %i\n", best_cost);
  while (dist_f > 0 && dist_b > 0) {
    int fi, bi;
    float cost;
    ColorSample background, foreground;
    fi = (start_fi + (rand() % (dist_f * 2 + 1)) - dist_f) % foreground_samples->len;
    bi = (start_bi + (rand() % (dist_b * 2 + 1)) - dist_b) % background_samples->len;
    background = g_array_index(background_samples, ColorSample, bi);
    foreground = g_array_index(foreground_samples, ColorSample, fi);
    cost = get_cost(foreground, background, &input[index * 3], x, y, best_fg_distance, best_bg_distance, avgf, avgb, devf, devb);
    if (cost < best_cost) {
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

static gint color_compare(gconstpointer p1, gconstpointer p2) {
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
                 const GeglRectangle *result) {

  //const GeglChantO *o = GEGL_CHANT_PROPERTIES (operation);
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

  gegl_buffer_get (input_buf, 1.0, result, babl_format (FORMAT_INPUT), input, GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_get (  aux_buf, 1.0, result, babl_format (FORMAT_AUX),  trimap, GEGL_AUTO_ROWSTRIDE);

  foreground_samples = g_array_new(FALSE, FALSE, sizeof(ColorSample));
  background_samples = g_array_new(FALSE, FALSE, sizeof(ColorSample));
  unknown_positions = g_array_new(FALSE, FALSE, sizeof(Position));

  // Get mask
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int mask = trimap[y * w + x];
      for (ydiff = -1; ydiff <= 1; ydiff++) {
        // Borders
        if (y+ydiff < 0 || y+ydiff >= h)
          continue;

        for (xdiff = -1; xdiff <= 1; xdiff++) {
          // Borders
          if (x+xdiff < 0 || x+xdiff >= w)
            continue;

          neighbour_mask = trimap[(y + ydiff) * w + x + xdiff];
          if (neighbour_mask != mask && (mask == 0 || mask == 255)) {
            int index = y*w+x;
            ColorSample s;
            s.pos.x = x;
            s.pos.y = y;
            COLOR(s.color[c] = input[index * 3 + c])

            if (mask == 255) {
              g_array_append_val(foreground_samples, s);
              output[index*4+0] = 0;
              output[index*4+1] = FLT_MAX;
            } else {
              g_array_append_val(background_samples, s);
              output[index*4+0] = FLT_MAX;
              output[index*4+1] = 0;
            }

            // Go to next pixel
            xdiff = 1;
            ydiff = 1;
          }
        }
      }
    }
  }

  // Initialize
  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int index = y * w + x;

      if (trimap[index] != 0 && trimap[index] != 255) {
        Position p;
        p.x = x;
        p.y = y;
        g_array_append_val(unknown_positions, p);
        output[index * 4 + 0] = FLT_MAX;
        output[index * 4 + 1] = FLT_MAX;
        FG_INDEX(output, index) = rand() % foreground_samples->len;
        BG_INDEX(output, index) = rand() % background_samples->len;
      }
    }
  }

  g_array_sort(foreground_samples, color_compare);
  g_array_sort(background_samples, color_compare);

  for (i = 0; i < 10; i++) {
    int j;

    printf("Iteration %i\n", i);

    for (j=0; j<unknown_positions->len; j++) {
      Position p = g_array_index(unknown_positions, Position, j);
      int x = p.x;
      int y = p.y;
      do_random_search(foreground_samples, background_samples, input, output, x, y, w, h);
    }

    for (j=0; j<unknown_positions->len; j++) {
      Position p = g_array_index(unknown_positions, Position, j);
      int x = p.x;
      int y = p.y;
      do_propagate(foreground_samples, background_samples, input, output, trimap, x, y, w, h);
    }
      
  }

  for (y = 0; y < h; y++) {
    for (x = 0; x < w; x++) {
      int index = y * w + x;
      if (trimap[index] == 0 || trimap[index] == 255) {
        // Use known values
        output[index * 4 + 0] = input[index * 3 + 0];
        output[index * 4 + 1] = input[index * 3 + 1];
        output[index * 4 + 2] = input[index * 3 + 2];

        if (trimap[index] == 0) {
          output[index * 4 + 3] = 0;
        } else if (trimap[index] == 255) {
          output[index * 4 + 3] = 1;
        }
      } else {
        int c;
        ColorSample background, foreground;
        foreground = g_array_index(foreground_samples, ColorSample, FG_INDEX(output, index));
        background = g_array_index(background_samples, ColorSample, BG_INDEX(output, index));

        output[index * 4 + 3] = get_alpha(foreground.color, background.color, &input[index * 3]);

        for (c = 0; c < 3; c++) {
          output[index * 4 + c] = foreground.color[c];
        }
      }
    }
  }

  gegl_buffer_set (output_buf, result, babl_format (FORMAT_OUTPUT), output,
                   GEGL_AUTO_ROWSTRIDE);
  success = TRUE;
  g_free (input);
  g_free (trimap);
  g_free (output);
  g_array_free(foreground_samples, FALSE);
  g_array_free(background_samples, FALSE);
  g_array_free(unknown_positions, FALSE);
  return success;
}

static void gegl_chant_class_init (GeglChantClass *klass) {
  GeglOperationClass         *operation_class;
  GeglOperationComposerClass *composer_class;

  composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);
  composer_class->process = matting_process;

  operation_class = GEGL_OPERATION_CLASS (klass);
  operation_class->prepare                 = matting_prepare;
  operation_class->get_required_for_output = matting_get_required_for_output;
  operation_class->get_cached_region       = matting_get_cached_region;
  operation_class->name        = "gegl:matting-global";
  operation_class->categories  = "misc";
  operation_class->description =
    _("Given a sparse user supplied tri-map and an input image, create a "
      "foreground alpha mat. Set white as selected, black as unselected, "
      "for the tri-map.");
}
#endif
