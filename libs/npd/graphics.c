/*
 * This file is part of N-point image deformation library.
 *
 * N-point image deformation library is free software: you can
 * redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * N-point image deformation library is distributed in the hope
 * that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with N-point image deformation library.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2013 Marek Dvoroznak <dvoromar@gmail.com>
 */

#include "graphics.h"
#include "glib.h"
#include <math.h>
#include "npd_math.h"
#include "lattice_cut.h"
#include <stdio.h>
#include <string.h>

static gfloat
npd_bilinear_interpolation (gfloat I0,
                            gfloat I1,
                            gfloat I2,
                            gfloat I3,
                            gfloat dx,
                            gfloat dy)
{
  return (I0 * (1 - dx) + I1 * dx) * (1 - dy) +
         (I2 * (1 - dx) + I3 * dx) *      dy;
}

static void
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

static gfloat
npd_blend_band (gfloat src,
                gfloat dst,
                gfloat src_alpha,
                gfloat dst_alpha,
                gfloat out_alpha_recip)
{
  return (src * src_alpha +
          dst * dst_alpha * (1 - src_alpha)) * out_alpha_recip;
}

static void
npd_blend_colors (NPDColor *src,
                  NPDColor *dst,
                  NPDColor *out_color)
{
  gfloat src_A = src->a / 255.0,
         dst_A = dst->a / 255.0;
  gfloat out_alpha = src_A + dst_A * (1 - src_A);
  if (out_alpha > 0)
    {
      gfloat out_alpha_recip = 1 / out_alpha;

      out_color->r = npd_blend_band (src->r, dst->r, src_A, dst_A, out_alpha_recip);
      out_color->g = npd_blend_band (src->g, dst->g, src_A, dst_A, out_alpha_recip);
      out_color->b = npd_blend_band (src->b, dst->b, src_A, dst_A, out_alpha_recip);
    }
  out_color->a = out_alpha * 255;
}

void
npd_process_pixel_bilinear (NPDImage   *input_image,
                            gfloat      ix,
                            gfloat      iy,
                            NPDImage   *output_image,
                            gfloat      ox,
                            gfloat      oy,
                            NPDSettings settings)
{
  gint fx, fy;
  gfloat dx, dy;
  NPDColor I0, interpolated, *final_color;

  fx = floor (ix);
  fy = floor (iy);

  npd_get_pixel_color (input_image, fx, fy, &I0);
  final_color = &I0;

  /* bilinear interpolation */
  if (settings & NPD_BILINEAR_INTERPOLATION)
    {
      NPDColor I1, I2, I3;

      dx =  ix - fx;
      dy =  iy - fy;

      npd_get_pixel_color (input_image, fx + 1, fy,     &I1);
      npd_get_pixel_color (input_image, fx,     fy + 1, &I2);
      npd_get_pixel_color (input_image, fx + 1, fy + 1, &I3);
      npd_bilinear_color_interpolation (&I0, &I1, &I2, &I3, dx, dy, &interpolated);
      final_color = &interpolated;
    }

  /* alpha blending */
  if (settings & NPD_ALPHA_BLENDING)
    {
      NPDColor dest;
      npd_get_pixel_color (output_image, ox, oy, &dest);
      npd_blend_colors (final_color, &dest, final_color);
    }

  npd_set_pixel_color (output_image, ox, oy, final_color);
}

static void
npd_draw_texture_line (gint        x1,
                       gint        x2,
                       gint        y,
                       NPDMatrix  *A,
                       NPDImage   *input_image,
                       NPDImage   *output_image,
                       NPDSettings settings)
{
  gint x;

  for (x = x1; x <= x2; x++)
    {
      NPDPoint p, q;

      q.x = x; q.y = y;
      npd_apply_transformation (A, &q, &p);

      npd_process_pixel (input_image, p.x, p.y,
                         output_image, x, y,
                         settings);
    }
}

