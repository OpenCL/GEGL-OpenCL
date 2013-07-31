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

#include "refine.h"
#include "npd_common.h"
#include "graphics.h"
#include <glib.h>
#include <stdio.h>
#include "npd_math.h"

void
npd_swap_ints (gint *i,
               gint *j)
{
  gint tmp = *i;
  *i = *j;
  *j = tmp;
}

gboolean
npd_is_edge_empty (NPDImage *image,
                   NPDPoint *p1,
                   NPDPoint *p2)
{
  gint x, y;
  gint X1 = p1->x, Y1 = p1->y, X2 = p2->x, Y2 = p2->y;
  NPDColor color;

  if (Y1 >  Y2) npd_swap_ints (&Y1, &Y2);
  if (X1 >  X2) npd_swap_ints (&X1, &X2);
  if (Y1 == Y2) Y2++;
  if (X1 == X2) X2++;

  for (y = Y1; y < Y2; y++)
    {
      for (x = X1; x < X2; x++)
        {
          npd_get_pixel_color (image, x, y, &color);
          if (!npd_is_color_transparent (&color))
            return FALSE;
        }
    }
  
  return TRUE;
}

GHashTable*
npd_find_edges (NPDModel *model)
{
  gint i, j;
  GHashTable *edges = g_hash_table_new (g_direct_hash, g_direct_equal);
  NPDHiddenModel *hm = model->hidden_model;

  /*
   Go through the 'reference pose' lattice and construct a list of edges
   which are empty.

   _|_|_|_
   _|_|_|_
   _|_|_|_
    | | |   
   */

  for (i = 0; i < hm->num_of_bones; i++)
    {
      NPDBone *ref_bone = &hm->reference_bones[i];
      
      for (j = 1; j < 3; j++)
        {
          NPDPoint *p1, *p2;
          p1 = &ref_bone->points[j];
          p2 = &ref_bone->points[j + 1];

          if (p1->overlapping_points->num_of_points != 4 &&
              p2->overlapping_points->num_of_points != 4)
            {
              /* we are at border of lattice */
              continue;
            }

          if (npd_is_edge_empty (model->reference_image, p1, p2))
            {
              GSList *slist;

#define NPD_ADD_EDGE(p1, p2)                                                   \
              slist = g_hash_table_lookup (edges, p1 ->overlapping_points);    \
              g_hash_table_insert (edges, p1 ->overlapping_points,             \
                      g_slist_append (slist, p2 ->overlapping_points));
              
              NPD_ADD_EDGE(p1, p2)
              NPD_ADD_EDGE(p2, p1)
            }
        }
    }

  return edges;
}

void
npd_sort_overlapping_points (NPDOverlappingPoints *op)
{
  if (op->num_of_points == 4)
    {  
      NPDPoint *pts[4];
      NPDPoint *p;
      gint i;
      for (i = 0; i < op->num_of_points; i++)
        {
          p = op->points[i];
          pts[p->index] = p;
        }

      for (i = 0; i < op->num_of_points; i++)
        {
          op->points[i] = pts[i];
        }
    }
}

#define NPD_NEW_OVERLAPPING_POINTS(name, number_of_points, a, b, c, d)         \
name .num_of_points  = number_of_points;                                       \
name .points         = g_new (NPDPoint*, 4);                                   \
name .points[0]      = a;                                                      \
name .points[1]      = b;                                                      \
name .points[2]      = c;                                                      \
name .points[3]      = d;                                                      \
name .representative = name .points[0];                      

