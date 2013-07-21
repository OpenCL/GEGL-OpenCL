/*
 * This file is part of n-point image deformation library.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#include "graphics.h"
#include "glib.h"
#include <math.h>
#include "npd_math.h"

void
npd_create_mesh_from_image (NPDModel *model,
                            gint      width,
                            gint      height,
                            gint      position_x,
                            gint      position_y)
{
  gint square_size = model->mesh_square_size;
  NPDHiddenModel *hidden_model = model->hidden_model;
  NPDImage *image = model->reference_image;
  gint i, cy, cx, y, x;
  NPDColor transparent = { 0, 0, 0, 0 };
  NPDColor pixel_color = { 0, 0, 0, 0 };
  GPtrArray *current_bones, *reference_bones;

  /* create quadrilaterals above the image */
  width  = ceil ((gfloat) width  / square_size);
  height = ceil ((gfloat) height / square_size);

  current_bones   = g_ptr_array_new ();
  reference_bones = g_ptr_array_new ();

  for (cy = 0; cy < height; cy++)
    {
      for (cx = 0; cx < width; cx++)
        {
          gboolean is_empty = TRUE;

          for (y = cy * square_size; y < (cy + 1) * square_size; y++)
            {
              for (x = cx * square_size; x < (cx + 1) * square_size; x++)
                {
                  npd_get_pixel_color (image, x, y, &pixel_color);

                  if (!npd_compare_colors (&pixel_color, &transparent))
                    {
                      is_empty = FALSE;
                      goto not_empty;
                    }
                }
            }

          not_empty:
          if (!is_empty)
            {
              NPDBone *current_bone, *reference_bone;
              NPDPoint *current_points, *ref_points;
              gfloat *weights;
              gint coords[8] = {cx, cx + 1, cx + 1, cx,
                                cy, cy,     cy + 1, cy + 1};

              current_bone   = g_new (NPDBone,  1);
              reference_bone = g_new (NPDBone,  1);
              current_points = g_new (NPDPoint, 4);
              ref_points     = g_new (NPDPoint, 4);
              weights        = g_new (gfloat,   4);
              
              for (i = 0; i < 4; i++)
                {
                  weights[i] = 1.0;
                
                  current_points[i].x = position_x + (coords[i] * square_size);
                  current_points[i].y = position_y + (coords[i + 4] * square_size);
                  current_points[i].weight = &weights[i];
                  current_points[i].fixed = FALSE;
                  current_points[i].current_bone = current_bone;
                  current_points[i].reference_bone = reference_bone;
                  current_points[i].index = i;

                  ref_points[i].x = current_points[i].x - position_x;
                  ref_points[i].y = current_points[i].y - position_y;
                  ref_points[i].weight = &weights[i];
                  ref_points[i].fixed = current_points[i].fixed;
                  ref_points[i].current_bone = current_bone;
                  ref_points[i].reference_bone = reference_bone;
                  ref_points[i].index = i;
                  
                  current_points[i].counterpart = &ref_points[i];
                  ref_points[i].counterpart = &current_points[i];
                }

              current_bone->points = current_points;
              current_bone->num_of_points = 4;
              current_bone->weights = weights;
              g_ptr_array_add (current_bones, current_bone);

              reference_bone->points = ref_points;
              reference_bone->num_of_points = 4;
              reference_bone->weights = weights;
              g_ptr_array_add (reference_bones, reference_bone);

              hidden_model->num_of_bones++;
            }
        }
    }
  
  hidden_model->current_bones = g_new (NPDBone, current_bones->len);
  hidden_model->reference_bones = g_new (NPDBone, reference_bones->len);
  
  for (i = 0; i < current_bones->len; i++)
    {
      hidden_model->current_bones[i] = *(NPDBone*) g_ptr_array_index (current_bones, i);
      hidden_model->reference_bones[i] = *(NPDBone*) g_ptr_array_index (reference_bones, i);
    }

  g_ptr_array_free(current_bones, TRUE);
  g_ptr_array_free(reference_bones, TRUE);
}