static void
npd_texture_fill_triangle (gint         x1,
                           gint         y1,
                           gint         x2,
                           gint         y2,
                           gint         x3,
                           gint         y3,
                           NPDMatrix   *A,
                           NPDImage    *input_image,
                           NPDImage    *output_image,
                           NPDSettings  settings)
{
  gint yA, yB, yC, xA, xB, xC;
  gint y;
  gint deltaXAB, deltaYAB;
  gint deltaXBC, deltaYBC;
  gint deltaXAC, deltaYAC;
  gint dX1, dX2, dX3, dX4;
  gint dY1, dY2, dY3, dY4;
  gint k, l;

  if (y1 <= y2)
    {
      if (y2 <= y3)
        {
          /* y1 <= y2 <= y3 */
          yA = y1; yB = y2; yC = y3;
          xA = x1; xB = x2; xC = x3;
        }
      else if (y1 <= y3)
        {
          /* y1 <= y3 < y2 */
          yA = y1; yB = y3; yC = y2;
          xA = x1; xB = x3; xC = x2;
        }
      else
        {
          /* y3 < y1 < y2 */
          yA = y3; yB = y1; yC = y2;
          xA = x3; xB = x1; xC = x2;
        }
    }
  else
    {
      if (y1 <= y3)
        {
          /* y2 < y1 <= y3 */
          yA = y2; yB = y1; yC = y3;
          xA = x2; xB = x1; xC = x3;
        }
      else if (y2 <= y3)
        {
          /* y2 <= y3 < y1 */
          yA = y2; yB = y3; yC = y1;
          xA = x2; xB = x3; xC = x1;
        }
      else
        {
          /* y3 < y2 < y1 */
          yA = y3; yB = y2; yC = y1;
          xA = x3; xB = x2; xC = x1;
        }
    }

  deltaXAB = xB - xA, deltaYAB = yB - yA;
  deltaXBC = xC - xB, deltaYBC = yC - yB;
  deltaXAC = xC - xA, deltaYAC = yC - yA;

  if (deltaYAB != 0)
    {
      gboolean test = ((gfloat) deltaXAB / deltaYAB) > ((gfloat) deltaXAC / deltaYAC);
      if (test)
        {
          dX1 = deltaXAC; dY1 = deltaYAC;
          dX2 = deltaXAB; dY2 = deltaYAB;
          dX3 = deltaXAC; dY3 = deltaYAC;
          dX4 = deltaXBC; dY4 = deltaYBC;
        }
      else
        {
          dX1 = deltaXAB; dY1 = deltaYAB;
          dX2 = deltaXAC; dY2 = deltaYAC;
          dX3 = deltaXBC; dY3 = deltaYBC;
          dX4 = deltaXAC; dY4 = deltaYAC;
        }

      k = xA * dY1; l = xA * dY2;

      for (y = yA; y < yB; y++)
        {
          npd_draw_texture_line (k / dY1, l / dY2 - 1,
                                 y, A,
                                 input_image, output_image,
                                 settings);
          k += dX1; l += dX2;
        }

      if (test) l = xB * dY4;
      else      k = xB * dY3;
    }
  else
    {
      if (deltaXAB > 0)
        {
          dX3 = deltaXAC; dY3 = deltaYAC;
          dX4 = deltaXBC; dY4 = deltaYBC;
          k = xA * dY3; l = xB * dY4;
        }
      else
        {
          dX3 = deltaXBC; dY3 = deltaYBC;
          dX4 = deltaXAC; dY4 = deltaYAC;
          k = xB * dY3; l = xA * dY4;
        }
    }

  for (y = yB; y < yC; y++)
    {
      npd_draw_texture_line (k / dY3, l / dY4 - 1,
                             y, A,
                             input_image, output_image,
                             settings);
      k += dX3; l += dX4;
    }
}

static void
npd_texture_quadrilateral (NPDBone    *reference_bone,
                           NPDBone    *current_bone,
                           NPDImage   *input_image,
                           NPDImage   *output_image,
                           NPDSettings settings)
{
  /* p1 are points of domain, p2 are points of codomain */
  NPDPoint *p1 = current_bone->points;
  NPDPoint *p2 = reference_bone->points;

  NPDMatrix A;

  npd_compute_affinity (&p1[0], &p1[1], &p1[2],
                        &p2[0], &p2[1], &p2[2], &A);
  npd_texture_fill_triangle ((gint) p1[0].x, (gint) p1[0].y,
                             (gint) p1[1].x, (gint) p1[1].y,
                             (gint) p1[2].x, (gint) p1[2].y,
                             &A, input_image, output_image,
                             settings);

  npd_compute_affinity (&p1[0], &p1[2], &p1[3],
                        &p2[0], &p2[2], &p2[3], &A);
  npd_texture_fill_triangle ((gint) p1[0].x, (gint) p1[0].y,
                             (gint) p1[2].x, (gint) p1[2].y,
                             (gint) p1[3].x, (gint) p1[3].y,
                             &A, input_image, output_image,
                             settings);
}

