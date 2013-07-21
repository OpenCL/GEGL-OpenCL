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

#include "npd_common.h"
#include "npd_math.h"
#include <math.h>
#include <glib.h>
#include <glib/gprintf.h>

void
npd_init_model (NPDModel *model)
{
  NPDHiddenModel *hidden_model;
  GArray         *control_points;
  
  /* init hidden model */
  hidden_model = g_new (NPDHiddenModel, 1);
  model->hidden_model = hidden_model;
  hidden_model->ARAP = TRUE;
  hidden_model->num_of_bones = 0;
  hidden_model->num_of_overlapping_points = 0;

  /* init control points */
  control_points = g_array_new (FALSE, FALSE, sizeof (NPDControlPoint));
  model->control_points = control_points;
  model->control_point_radius = 6;
  model->control_points_visible = TRUE;

  /* init other */
  model->mesh_visible = TRUE;
  model->texture_visible = TRUE;
}

void
npd_destroy_hidden_model (NPDHiddenModel *hm)
{
  gint i;
  for (i = 0; i < hm->num_of_overlapping_points; i++)
    {
      g_free (hm->list_of_overlapping_points[i].points);
    }

  g_free (hm->list_of_overlapping_points);

  for (i = 0; i < hm->num_of_bones; i++)
    {
      g_free (hm->current_bones[i].weights);
      g_free (hm->current_bones[i].points);
      g_free (hm->reference_bones[i].points);
    }
  
  g_free (hm->current_bones);
  g_free (hm->reference_bones);
}

void
npd_destroy_model (NPDModel *model)
{
  /* destroy control points */
  g_array_free (model->control_points, TRUE);

  /* destroy hidden model */
  npd_destroy_hidden_model (model->hidden_model);
  g_free (model->hidden_model);
}

NPDControlPoint*
npd_add_control_point (NPDModel *model,
                       NPDPoint *coord)
{
  gint                  num_of_ops, i, closest;
  gfloat                min, current;
  NPDOverlappingPoints *list_of_ops;
  NPDControlPoint       cp;

  list_of_ops = model->hidden_model->list_of_overlapping_points;
  num_of_ops  = model->hidden_model->num_of_overlapping_points;

  /* find closest overlapping points */
  closest = 0;
  min     = npd_SED (coord, list_of_ops[0].representative);

  for (i = 1; i < num_of_ops; i++)
    {
      NPDPoint *op = list_of_ops[i].representative;
      current      = npd_SED(coord, op);

      if (min > current)
        {
          closest = i;
          min     = current;
        }
    }

  cp.point.weight       = list_of_ops[closest].representative->weight;
  cp.overlapping_points = &list_of_ops[closest];
  
  npd_set_point_coordinates (&cp.point, list_of_ops[closest].representative);
  g_array_append_val (model->control_points, cp);

  return &g_array_index (model->control_points,
                         NPDControlPoint,
                         model->control_points->len - 1);
}

void
npd_remove_control_point (NPDModel        *model,
                          NPDControlPoint *control_point)
{
  gint i;
  NPDControlPoint *cp;
  
  for (i = 0; i < model->control_points->len; i++)
    {
      cp = &g_array_index(model->control_points, NPDControlPoint, i);
      
      if (cp == control_point)
        {
          npd_set_control_point_weight(cp, 1.0);
          g_array_remove_index (model->control_points, i);
          return;
        }
    }
}

void
npd_remove_all_control_points (NPDModel *model)
{
  g_array_remove_range (model->control_points,
                        0,
                        model->control_points->len);
}

void
npd_set_control_point_weight (NPDControlPoint *cp,
                              gfloat           weight)
{
  npd_set_overlapping_points_weight(cp->overlapping_points, weight);
}

gboolean
npd_equal_coordinates (NPDPoint *p1,
                       NPDPoint *p2)
{
  return npd_equal_coordinates_epsilon(p1, p2, NPD_EPSILON);
}

gboolean
npd_equal_coordinates_epsilon (NPDPoint *p1,
                               NPDPoint *p2,
                               gfloat    epsilon)
{
  if (npd_equal_floats_epsilon (p1->x, p2->x, epsilon) &&
      npd_equal_floats_epsilon (p1->y, p2->y, epsilon))
    {
      return TRUE;
    }
  
  return FALSE;
}

