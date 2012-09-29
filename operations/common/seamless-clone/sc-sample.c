/* This file is an image processing operation for GEGL
 *
 * make-mesh.c
 * Copyright (C) 2011 Barak Itkin <lightningismyname@gmail.com>
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
 */

/* This file is part 2 of 3 in the seamless clone operation. The logic in this
 * file takes an outline as returned from find-outline.c, and makes a fine mesh
 * out of it. After generating the mesh, the list of sample points for each
 * inner point of the mesh will be defined.
 */

#include <glib.h>
#include <poly2tri-c/refine/refine.h>

#include "sc-outline.h"
#include "sc-sample.h"

#define g_ptr_array_index_cyclic(array,index_) g_ptr_array_index(array,(index_)%((array)->len))

#define SC_SAMPLE_BASE_POINT_COUNT 16

/* This won't add the point in the second index, to allow avoiding
 * insertion of a points twice from two adjacent segments. The caller
 * must do that
 */
static void
sc_compute_sample_list_part (ScOutline     *outline,
                             gint           index1,
                             gint           index2,
                             gdouble        Px,
                             gdouble        Py,
                             ScSampleList  *sl,
                             gint           k)
{
  GPtrArray *real = (GPtrArray*) outline;
  
  ScPoint *pt1 = g_ptr_array_index_cyclic (real, index1);
  ScPoint *pt2 = g_ptr_array_index_cyclic (real, index2);

  /* Compute the angle pt1-x-pt2 */
  gdouble dx1 = Px - pt1->x, dy1 = Py - pt1->y;
  gdouble dx2 = Px - pt2->x, dy2 = Py - pt2->y;
  gdouble norm1 = sqrt (dx1 * dx1 + dy1 * dy1);
  gdouble norm2 = sqrt (dx2 * dx2 + dy2 * dy2);
  gdouble angle = acos ((dx1 * dx2 + dy1 * dy2) / (norm1 * norm2));

  gint d = index2 - index1;

  gdouble edist = real->len / (SC_SAMPLE_BASE_POINT_COUNT * pow (2.5, k));
  gdouble eang = 0.75 * pow (0.8, k);
  gboolean needsMore = !(norm1 > edist && norm2 > edist && angle < eang);

  /* Check if inserting more would help, and if there are actually
   * points in the middle. */
   
  if (!needsMore || d <= 1)
    {
      g_ptr_array_add (sl->points, pt1);
      return;
    }
  else
    {
      gint index12 = (index1 + index2) / 2;
      sc_compute_sample_list_part (outline, index1, index12, Px, Py, sl, k + 1);
      sc_compute_sample_list_part (outline, index12, index2, Px, Py, sl, k + 1);
      return;
    }
}

static void
sc_compute_sample_list_weights (gdouble        Px,
                                gdouble        Py,
                                ScSampleList  *sl)
{
  gint N = sl->points->len;
  gdouble *tan_as_half = g_new (gdouble, N);
  gdouble *norms       = g_new (gdouble, N);

  gint i;

  gdouble weightTemp;

  sl->total_weight = 0;

  for (i = 0; i < N; i++)
    {
      ScPoint *pt1 = g_ptr_array_index_cyclic (sl->points, i);
      ScPoint *pt2 = g_ptr_array_index_cyclic (sl->points, i + 1);

      gdouble dx1 = Px - pt1->x, dy1 = Py - pt1->y;
      gdouble dx2 = Px - pt2->x, dy2 = Py - pt2->y;
      gdouble norm1 = sqrt (dx1 * dx1 + dy1 * dy1);
      gdouble norm2 = sqrt (dx2 * dx2 + dy2 * dy2);
      gdouble ang, temp;

      norms[i] = norm1;

      /* Did the point match one of the outline points? If so, convert
       * the sample list to be made only of that outline point.
       * This shouldn't happen since we give a direct sample list to
       * boundry points, but this is a backup and also in case that due
       * to small distances the norm came out zero
       */
      if (norm1 == 0)
        {
          gdouble temp = 1;
          g_ptr_array_remove_range (sl->points, 0, N);
          /* No weights yet so nothing to remove */
          
          g_ptr_array_add (sl->points, pt1);
          g_array_append_val (sl->weights, temp);
          sl->total_weight = 1;
          return;
        }

      temp = (dx1 * dx2 + dy1 * dy2) / (norm1 * norm2);

      if (temp <= 1 && temp >= -1)
        {
          /* Result is in the range of 0 to PI */
          ang = acos (temp);
        }
      else
        {
           ang = 0;
        }
      
      tan_as_half[i] = tan (ang / 2);
      tan_as_half[i] = ABS (tan_as_half[i]);
    }

  weightTemp = (tan_as_half[0] + tan_as_half[N-1]) / norms[0];
  g_array_append_val (sl->weights, weightTemp);
    
  for (i = 1; i < N; i++)
    {
      weightTemp = (tan_as_half[i - 1] + tan_as_half[i % N]) / pow (norms[i % N], 2);
      sl->total_weight += weightTemp;
      g_array_append_val (sl->weights, weightTemp);
    }
}