void
npd_draw_model_into_image (NPDModel *model,
                           NPDImage *image)
{
  NPDHiddenModel *hm = model->hidden_model;
  gint i;

  for (i = 0; i < hm->num_of_bones; i++)
    {
      npd_texture_quadrilateral (&hm->reference_bones[i],
                                 &hm->current_bones[i],
                                  model->reference_image,
                                  image,
                                  NPD_BILINEAR_INTERPOLATION | NPD_ALPHA_BLENDING);
    }
}

gboolean
npd_is_color_transparent (NPDColor *color)
{
  if (npd_equal_floats (color->a, 0.0))
    return TRUE;

  return FALSE;
}

static void
npd_create_mesh (NPDModel *model,
                 gint      width,
                 gint      height,
                 gint      position_x,
                 gint      position_y)
{
  gint      square_size = model->mesh_square_size;
  NPDImage *image = model->reference_image;
  gint      i, cy, cx, y, x, r, c, ow, oh;
  NPDColor  pixel_color;
  GArray   *squares;
  gint     *sq2id;
  gboolean *empty_squares;
  GList    *tmp_ops = NULL;
  GArray   *ops = g_array_new (FALSE, FALSE, sizeof (NPDOverlappingPoints));
  GList   **edges;
  NPDHiddenModel *hm = model->hidden_model;

  /* create squares above the image */
  width  = ceil ((gfloat) width  / square_size);
  height = ceil ((gfloat) height / square_size);

  squares = g_array_new (FALSE, FALSE, sizeof (NPDBone));
  empty_squares = g_new0 (gboolean, width * height);
  sq2id = g_new0 (gint, width * height);

  i = 0;
  for (cy = 0; cy < height; cy++)
  for (cx = 0; cx < width; cx++)
    {
      gboolean is_empty = TRUE;

      for (y = cy * square_size; y < (cy + 1) * square_size; y++)
      for (x = cx * square_size; x < (cx + 1) * square_size; x++)
        {
          /* test of emptiness */
          npd_get_pixel_color (image, x, y, &pixel_color);
          if (!npd_is_color_transparent (&pixel_color))
            {
              is_empty = FALSE;
              goto not_empty;
            }
        }

#define NPD_SQ_ID cy * width + cx

      not_empty:
      if (!is_empty)
        {
          /* create a square */
          NPDBone square;
          npd_create_square (&square,
                             position_x + cx * square_size,
                             position_y + cy * square_size,
                             square_size, square_size);
          g_array_append_val (squares, square);
          sq2id[NPD_SQ_ID] = i;
          i++;
        }
      else
        empty_squares[NPD_SQ_ID] = TRUE;
    }

  /* find empty edges */
  edges = npd_find_edges (model->reference_image, width, height, square_size);

  /* create provisional overlapping points */
#define NPD_ADD_P(op,r,c,point)                                              \
  if ((r) > -1 && (r) < (oh - 1) && (c) > -1 && (c) < (ow - 1) &&            \
      edges[op] == NULL && !empty_squares[(r) * width + (c)])                \
    {                                                                        \
      tmp_ops = g_list_append (tmp_ops, GINT_TO_POINTER ((r) * width + (c)));\
      tmp_ops = g_list_append (tmp_ops, GINT_TO_POINTER (point));            \
      num++;                                                                 \
    }

  ow = width + 1; oh = height + 1;

  for (r = 0; r < oh; r++)
  for (c = 0; c < ow; c++)
    {
      gint index = r * ow + c, num = 0;
      NPD_ADD_P (index, r - 1, c - 1, 2);
      NPD_ADD_P (index, r - 1, c,     3);
      NPD_ADD_P (index, r,     c,     0);
      NPD_ADD_P (index, r,     c - 1, 1);
      if (num > 0)
        tmp_ops = g_list_insert_before (tmp_ops,
                                        g_list_nth_prev (g_list_last (tmp_ops), 2 * num - 1),
                                        GINT_TO_POINTER (num));
    }
#undef NPD_ADD_P

  /* cut lattice's edges and continue with creating of provisional overlapping points */
  tmp_ops = g_list_concat (tmp_ops, npd_cut_edges (edges, ow, oh));
  for (i = 0; i < ow * oh; i++) g_list_free (edges[i]);
  g_free (edges);

  /* create model's bones */
  hm->num_of_bones = squares->len;
  hm->current_bones = (NPDBone*) (gpointer) squares->data;
  g_array_free (squares, FALSE);

  /* create model's overlapping points */
  while (g_list_next (tmp_ops))
    {
      GPtrArray *ppts = g_ptr_array_new ();
      gint count = GPOINTER_TO_INT (tmp_ops->data);
      gint j = 0;

      for (i = 0; i < count; i++)
        {
          gint sq, p;
          tmp_ops = g_list_next (tmp_ops);
          sq = GPOINTER_TO_INT (tmp_ops->data);
          tmp_ops = g_list_next (tmp_ops);
          p  = GPOINTER_TO_INT (tmp_ops->data);

          if (!empty_squares[sq])
            {
              g_ptr_array_add (ppts, &hm->current_bones[sq2id[sq]].points[p]);
              j++;
            }
        }

      if (j > 0)
        {
          NPDOverlappingPoints op;
          op.num_of_points = j;
          op.points = (NPDPoint**) ppts->pdata;
          g_ptr_array_free (ppts, FALSE);
          op.representative = op.points[0];
          g_array_append_val (ops, op);
        }
      tmp_ops = g_list_next (tmp_ops);
    }

  g_list_free (tmp_ops);
  g_free (empty_squares);
  g_free (sq2id);

  hm->num_of_overlapping_points = ops->len;
  hm->list_of_overlapping_points = (NPDOverlappingPoints*) (gpointer) ops->data;
  g_array_free (ops, FALSE);

  /* create reference bones according to current bones */
  hm->reference_bones = g_new (NPDBone, hm->num_of_bones);
  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone *current_bone = &hm->current_bones[i];
      NPDBone *reference_bone = &hm->reference_bones[i];
      NPDPoint *current_points, *ref_points;
      gint j, n = current_bone->num_of_points;

      reference_bone->num_of_points = n;
      reference_bone->points = g_new (NPDPoint, n);
      memcpy (reference_bone->points, current_bone->points, n * sizeof (NPDPoint));
      reference_bone->weights = current_bone->weights;

      current_points = current_bone->points;
      ref_points = reference_bone->points;

      for (j = 0; j < n; j++)
        {
          current_points[j].current_bone = current_bone;
          current_points[j].reference_bone = reference_bone;
          ref_points[j].current_bone = current_bone;
          ref_points[j].reference_bone = reference_bone;
          ref_points[j].x = current_points[j].x - position_x;
          ref_points[j].y = current_points[j].y - position_y;
          current_points[j].counterpart = &ref_points[j];
          ref_points[j].counterpart = &current_points[j];
        }
    }

