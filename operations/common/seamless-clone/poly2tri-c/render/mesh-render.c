#include <glib.h>
#include <stdlib.h>
#include <stdio.h>
#include "../refine/triangulation.h"
#include "mesh-render.h"

/* Most computations using the Barycentric Coordinates are Based on
 * http://www.blackpawn.com/texts/pointinpoly/default.html */

/* This function is simply to make sure the code is consitant */
void
p2tr_triangle_barycentric_get_points (P2tRTriangle  *self,
                                      P2tRPoint    **A,
                                      P2tRPoint    **B,
                                      P2tRPoint    **C)
{
  *A = p2tr_edge_get_end (self->edges[2]);
  *B = p2tr_edge_get_end (self->edges[0]);
  *C = p2tr_edge_get_end (self->edges[1]);
}

#define USE_BARYCENTRIC(u,v,A,B,C) ((A) + (v)*((B)-(A)) + (u)*((C)-(A)))

gboolean
p2tr_triangle_compute_barycentric_coords (P2tRTriangle *tr,
                                          gdouble       Px,
                                          gdouble       Py,
                                          gdouble      *u_out,
                                          gdouble      *v_out)
{
  P2tRPoint *A, *B, *C;

  gdouble u, v;
  gdouble v0x, v0y, v1x, v1y, v2x, v2y;
  gdouble dot00, dot01, dot02, dot11, dot12;
  gdouble invDenom;

  p2tr_triangle_barycentric_get_points (tr, &A, &B, &C);

  /* v0 = C-A */
  v0x = C->x - A->x;
  v0y = C->y - A->y;
  /* v1 = B-A */
  v1x = B->x - A->x;
  v1y = B->y - A->y;
  /* v2 = P-A */
  v2x = Px - A->x;
  v2y = Py - A->y;

  /* Compute dot products */
  dot00 = v0x * v0x + v0y * v0y;
  dot01 = v0x * v1x + v0y * v1y;
  dot02 = v0x * v2x + v0y * v2y;
  dot11 = v1x * v1x + v1y * v1y;
  dot12 = v1x * v2x + v1y * v2y;

  /* Compute barycentric coordinates */
  invDenom = 1 / (dot00 * dot11 - dot01 * dot01);

  /* P = A + v*(B-A) + u*(C-A) */
  *u_out = u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  *v_out = v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  /* Check if point is in triangle */
  return (u > -EPSILON2) && (v > -EPSILON2) && (u + v < 1 + EPSILON2);

}

/* This function implements box logic to see if a point is contained in a
 * triangles bounding box. This is very useful for cases where there are many
 * triangles to test against a single point, and most of them aren't even near
 * it.
 *
 * Instead of finding the Xmin, Xmax, Ymin, Ymax and checking if the the point
 * is outside, just check if the point is on the SAME SIDE compared to all the
 * points of the triangle.
 * See http://lightningismyname.blogspot.com/2011/08/quickboxa-quick-point-in-triangle-test.html
 */
gboolean
p2tr_triangule_quick_box_test (P2tRTriangle *self,
                               gdouble       Px,
                               gdouble       Py)
{
  P2tRPoint *A = p2tr_edge_get_end (self->edges[2]);
  P2tRPoint *B = p2tr_edge_get_end (self->edges[0]);
  P2tRPoint *C = p2tr_edge_get_end (self->edges[1]);

  register gboolean xPBorder = B->x <= Px;
  register gboolean yPBorder = B->y <= Py;

  return (((A->x <= Px) == xPBorder) && (xPBorder == (C->x <= Px)))
          || (((A->y <= Py) == yPBorder) && (yPBorder == (C->y <= Py)));
}
/**
 * p2tr_triangulation_locate_point2:
 * @T: A triangulation object
 * @X: The point to locate
 * @guess: Some triangle near the point, or NULL if not known.
 *         WARNING! The triangle must be inside the same continuos region as the
 *                  point! If not, this function may return wrong values!
 *
 * Returns: A triangle containing the point, or NULL if the point is outside the
 *          triangulation domain.
 */
P2tRTriangle*
p2tr_triangulation_locate_point2 (P2tRTriangulation *T,
                                  gdouble            Px,
                                  gdouble            Py,
                                  P2tRTriangle      *guess,
                                  gdouble           *u,
                                  gdouble           *v)
{
  if (guess == NULL || ! p2tr_hash_set_contains (T->tris, guess))
    {
      /* If we have nothing, check all the triangles.
       * TODO: This can probably be improved by sampling several triangles at
       * random, picking the closest and using it as a guess.*/
      P2tRTriangle *tr = NULL;
      P2trHashSetIter iter;
      p2tr_hash_set_iter_init (&iter, T->tris);
      while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tr))
        if (p2tr_triangle_compute_barycentric_coords (tr, Px, Py, u, v))
          return tr;
      return NULL;
    }
  else
    {
      /* Maintain a set of checked triangles, and a queue of ones to check.
       * For each triangle in the queue, check if it has the point, and if not
       * then add it's neighbors at the end of the queue. This also gaurantess
       * to some level a search that starts local around the triangles and only
       * goes farther if needed. */
      P2tRHashSet *checked = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
      P2tRTriangle *result = NULL, *current = NULL;
      GQueue tris;
      gint i;

      g_queue_init (&tris);
      g_queue_push_tail (&tris, guess);

      while (! g_queue_is_empty (&tris))
        {
          current = (P2tRTriangle*) g_queue_pop_head (&tris);
          if (p2tr_triangle_compute_barycentric_coords (current, Px, Py, u, v))
            {
              result = current;
              break;
            }
          else for (i = 0; i < 3; i++)
            {
              P2tRTriangle *neighbor = current->edges[i]->mirror->tri;
              if (neighbor != NULL && ! p2tr_hash_set_contains (checked, neighbor))
                g_queue_push_tail (&tris, current->edges[i]->mirror->tri);
            }

          p2tr_hash_set_insert (checked, current);
        }

      /* If the queue is empty, then we have nothing to free. It's struct is
       * allocated directly on the stack and it has nothing dynamic in it. */

      g_hash_table_destroy (checked);
      
      return result;
    }
}