NPDControlPoint*
npd_get_control_point_at (NPDModel *model,
                          NPDPoint *coord)
{
  gint i;
  for (i = 0; i < model->control_points->len; i++)
    {
      NPDControlPoint *cp = &g_array_index (model->control_points,
                                            NPDControlPoint,
                                            i);
      if (npd_equal_coordinates_epsilon (&cp->point,
                                          coord,
                                          model->control_point_radius))
        {
          return cp;
        }
    }

  g_printf ("no control points\n");
  return NULL;
}

void
npd_create_list_of_overlapping_points (NPDHiddenModel *hm)
{
  gint        i, j, num_of_bones;
  NPDBone    *bone;
  NPDPoint   *point;
  GPtrArray  *list_of_ops;
  GHashTable *coords_to_cluster;
  
  list_of_ops       = g_ptr_array_new ();
  num_of_bones      = hm->num_of_bones;
  coords_to_cluster = g_hash_table_new_full
                          (g_str_hash, g_str_equal,
                           g_free,     (GDestroyNotify) g_hash_table_destroy);

  for (i = 0; i < num_of_bones; i++)
    {
      bone = &hm->current_bones[i];

      for (j = 0; j < bone->num_of_points; j++)
        {
          point =  &bone->points[j];
          add_point_to_suitable_cluster (coords_to_cluster,
                                         point,
                                         list_of_ops);
        }
    }

  hm->list_of_overlapping_points = g_new (NPDOverlappingPoints,
                                          list_of_ops->len);
  hm->num_of_overlapping_points  = list_of_ops->len;

  for (i = 0; i < list_of_ops->len; i++)
    {
      GPtrArray *op = g_ptr_array_index (list_of_ops, i);
      hm->list_of_overlapping_points[i].points = (NPDPoint**) op->pdata;
      hm->list_of_overlapping_points[i].num_of_points = op->len;
      hm->list_of_overlapping_points[i].representative =
              hm->list_of_overlapping_points[i].points[0];
      
      for (j = 0; j < op->len; j++)
        {
          NPDPoint *p = hm->list_of_overlapping_points[i].points[j];
          p->overlapping_points = &hm->list_of_overlapping_points[i];
          p->counterpart->overlapping_points = &hm->list_of_overlapping_points[i];
        }

      g_ptr_array_free (op, FALSE); /* we want to preserve the underlying
                                       array */
    }

  /* free allocated memory */
  g_hash_table_destroy (coords_to_cluster);
  g_ptr_array_free (list_of_ops, TRUE);
}

#define NPD_FLOAT_TO_STRING(name_of_string, value)                             \
/* must be freed */                                                            \
name_of_string = g_new (gchar, 10);                                            \
g_ascii_dtostr (name_of_string, 10, value);

void
add_point_to_suitable_cluster (GHashTable *coords_to_cluster,
                               NPDPoint   *point,
                               GPtrArray  *list_of_overlapping_points)
{
  gchar      *str_coord_x, *str_coord_y;
  GHashTable *coord_y;
  GPtrArray  *op;

  NPD_FLOAT_TO_STRING (str_coord_x, point->x);
  NPD_FLOAT_TO_STRING (str_coord_y, point->y);
  
  coord_y = g_hash_table_lookup (coords_to_cluster, str_coord_x);

  if (coord_y == NULL)
    {
      /* coordinate doesn't exist */
      coord_y = g_hash_table_new_full (g_str_hash,  /* is freed during   */
                                       g_str_equal, /* destroying        */
                                       g_free,      /* coords_to_cluster */
                                       NULL);       /* hash table        */
      g_hash_table_insert (coords_to_cluster, str_coord_x, coord_y);
    }
  
  op = g_hash_table_lookup (coord_y, str_coord_y);
  if (op == NULL)
    {
      op = g_ptr_array_new ();
      g_hash_table_insert (coord_y, str_coord_y, op);
      g_ptr_array_add (list_of_overlapping_points, op);
    }
  
  g_ptr_array_add (op, point);
}

void
npd_set_overlapping_points_weight (NPDOverlappingPoints *op,
                                   gfloat                weight)
{
  gint i;
  for (i = 0; i < op->num_of_points; i++)
    {
      (*op->points[i]->weight) = weight;
    }
}

void
npd_set_point_coordinates (NPDPoint *target,
                           NPDPoint *source)
{
  target->x = source->x;
  target->y = source->y;
}

void
npd_print_hidden_model (NPDHiddenModel *hm,
                        gboolean        print_bones,
                        gboolean        print_overlapping_points)
{
  gint i;
  g_printf ("NPDHiddenModel:\n");
  g_printf ("number of bones: %d\n", hm->num_of_bones);
  g_printf ("ARAP: %d\n", hm->ARAP);
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