gboolean
npd_compare_colors (NPDColor *c1,
                    NPDColor *c2)
{
  if (c1->r == c2->r &&
      c1->g == c2->g &&
      c1->b == c2->b &&
      c1->a == c2->a)
    return TRUE;

  return FALSE;
}

gboolean
npd_is_color_transparent (NPDColor *color)
{
  if (color->a == 0 &&
      color->b == 0 &&
      color->g == 0 &&
      color->r == 0)
    return TRUE;

  return FALSE;
}

gint
npd_bilinear_interpolation (gint   I0,
                            gint   I1,
                            gint   I2,
                            gint   I3,
                            gfloat dx,
                            gfloat dy)
{
  return floor ((I0 * (1 - dx) + I1 * dx) * (1 - dy)
              + (I2 * (1 - dx) + I3 * dx) * dy);
}

void
npd_bilinear_color_interpolation (NPDColor *I0,
                                  NPDColor *I1,
                                  NPDColor *I2,
                                  NPDColor *I3,
                                  gfloat    dx,
                                  gfloat    dy,
                                  NPDColor *out)
{
  out->r = npd_bilinear_interpolation (I0->r, I1->r, I2->r, I3->r, dx, dy);
  out->g = npd_bilinear_interpolation (I0->g, I1->g, I2->g, I3->g, dx, dy);
  out->b = npd_bilinear_interpolation (I0->b, I1->b, I2->b, I3->b, dx, dy);
  out->a = npd_bilinear_interpolation (I0->a, I1->a, I2->a, I3->a, dx, dy);
}

void
npd_texture_fill_triangle (gint x1,
                           gint y1,
                           gint x2,
                           gint y2,
                           gint x3,
                           gint y3,
                           NPDMatrix *A,
                           NPDImage *input_image,
                           NPDImage *output_image)
{
  gint yA, yB, yC, xA, xB, xC;
  gint tmp, y;
  gint deltaXAB, deltaYAB;
  gint deltaXBC, deltaYBC;
  gint deltaXAC, deltaYAC;

  gfloat slopeAB;
  gfloat slopeBC;
  gfloat slopeAC;

  gfloat k, l;
  gfloat slope1, slope2, slope3, slope4;

  if (y1 == y2 && x1 > x2) {
      tmp = y1;y1 = y2;y2 = tmp;
      tmp = x1;x1 = x2;x2 = tmp;
  }
  if (y1 == y3 && x1 > x3) {
      tmp = y1;y1 = y3;y3 = tmp;
      tmp = x1;x1 = x3;x3 = tmp;
  }
  if (y2 == y3 && x2 > x3) {
      tmp = y2;y2 = y3;y3 = tmp;
      tmp = x2;x2 = x3;x3 = tmp;
  }

  if (y1 <= y2) {
      if (y2 <= y3) {
          // y1 <= y2 <= y3
          yA=y1;yB=y2;yC=y3;
          xA=x1;xB=x2;xC=x3;
      } else if (y1 <= y3) {
          // y1 <= y3 < y2
          yA=y1;yB=y3;yC=y2;
          xA=x1;xB=x3;xC=x2;
      } else {
          // y3 < y1 < y2
          yA=y3;yB=y1;yC=y2;
          xA=x3;xB=x1;xC=x2;
      }
  } else {
      if (y1 <= y3) {
          // y2 < y1 <= y3
          yA=y2;yB=y1;yC=y3;
          xA=x2;xB=x1;xC=x3;
      } else if (y2 <= y3) {
          // y2 <= y3 < y1
          yA=y2;yB=y3;yC=y1;
          xA=x2;xB=x3;xC=x1;
      } else {
          // y3 < y2 < y1
          yA=y3;yB=y2;yC=y1;
          xA=x3;xB=x2;xC=x1;
      }
  }

  deltaXAB = xB-xA, deltaYAB = yB-yA;
  deltaXBC = xC-xB, deltaYBC = yC-yB;
  deltaXAC = xC-xA, deltaYAC = yC-yA;

  slopeBC = (deltaYBC == 0 ? 0 : (gfloat) deltaXBC/deltaYBC);
  slopeAC = (deltaYAC == 0 ? 0 : (gfloat) deltaXAC/deltaYAC);

  if (deltaYAB == 0) {
      slopeAB = 0;
      k = xA;
      l = xB;

      slope1 = slopeAB;
      slope2 = slopeAC;
      slope3 = slopeAC;
      slope4 = slopeBC;
  } else {
      slopeAB = (gfloat) deltaXAB/deltaYAB;
      k = xA;
      l = xA;

      if (slopeAB > slopeAC) {
          slope1 = slopeAC;
          slope2 = slopeAB;
          slope3 = slopeAC;
          slope4 = slopeBC;
      } else {
          slope1 = slopeAB;
          slope2 = slopeAC;
          slope3 = slopeBC;
          slope4 = slopeAC;
      }
  }

  for (y=yA; y<yB; ++y) {
     npd_draw_texture_line((gint)round(k), (gint)round(l), y, A, input_image, output_image);
     k += slope1;
     l += slope2;
  }

  for (y=yB; y<=yC; ++y) {
     npd_draw_texture_line((gint)round(k), (gint)round(l), y, A, input_image, output_image);
     k += slope3;
     l += slope4;
  }
}