ScSampleList*
sc_sample_list_compute (ScOutline     *outline,
                        gdouble        Px,
                        gdouble        Py)
{
  ScSampleList *sl = g_slice_new (ScSampleList);
  GPtrArray *real = (GPtrArray*) outline;
  gint i;

  sl->direct_sample = FALSE;
  sl->points = g_ptr_array_new ();
  sl->weights = g_array_new (FALSE, TRUE, sizeof (gdouble));

  if (real->len <= SC_SAMPLE_BASE_POINT_COUNT)
    {
      for (i = 0; i < real->len; i++)
        g_ptr_array_add (sl->points, g_ptr_array_index (real, i));
    }
  else
    {
      for (i = 0; i < SC_SAMPLE_BASE_POINT_COUNT; i++)
        {
          gint index1 = i * real->len / SC_SAMPLE_BASE_POINT_COUNT;
          gint index2 = (i + 1) * real->len / SC_SAMPLE_BASE_POINT_COUNT;

          sc_compute_sample_list_part (outline, index1, index2, Px, Py, sl, 0);
        }
    }

  sc_compute_sample_list_weights (Px, Py, sl);

  return sl;
}

ScSampleList*
sc_sample_list_direct (void)
{
  ScSampleList *sl = g_slice_new (ScSampleList);
  sl->direct_sample = TRUE;
  sl->points = NULL;
  sl->weights = NULL;
  sl->total_weight = 0;
  return sl;
}

void
sc_sample_list_free (ScSampleList *self)
{
  if (! self->direct_sample)
    {
      g_ptr_array_free (self->points, TRUE);
      g_array_free (self->weights, TRUE);
    }
  else
    {
      g_assert (self->points == NULL);
      g_assert (self->weights == NULL);
    }
  g_slice_free (ScSampleList, self);
}

ScMeshSampling*
sc_mesh_sampling_compute (ScOutline *outline,
                          P2trMesh  *mesh)
{
  GHashTable *pt2sample = g_hash_table_new (g_direct_hash, g_direct_equal);
  P2trPoint  *pt = NULL;
  P2trHashSetIter iter;

  p2tr_hash_set_iter_init (&iter, mesh->points);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*) &pt))
    {
      ScSampleList *sl;
      if (p2tr_point_is_fully_in_domain (pt))
        sl = sc_sample_list_compute (outline, pt->c.x, pt->c.y);
      else
        sl = sc_sample_list_direct ();
      g_hash_table_insert (pt2sample, pt, sl);
    }

  return pt2sample;
}

static void
sc_mesh_sampling_entry_free_hfunc (gpointer point,
                                   gpointer sampling_list,
                                   gpointer unused)
{
  /* Unref the point returned from triangulation_get_points */
  p2tr_point_unref ((P2trPoint*)point);
  /* Free the sampling list */
  sc_sample_list_free ((ScSampleList*)sampling_list);
}

void
sc_mesh_sampling_free (ScMeshSampling *self)
{
  GHashTable     *real = (GHashTable*) self;
  g_hash_table_foreach (real, sc_mesh_sampling_entry_free_hfunc, NULL);
  g_hash_table_destroy (real);
}

