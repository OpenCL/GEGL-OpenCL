/* This file is an image processing operation for GEGL
 *
 * seamless-clone.c
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

#ifdef GEGL_CHANT_PROPERTIES
#else

#define GEGL_CHANT_TYPE_COMPOSER
#define GEGL_CHANT_C_FILE       "seamless-clone.c"

#include "config.h"
#include <glib/gi18n-lib.h>
#include "gegl-chant.h"

#include <cairo.h>

#include <stdio.h> /* TODO: get rid of this debugging way! */

#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"
#include "seamless-clone.h"

static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *region)
{
  GeglRectangle result;

  if (g_strcmp0 (input_pad, "input"))
    result = *gegl_operation_source_get_bounding_box (operation, "input");
  else if (g_strcmp0 (input_pad, "aux"))
    result = *gegl_operation_source_get_bounding_box (operation, "aux");
  else
    g_assert_not_reached ();

  printf ("Input \"%s\" size is:\n", input_pad);
  gegl_rectangle_dump (&result);

  return result;
}

static void
prepare (GeglOperation *operation)
{
  Babl *format = babl_format ("RGBA float");

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "aux",    format);
  gegl_operation_set_format (operation, "output", format);
}

/*******************************************************************/
/******************** Part 2 - Mesh rendering **********************/
/*******************************************************************/

/*
 * Store the bounds of the mesh inside mesh_bounds
 */
static cairo_pattern_t*
gimp_operation_seamless_clone_make_fine_mesh (GPtrArray     *ptList,
                                              GeglBuffer    *aux,
                                              GeglBuffer    *input,
                                              GeglRectangle *mesh_bounds)
{
  Babl *format = babl_format("RGB float");
  gfloat dest[3], aux_c[3], input_c[3];
  GList *X = NULL;   /* List of P2tRPoint */
  GList *XEs = NULL; /* List of P2tREdge */
  gint i, N = ptList->len;

  gint min_x = G_MAXINT, max_x = -G_MAXINT, min_y = G_MAXINT, max_y = -G_MAXINT;

  GList *tmpPts = NULL, *tris = NULL;

  printf ("Making mesh from %d points!\n", ptList->len);

  for (i = 0; i < N; i++)
    {
      SPoint *pt = (SPoint*) g_ptr_array_index (ptList, i);
      min_x = MIN (pt->x, min_x);
      min_y = MIN (pt->y, min_y);
      max_x = MAX (pt->x, max_x);
      max_y = MAX (pt->y, max_y);
      /* No one should care if the points are given in reverse order */
      X = g_list_prepend (X, p2tr_point_new (pt->x, pt->y));
    }

  mesh_bounds->x = min_x;
  mesh_bounds->y = min_y;
  mesh_bounds->width = max_x - min_x;
  mesh_bounds->height = max_y - min_y;

  GList *liter = X;
  while (liter != NULL)
    {
      P2tREdge *E = p2tr_edge_new ((P2tRPoint*)(liter->data), (P2tRPoint*)(g_list_cyclic_next(X,liter)->data));
      XEs = g_list_prepend (XEs, E);
      p2tr_edge_set_constrained (E, TRUE);
      liter = liter->next;
    }

  printf ("Edges %p made, now just triangulate!\n", XEs);
  P2tRTriangulation *T = p2tr_triangulate (X);
  printf ("Triangulation %p made! %d triangles, %d(/2) edges, %d points\n", T, g_hash_table_size (T->tris),g_hash_table_size (T->edges),g_hash_table_size (T->pts));

  DelaunayTerminator (T,XEs,M_PI/7,p2tr_false_delta);
  printf ("Terminated!\n");

  P2trHashSetIter iter;
  P2tRTriangle *t;
  p2tr_hash_set_iter_init (&iter, T->tris);
  cairo_pattern_t *mesh = cairo_pattern_create_mesh ();

  P2tRHashSet  *edgePts;
  GHashTable   *sampling;

  ComputeSampling (T, &edgePts, &sampling);


#define GetPtColor(pt,dest)                    \
do {                                           \
  if (p2tr_hash_set_contains (edgePts, (pt)))  \
    {                                          \
      gegl_buffer_sample (aux, (pt)->x, (pt)->y, NULL, aux_c, format, GEGL_INTERPOLATION_NEAREST); \
      gegl_buffer_sample (input, (pt)->x, (pt)->y, NULL, input_c, format, GEGL_INTERPOLATION_NEAREST); \
      (dest)[0] = TO_CAIRO(input_c[0] - aux_c[0]);         \
      (dest)[1] = TO_CAIRO(input_c[1] - aux_c[1]);         \
      (dest)[2] = TO_CAIRO(input_c[2] - aux_c[2]);         \
    }                                          \
  else                                         \
    ComputeInnerSample ((pt), g_hash_table_lookup (sampling, (pt)), input, aux, (dest)); \
} while (FALSE)

  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&t))
    {
      p2tr_assert_and_explain (t != NULL, "NULL triangle found!\n");
      p2tr_assert_and_explain (t->edges[0] != NULL && t->edges[1] != NULL && t->edges[2] != NULL,
                               "Removed triangle found!\n");


      P2tRPoint* tpt;
      cairo_mesh_pattern_begin_patch (mesh);

      tpt = p2tr_edge_get_start (t->edges[0]);
      cairo_mesh_pattern_move_to (mesh, tpt->x, tpt->y);

      tpt = p2tr_edge_get_start (t->edges[1]);
      cairo_mesh_pattern_line_to (mesh, tpt->x, tpt->y);

      tpt = p2tr_edge_get_start (t->edges[2]);
      cairo_mesh_pattern_line_to (mesh, tpt->x, tpt->y);

//      tpt = p2t_triangle_get_point (triangle_index(tris,i), 0);
//      cairo_mesh_pattern_line_to (mesh, tpt->x, tpt->y);

      tpt = p2tr_edge_get_start (t->edges[0]);
      GetPtColor (tpt,dest);
      cairo_mesh_pattern_set_corner_color_rgb (mesh, 0, dest[2], dest[1], dest[0]);

      tpt = p2tr_edge_get_start (t->edges[1]);
      GetPtColor (tpt,dest);
      cairo_mesh_pattern_set_corner_color_rgb (mesh, 1, dest[2], dest[1], dest[0]);

      tpt = p2tr_edge_get_start (t->edges[2]);
      GetPtColor (tpt,dest);
      cairo_mesh_pattern_set_corner_color_rgb (mesh, 2, dest[2], dest[1], dest[0]);

      cairo_mesh_pattern_end_patch (mesh);
      }
  return mesh;
}