void
npd_texture_quadrilateral (NPDBone  *reference_bone,
                           NPDBone  *current_bone,
                           NPDImage *input_image,
                           NPDImage *output_image)
{
  /* p1 are points of domain, p2 are points of codomain */
  NPDPoint *p1 = current_bone->points;
  NPDPoint *p2 = reference_bone->points;

  NPDMatrix *A = NULL;
  npd_new_matrix(&A);

  npd_compute_affinity(&p1[0], &p1[1], &p1[2], &p2[0], &p2[1], &p2[2], A);
  npd_texture_fill_triangle((int)p1[0].x, (int)p1[0].y, (int)p1[1].x, (int)p1[1].y, (int)p1[2].x, (int)p1[2].y, A, input_image, output_image);

  npd_compute_affinity(&p1[0], &p1[2], &p1[3], &p2[0], &p2[2], &p2[3], A);
  npd_texture_fill_triangle((int)p1[0].x, (int)p1[0].y, (int)p1[2].x, (int)p1[2].y, (int)p1[3].x, (int)p1[3].y, A, input_image, output_image);

  npd_destroy_matrix(&A);
}

void
npd_draw_texture_line (gint        x1,
                       gint        x2,
                       gint        y,
                       NPDMatrix  *A,
                       NPDImage   *input_image,
                       NPDImage   *output_image)
{
  gint x, fx, fy;
  gfloat dx, dy;
  
  for (x = x1; x <= x2; x++)
    {
      NPDPoint p, q;
      NPDColor I0, I1, I2, I3, interpolated;

      q.x = x; q.y = y;
      npd_apply_transformation(A, &q, &p);

      fx = (gint) floor(p.x);
      fy = (gint) floor(p.y);
      dx = p.x - floor(p.x);
      dy = p.y - floor(p.y);

      /* bilinear interpolation */
      npd_get_pixel_color(input_image, fx, fy, &I0);
      npd_get_pixel_color(input_image, fx+1, fy, &I1);
      npd_get_pixel_color(input_image, fx, fy+1, &I2);
      npd_get_pixel_color(input_image, fx+1, fy+1, &I3);
      npd_bilinear_color_interpolation(&I0, &I1, &I2, &I3, dx, dy, &interpolated);

      npd_set_pixel_color (output_image, x, y, &interpolated);
    }
}