void
npd_handle_corner_and_segment (NPDOverlappingPoints *center,
                               NPDOverlappingPoints *p1,
                               NPDOverlappingPoints *p2,
                               GHashTable *changed_ops)
{
  gboolean x_differs = FALSE,
           y_differs = FALSE;
  NPDOverlappingPoints *new = g_new (NPDOverlappingPoints, 2);
  gint a, b, c, d;
  
  if (!npd_equal_floats (p1->representative->x, p2->representative->x))
    x_differs = TRUE;
  if (!npd_equal_floats (p1->representative->y, p2->representative->y))
    y_differs = TRUE;

  if (x_differs && y_differs)
    {
      /* corner */
      NPDOverlappingPoints *B, *C;
      B = p1;
      C = p2;
      
      if (npd_equal_floats (center->representative->x, p1->representative->x))
        {
          B = p2;
          C = p1;
        }
      
      if (center->representative->y < C->representative->y)
        {
          if (center->representative->x < B->representative->x)
            {
              /* IV. quadrant */
              a = 2; b = 3; c = 1; d = 0;
            }
          else
            {
              /* III. quadrant */
              a = 2; b = 3; c = 0; d = 1;
            }
        }
      else
        {
          if (center->representative->x < B->representative->x)
            {
              /* I. quadrant */
              a = 2; b = 0; c = 1; d = 3;
            }
          else
            {
              /* II. quadrant */
              a = 0; b = 3; c = 1; d = 2;
            }
        
        }

      NPD_NEW_OVERLAPPING_POINTS(new[0], 3, center->points[a], center->points[b], center->points[c], NULL)
      NPD_NEW_OVERLAPPING_POINTS(new[1], 1, center->points[d], NULL, NULL, NULL)
    }
  else
    {
      /* segment */
      a = 0; b = 1; c = 2; d = 3;
      if (y_differs) { a = 1; b = 2; c = 0; d = 3; }

      NPD_NEW_OVERLAPPING_POINTS(new[0], 2, center->points[a], center->points[b], NULL, NULL)
      NPD_NEW_OVERLAPPING_POINTS(new[1], 2, center->points[c], center->points[d], NULL, NULL)
    }

  g_hash_table_insert (changed_ops, center, &new[0]);
  g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[1]));
}

void
npd_handle_3_neighbors (NPDOverlappingPoints *center,
                        NPDOverlappingPoints *p1,
                        NPDOverlappingPoints *p2,
                        NPDOverlappingPoints *p3,
                        GHashTable *changed_ops)
{
  NPDOverlappingPoints *new = g_new (NPDOverlappingPoints, 3);
  NPDOverlappingPoints *B, *C, *D;
  gint a = 2, b = 1, c = 3, d = 0;
  B = p1; C = p2; D = p3;

  if (!npd_equal_floats (B->representative->x, C->representative->x) &&
      !npd_equal_floats (B->representative->y, C->representative->y))
    {
      /* B and C form corner */
      B = p3; D = p1;
    }

  if (!npd_equal_floats (B->representative->x, C->representative->x) &&
      !npd_equal_floats (B->representative->y, C->representative->y))
    {
      /* B and C form corner */
      C = p3; D = p2;
    }

  /* B and C form segment */
  if (npd_equal_floats (B->representative->x, C->representative->x))
    {
      if (B->representative->x < D->representative->x)
        {
          /* |_
             |  */
          a = 2; b = 1; c = 3; d = 0;
        }
      else
        {
          /* _|
              | */
          a = 3; b = 0; c = 2; d = 1;
        }
    }
  else if (npd_equal_floats (B->representative->y, C->representative->y))
    {
      if (B->representative->y < D->representative->y)
        {
          /* _ _
              |  */
          a = 2; b = 3; c = 1; d = 0;
        }
      else
        {
          /* _|_ */
          a = 1; b = 0; c = 2; d = 3;
        }
    }

  NPD_NEW_OVERLAPPING_POINTS(new[0], 2, center->points[a], center->points[b], NULL, NULL)
  NPD_NEW_OVERLAPPING_POINTS(new[1], 1, center->points[c], NULL, NULL, NULL)
  NPD_NEW_OVERLAPPING_POINTS(new[2], 1, center->points[d], NULL, NULL, NULL)

  g_hash_table_insert (changed_ops, center, &new[0]);
  g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[1]));
  g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[2]));
}