#if 0
  /* could be useful later */
  gint j;
  for (i = 0; i < hm->num_of_overlapping_points; i++)
  for (j = 0; j < hm->list_of_overlapping_points[i].num_of_points; j++)
    {
      NPDPoint *p = hm->list_of_overlapping_points[i].points[j];
      p->overlapping_points = &hm->list_of_overlapping_points[i];
      p->counterpart->overlapping_points = &hm->list_of_overlapping_points[i];
    }
#endif
}

void
npd_create_model_from_image (NPDModel  *model,
                             NPDImage  *image,
                             gint       width,
                             gint       height,
                             gint       position_x,
                             gint       position_y,
                             gint       square_size)
{
  npd_init_model (model);
  model->reference_image = image;
  model->mesh_square_size = square_size;

  npd_create_mesh (model, width, height, position_x, position_y);
}

void
npd_draw_mesh (NPDModel   *model,
               NPDDisplay *display)
{
  NPDHiddenModel *hm = model->hidden_model;
  gint i, j;

  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone  *bone  = &hm->current_bones[i];
      NPDPoint *first = &bone->points[0];
      NPDPoint *p0, *p1 = NULL;

      for (j = 1; j < bone->num_of_points; j++)
        {
          p0 = &bone->points[j - 1];
          p1 = &bone->points[j];
          npd_draw_line (display, p0->x, p0->y, p1->x, p1->y);
        }
      npd_draw_line (display, p1->x, p1->y, first->x, first->y);
    }
}

void (*npd_process_pixel)   (NPDImage*, gfloat, gfloat, NPDImage*, gfloat, gfloat, NPDSettings) = NULL;
void (*npd_draw_line)       (NPDDisplay*, gfloat, gfloat, gfloat, gfloat) = NULL;
void (*npd_get_pixel_color) (NPDImage*, gint, gint, NPDColor*) = NULL;
void (*npd_set_pixel_color) (NPDImage*, gint, gint, NPDColor*) = NULL;