static void
render_outline (GPtrArray       *pts,
                cairo_t         *cr)
{
  SPoint *pt;
  gint i;

  if (pts->len <= 0)
    return;

  pt = (SPoint*) g_ptr_array_index (pts, 0);

  cairo_move_to (cr, pt->x, pt->y);

  for (i = 1; i < pts->len; i++)
    {
      pt = (SPoint*) g_ptr_array_index (pts, i);
      cairo_line_to (cr, pt->x, pt->y);
    }

  cairo_close_path (cr);
}

/*******************************************************************/
/********************** Part 3 - Cairo Utils ***********************/
/*******************************************************************/
static cairo_surface_t *
surface_for_rect (const GeglRectangle *roi)
{
  guchar *data;

  data = g_new0 (guchar, roi->width * roi->height * 4);

  return cairo_image_surface_create_for_data (data,
                                              CAIRO_FORMAT_ARGB32,
                                              roi->width,
                                              roi->height,
                                              roi->width * 4);
}

static void
commit_and_free_surface (GeglBuffer          *output,
                         GeglBuffer          *aux,
                         const GeglRectangle *roi,
                         cairo_surface_t     *surface)
{
  gint i, j;
  Babl *format = babl_format ("RGBA u8");
  guchar *data = cairo_image_surface_get_data (surface);
  guchar bg[3];

  g_assert (cairo_image_surface_get_width (surface) == roi->width);
  g_assert (cairo_image_surface_get_height (surface) == roi->height);
  g_assert (cairo_image_surface_get_stride (surface) == roi->width * 4);

  for (i = 0; i < roi->height; i++)
    for (j = 0; j < roi->width; j++)
      {
        gint off = (i * roi->width + j) * 4;
        gegl_buffer_sample (aux, roi->x + j, roi->y + i, NULL, bg, format, GEGL_INTERPOLATION_NEAREST);

        data[off++] = (gint)CLAMP(bg[0] + FROM_CAIRO(data[off]), 0, 255);
        data[off++] = (gint)CLAMP(bg[1] + FROM_CAIRO(data[off]), 0, 255);
        data[off++] = (gint)CLAMP(bg[2] + FROM_CAIRO(data[off]), 0, 255);
        data[off] = (data[off] >= 255) ? 255 : 0;
      }


  gegl_buffer_set (output, roi, format, data,
                   GEGL_AUTO_ROWSTRIDE);

  cairo_surface_destroy (surface);
//  g_free (data);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *aux,
         GeglBuffer          *output,
         const GeglRectangle *result)
{
  GPtrArray *ptList;
  gfloat    *aux_raw;

  // GeglRectangle input_rect = *gegl_buffer_get_extent (input);
  GeglRectangle aux_rect   = *gegl_operation_source_get_bounding_box (operation, "aux");

  cairo_surface_t *out_surf;
  cairo_t         *cr;

  printf ("The aux_rect is:\n");
  gegl_rectangle_dump (&aux_rect);

  aux_raw = g_new (gfloat, 4 * aux_rect.width * aux_rect.height);

  gegl_buffer_get (aux, 1.0, &aux_rect, babl_format("RGBA float"), aux_raw, GEGL_AUTO_ROWSTRIDE);

  ptList = outline_find_ccw (&aux_rect, aux_raw);

  g_free (aux_raw);
  aux_raw = NULL;

  printf ("The result is:\n");
  gegl_rectangle_dump (result);

  out_surf = surface_for_rect (result);
  cr = cairo_create (out_surf);

  GeglRectangle mesh_bounds;
  cairo_pattern_t *mesh = gimp_operation_seamless_clone_make_fine_mesh (ptList, aux, input, &mesh_bounds);
  cairo_set_source (cr, mesh);
  render_outline (ptList, cr);
  cairo_fill_preserve (cr);
  
  cairo_pattern_destroy (mesh);
  cairo_destroy (cr);
  commit_and_free_surface (output, aux, result, out_surf);

  g_free (aux_raw);
  g_ptr_array_free (ptList, TRUE);

  return  TRUE;
}

static void
gegl_chant_class_init (GeglChantClass *klass)
{
  GeglOperationClass         *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationComposerClass *composer_class  = GEGL_OPERATION_COMPOSER_CLASS (klass);

  operation_class->prepare     = prepare;
  operation_class->name        = "gegl:seamless-clone";
  operation_class->categories  = "blend";
  operation_class->description = "Seamless cloning operation";
  operation_class->get_required_for_output = get_required_for_output;

  composer_class->process      = process;
}


#endif
