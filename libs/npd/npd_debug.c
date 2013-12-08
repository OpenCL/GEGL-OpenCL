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

#include "npd_debug.h"
#include "npd_common.h"
#include <glib.h>
#include <glib/gprintf.h>

void
npd_print_model (NPDModel        *model,
                 gboolean         print_control_points)
{
  gint i;
  g_printf ("NPDModel:\n");
  g_printf ("control point radius: %f\n", model->control_point_radius);
  g_printf ("control points visible: %d\n", model->control_points_visible);
  g_printf ("mesh visible: %d\n", model->mesh_visible);
  g_printf ("texture visible: %d\n", model->texture_visible);
  g_printf ("mesh square size: %d\n", model->mesh_square_size);

  npd_print_hidden_model (model->hidden_model, FALSE, FALSE);

  if (print_control_points)
    {
      g_printf ("%d control points:\n", model->control_points->len);
      for (i = 0; i < model->control_points->len; i++)
        {
          NPDControlPoint *cp = &g_array_index (model->control_points,
                                                NPDControlPoint,
                                                i);
          npd_print_point (&cp->point, TRUE);
        }
    }
}

void
npd_print_hidden_model (NPDHiddenModel *hm,
                        gboolean        print_bones,
                        gboolean        print_overlapping_points)
{
  gint i;
  g_printf ("NPDHiddenModel:\n");
  g_printf ("number of bones: %d\n", hm->num_of_bones);
  g_printf ("ASAP: %d\n", hm->ASAP);
  g_printf ("MLS weights: %d\n", hm->MLS_weights);
  g_printf ("number of overlapping points: %d\n", hm->num_of_overlapping_points);

  if (print_bones)
    {
      g_printf ("bones:\n");
      for (i = 0; i < hm->num_of_bones; i++)
        {
          npd_print_bone (&hm->current_bones[i]);
        }
    }

  if (print_overlapping_points)
    {
      g_printf ("overlapping points:\n");
      for (i = 0; i < hm->num_of_overlapping_points; i++)
        {
          npd_print_overlapping_points (&hm->list_of_overlapping_points[i]);
        }
    }
}

void
npd_print_bone (NPDBone *bone)
{
  gint i;
  g_printf ("NPDBone:\n");
  g_printf ("number of points: %d\n", bone->num_of_points);
  g_printf ("points:\n");
  for (i = 0; i < bone->num_of_points; i++)
    {
      npd_print_point (&bone->points[i], TRUE);
    }
}

void
npd_print_point (NPDPoint *point,
                 gboolean  print_details)
{
  if (print_details)
    {
      g_printf ("(NPDPoint: x: %f, y: %f, weight: %f, fixed: %d)\n",
              point->x, point->y, *point->weight, point->fixed);
    }
  else
    {
      g_printf ("(NPDPoint: x: %f, y: %f)\n",
              point->x, point->y);
    }
}

void
npd_print_overlapping_points (NPDOverlappingPoints *op)
{
  gint i;
  g_printf ("NPDOverlappingPoints:\n");
  g_printf ("number of points: %d\n", op->num_of_points);
  g_printf ("representative: ");
  npd_print_point (op->representative, TRUE);
  g_printf ("points:\n");
  for (i = 0; i < op->num_of_points; i++)
    {
      npd_print_point (op->points[i], TRUE);
    }
}

