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

#include "deformation.h"
#include "npd_math.h"
#include <math.h>

static void
npd_compute_centroid_of_overlapping_points (gint       num_of_points,
                                            NPDPoint **points,
                                            NPDPoint  *centroid)
{
  gfloat x_sum = 0, y_sum = 0;
  gint i;

  /* first compute sum of all values for each coordinate */
  for (i = 0; i < num_of_points; ++i)
    {
      x_sum += points[i]->x;
      y_sum += points[i]->y;
    }

  /* then compute mean */
  centroid->x = x_sum / num_of_points;
  centroid->y = y_sum / num_of_points;
}

static void
npd_compute_centroid_from_weighted_points (gint      num_of_points,
                                           NPDPoint *points,
                                           gfloat   *weights,
                                           NPDPoint *centroid)
{
  gfloat x_sum = 0, y_sum = 0, weights_sum = 0;
  gint i;

  /* first compute sum of all values for each coordinate */
  for (i = 0; i < num_of_points; ++i)
    {
      x_sum       += weights[i] * points[i].x;
      y_sum       += weights[i] * points[i].y;
      weights_sum += weights[i];
    }

  /* then compute mean */
  centroid->x = x_sum / weights_sum;
  centroid->y = y_sum / weights_sum;
}

static void
npd_compute_ARSAP_transformation (gint      num_of_points,
                                  NPDPoint *reference_points,
                                  NPDPoint *current_points,
                                  gfloat   *weights,
                                  gboolean  ASAP)
{
  NPDPoint pc = {0, 0}, qc = {0, 0};
  gfloat a = 0, b = 0, mu_part = 0, mu, r1, r2, x0, y0;
  gint i;

  /* p - points of reference pose */
  npd_compute_centroid_from_weighted_points (num_of_points,
                                             reference_points,
                                             weights,
                                            &pc);
  /* q - points of current pose */
  npd_compute_centroid_from_weighted_points (num_of_points,
                                             current_points,
                                             weights,
                                            &qc);

  /* get rotation */
  for (i = 0; i < num_of_points; ++i)
    {
      gfloat px_minus_pcx = reference_points[i].x - pc.x;
      gfloat py_minus_pcy = reference_points[i].y - pc.y;
      gfloat qx_minus_qcx =   current_points[i].x - qc.x;
      gfloat qy_minus_qcy =   current_points[i].y - qc.y;

      a += weights[i]
              * ((px_minus_pcx)
              *  (qx_minus_qcx)
              +  (py_minus_pcy)
              *  (qy_minus_qcy));
      b += weights[i]
              * ((px_minus_pcx)
              *  (qy_minus_qcy)
              -  (py_minus_pcy)
              *  (qx_minus_qcx));

      mu_part += weights[i]
              * ((px_minus_pcx)
              *  (px_minus_pcx)
              +  (py_minus_pcy)
              *  (py_minus_pcy));
    }

  mu = 1;
  if (ASAP) mu = mu_part;
  else      mu = sqrt (a * a + b * b);

  if (npd_equal_floats (mu, 0)) mu = NPD_EPSILON;

  r1 =  a / mu;
  r2 = -b / mu;

  /* get translation */
  x0 = qc.x - ( r1 * pc.x + r2 * pc.y);
  y0 = qc.y - (-r2 * pc.x + r1 * pc.y);

  /* transform points */
  for (i = 0; i < num_of_points; ++i)
    {
      if (!current_points[i].fixed)
        {
          current_points[i].x =  r1 * reference_points[i].x +
                                 r2 * reference_points[i].y + x0;
          current_points[i].y = -r2 * reference_points[i].x +
                                 r1 * reference_points[i].y + y0;
        }
    }
}

static void
npd_compute_ARSAP_transformations (NPDHiddenModel *hidden_model)
{
  gint     i;

  for (i = 0; i < hidden_model->num_of_bones; ++i)
    {
      NPDBone *reference_bone = &hidden_model->reference_bones[i];
      NPDBone *current_bone   = &hidden_model->current_bones[i];
      npd_compute_ARSAP_transformation (reference_bone->num_of_points,
                                        reference_bone->points,
                                        current_bone->points,
                                        current_bone->weights,
                                        hidden_model->ASAP);
    }
}

static void
npd_deform_hidden_model_once (NPDHiddenModel *hidden_model)
{
  gint i, j;
  npd_compute_ARSAP_transformations (hidden_model);

  /* overlapping points are not overlapping after the deformation,
     so we have to move them to their centroid */
  for (i = 0; i < hidden_model->num_of_overlapping_points; ++i)
    {
      NPDOverlappingPoints *list_of_ops
              = &hidden_model->list_of_overlapping_points[i];
      NPDPoint centroid;

      npd_compute_centroid_of_overlapping_points (list_of_ops->num_of_points,
                                                  list_of_ops->points,
                                                 &centroid);

      for (j = 0; j < list_of_ops->num_of_points; ++j)
        {
          list_of_ops->points[j]->x = centroid.x;
          list_of_ops->points[j]->y = centroid.y;
        }
    }
}

static void
npd_deform_model_once (NPDModel *model)
{
  gint i, j;

  /* updates associated overlapping points according to this control point */
  for (i = 0; i < model->control_points->len; ++i)
    {
      NPDControlPoint *cp = &g_array_index (model->control_points,
                                            NPDControlPoint,
                                            i);

      for (j = 0; j < cp->overlapping_points->num_of_points; ++j)
        {
          npd_set_point_coordinates (cp->overlapping_points->points[j],
                                     &cp->point);
        }
    }

  npd_deform_hidden_model_once (model->hidden_model);
}

void
npd_deform_model (NPDModel *model,
                  gint      rigidity)
{
  gint i;
  for (i = 0; i < rigidity; ++i)
    {
      npd_deform_model_once (model);
    }
}