void p2tr_test_point_to_color (P2tRPoint* point, gfloat *dest, gpointer user_data)
{
/*
  GRand* sr = g_rand_new_with_seed ((*((guchar*)&point->x)) ^ (*((guchar*)&point->y)));
  gfloat temp;

  temp = (gfloat) g_rand_double (sr);
  dest[0] = ABS (temp);

  temp = (gfloat) g_rand_double (sr);
  dest[1] = ABS (temp);

  temp = (gfloat) g_rand_double (sr);
  dest[2] = ABS (temp);

  dest[3] = 1;
*/
  dest[0] = 0;
  dest[1] = 0.5;
  dest[2] = 1;
}

void
p2tr_mesh_render_scanline (P2tRTriangulation    *T,
                           gfloat               *dest,
                           P2tRImageConfig      *config,
                           P2tRPointToColorFunc  pt2col,
                           gpointer              pt2col_user_data)
{
  gdouble *uvbuf = g_new (gdouble, 2 * config->x_samples * config->y_samples), *Puv = uvbuf;
  P2tRTriangle **tribuf = g_new (P2tRTriangle*, config->x_samples * config->y_samples), **Ptri = tribuf;
  P2tRTriangle *tr_prev = NULL, *tr_now;

  gint x, y;

  tribuf[0] = p2tr_triangulation_locate_point2 (T, config->min_x, config->min_y, NULL, &uvbuf[0], &uvbuf[1]);

  for (x = 0; x < config->x_samples; x++)
    for (y = 0; y < config->y_samples; y++)
    {
      gdouble Px = config->min_x + x * config->step_x;
      gdouble Py = config->min_y + y * config->step_y;
      *Ptri++ = p2tr_triangulation_locate_point2 (T, Px, Py, *Ptri, &Puv[0], &Puv[1]);
      Puv+=2;
    }

  Puv = uvbuf;
  Ptri = tribuf;

  P2tRPoint *A = NULL, *B = NULL, *C = NULL;

  gfloat *col =  g_new (gfloat, config->cpp);
  gfloat *colA = g_new (gfloat, config->cpp);
  gfloat *colB = g_new (gfloat, config->cpp);
  gfloat *colC = g_new (gfloat, config->cpp);

  gfloat *pixel = dest;

  GTimer *timer = g_timer_new ();
  g_timer_start (timer);

  for (x = 0; x < config->x_samples; x++)
    for (y = 0; y < config->y_samples; y++)
    {
        gdouble u = Puv[0], v = Puv[1];
        tr_now = *Ptri++;
        uvbuf++;
        
        if (tr_now == NULL)
          {
            pixel[3] = 0;
            pixel += 4;
          }
        else
          {
            if (tr_now != tr_prev)
              {
                p2tr_triangle_barycentric_get_points (tr_now, &A, &B, &C);
                pt2col (A, colA, pt2col_user_data);
                pt2col (B, colB, pt2col_user_data);
                pt2col (C, colC, pt2col_user_data);
                tr_now = tr_prev;
              }

            *pixel++ = USE_BARYCENTRIC (u,v,colA[0],colB[0],colC[0]);
            *pixel++ = USE_BARYCENTRIC (u,v,colA[1],colB[1],colC[1]);
            *pixel++ = USE_BARYCENTRIC (u,v,colA[2],colB[2],colC[2]);
            *pixel++ = 1;
          }
    }
  g_timer_stop (timer);
  p2tr_debug ("Mesh rendering took %f seconds\n", g_timer_elapsed (timer, NULL));
}

void
p2tr_write_ppm (FILE            *f,
                gfloat          *dest,
                P2tRImageConfig *config)
{
  gint x, y;
  fprintf (f, "P3\n");
  fprintf (f, "%d %d\n", config->x_samples, config->y_samples);
  fprintf (f, "255\n");

  gfloat *pixel = dest;

  for (y = 0; y < config->y_samples; y++)
    {
      for (x = 0; x < config->x_samples; x++)
        {
          if (pixel[3] <= 0.5)
            fprintf (f, "  0   0   0");
          else
            fprintf (f, "%3d %3d %3d", (guchar)(pixel[0] * 255), (guchar)(pixel[1] * 255), (guchar)(pixel[2] * 255));

          if (x != config->x_samples - 1)
            fprintf (f, "   ");

          pixel += 4;
        }
      fprintf (f, "\n");
    }
}