void
npd_cut_edges (NPDHiddenModel *hm,
               GHashTable *edges)
{
  GHashTableIter iter;
  gpointer key, value;
  GSList *ops = NULL;
  NPDOverlappingPoints *neighbors[4], *op;
  gint i;
  GHashTable *changed_ops = g_hash_table_new (g_direct_hash, g_direct_equal);

  g_hash_table_iter_init (&iter, edges);
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      gint num_of_neighbors = 0;
      op = key;
      ops = (GSList*) value;
      num_of_neighbors = g_slist_length (ops);

      npd_sort_overlapping_points (op);
      
      for (i = 0; i < num_of_neighbors; i++)
        {
          neighbors[i] = g_slist_nth_data (ops, i);
        }
      
      if (num_of_neighbors == 1 && op->num_of_points == 2)
        {
          NPDOverlappingPoints *new = g_new (NPDOverlappingPoints, 2);
          NPD_NEW_OVERLAPPING_POINTS(new[0], 1, op->points[0], NULL, NULL, NULL)
          NPD_NEW_OVERLAPPING_POINTS(new[1], 1, op->points[1], NULL, NULL, NULL)

          g_hash_table_insert (changed_ops, op, &new[0]);
          g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[1]));
        }
      else
      if (num_of_neighbors == 2)
        {
          npd_handle_corner_and_segment (op,
                                         neighbors[0],
                                         neighbors[1],
                                         changed_ops);
        }
      else
      if (num_of_neighbors == 3)
        {
          npd_handle_3_neighbors (op,
                                  neighbors[0],
                                  neighbors[1],
                                  neighbors[2],
                                  changed_ops);
        }
      else
      if (num_of_neighbors == 4)
        {
          NPDOverlappingPoints *new = g_new (NPDOverlappingPoints, 4);
          NPD_NEW_OVERLAPPING_POINTS(new[0], 1, op->points[0], NULL, NULL, NULL)
          NPD_NEW_OVERLAPPING_POINTS(new[1], 1, op->points[1], NULL, NULL, NULL)
          NPD_NEW_OVERLAPPING_POINTS(new[2], 1, op->points[2], NULL, NULL, NULL)
          NPD_NEW_OVERLAPPING_POINTS(new[3], 1, op->points[3], NULL, NULL, NULL)

          g_hash_table_insert (changed_ops, op, &new[0]);
          g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[1]));
          g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[2]));
          g_hash_table_insert (changed_ops, NULL, g_slist_append (g_hash_table_lookup (changed_ops, NULL), &new[3]));
        }
    }
  
  {
  /* recreate overlapping points */
  GHashTableIter iter;
  gpointer key, value;
  GSList *ops;
  NPDOverlappingPoints *op;
  gint num_of_ops, i;

  g_hash_table_iter_init (&iter, changed_ops);
  while (g_hash_table_iter_next (&iter, &key, &value))
  {
    if (key != NULL)
      {
        NPDOverlappingPoints *old_op = key, *new_op = value;
//        g_free (old_op->points);
        *old_op = *new_op;
        /* TODO needs freeing */
      }
  }

  ops = g_hash_table_lookup (changed_ops, NULL);
  printf("null\n");
  num_of_ops = hm->num_of_overlapping_points;

  hm->num_of_overlapping_points += g_slist_length (ops);
  hm->list_of_overlapping_points = g_renew (NPDOverlappingPoints, hm->list_of_overlapping_points, hm->num_of_overlapping_points);

  i = 0;
  while (ops != NULL)
    {
      op = ops->data;
      hm->list_of_overlapping_points[num_of_ops + i] = *op;
      i++;

      ops = g_slist_next (ops);
      /* TODO needs freeing */
    }
  }

/*
  {
  GPtrArray new_ops = g_ptr_array_new ();
  GPtrArray new_ref_bones = g_ptr_array_new ();
  GPtrArray new_cur_bones = g_ptr_array_new ();
  // remove all empty squares 
  for (i = 0; i < hm->reference_bones; i++)
    {
      NPDBone *ref_bone = &hm->reference_bones[i];
      gboolean empty_bone = TRUE;
      
      for (j = 0; j < ref_bone->num_of_points; j++)
        {
          NPDPoint *p = &ref_bone->points[j];
          
          if (p->overlapping_points->num_of_points > 1)
            {
              empty_bone = FALSE;
              break;
            }
        }
      
      if (empty_bone)
        {
          g_ptr_array_add (new_ref_bones, ref_bone);
        }
    }
  }*/
}