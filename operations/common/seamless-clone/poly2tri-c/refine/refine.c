#include <stdlib.h>

#include "triangulation.h"

/* Place holder function. In the future, this should be replaced by some search
 * tree, such as a quad tree.
 */
P2tRTriangle *
p2tr_triangulation_locate_point (P2tRTriangulation *T, P2tRPoint *X)
{
  P2trHashSetIter iter;
  P2tRTriangle *result;

  p2tr_hash_set_iter_init (&iter, T->tris);

  while (p2tr_hash_set_iter_next(&iter, (gpointer*)&result))
    {
      if (p2tr_triangle_contains_pt (result, X))
          return result;
    }

  return NULL;
}

gboolean
p2tr_points_are_same (P2tRPoint *pt1, P2tRPoint *pt2)
{
  return p2tr_math_edge_len_sq (pt1->x, pt1->y, pt2->x, pt2->y) < EPSILON2;
}

/* As the algorithm paper states, an edge can only be encroached by the
 * point opposite to it in one of the triangles it's a part of.
 * We can deduce from that that a point may only encroach upon edges that form
 * the triangle in/on which it is located.
 * which are opposite to it in some triangle, and that it may encroach free
 * points (points which are not a part of any triangles)
 * We can find the list of triangles in which a point it a part, by iterating
 * over the .tri property of the out going edges.
 * NOTE: If there is a triangle formed by edges, but without a real triangle
 *       there, then we are dealing with the edges of the domain which have to
 *       be constrained! Therefore, we can ignore these edges and not return them
 */
P2tRHashSet *
p2tr_triangulation_get_encroaches_upon (P2tRPoint *pt, P2tRTriangulation *T)
{
  GList *iter;
  P2tRTriangle *Tri = p2tr_triangulation_locate_point (T, pt);
  P2tRPoint *p;
  P2tRHashSet *E = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
  gint i;

  if (Tri == NULL)
    return E;

  for (i = 0; i < 3; i++)
    {
      /* If the point is the same as any of the points of the triangle, then it
       * is inside the diametral circle of all the edges that connect to that
       * point. In that case, add all the edges of the point to the list of
       * encroahced edges.
       * TODO: check if you can break after finding one in this case
       */
      if (p2tr_points_are_same (pt, p = p2tr_edge_get_end (Tri->edges[i])))
        foreach (iter, p->edges)
          p2tr_hash_set_insert (E, (P2tREdge*) iter->data);

      /* Now, check if any of the edges contains the point inside it's diametral
       * circle */
      if (p2tr_edge_diametral_circle_contains (Tri->edges[i], pt))
        p2tr_hash_set_insert (E, Tri->edges[i]);

      /* TODO: add a check for the case where the point is actually on one of the
       *       edges */
    }

    return E;
}


/**
 * Split an edge at a given point. If the point is at the same location as one
 * of the sides, the edge will not be split in order to avoid zero lengthed edges.
 *
 * @param e the edge to split
 * @param pt the point to add
 * @param T the triangulation to maintain
 * @param patchHoles Whethe the holes created by removing the original edge
 *        should be patched
 * @param dest An array where the results of the split are to be stored. The
 *        array should be of size 2, and it will contain one edge if no split
 *        occured or two edges if a split did in fact occur.
 *
 * @return The amount of edges resulting from the split - 1 if no split, 2 if a
 *         split occured
 */
gint
p2tr_split_edge (P2tREdge          *e,
                 P2tRPoint         *pt,
                 P2tRTriangulation *T,
                 gboolean           patchHoles,
                 P2tREdge          *dest[2],
                 gboolean           flipFix)
{
  /*       A
   *      / \
   *     / | \
   *    /     \
   *   /   |   \
   *  /         \
   * S---- pt----E   ===> This is edge e, and pt is the point we insert
   * \     |     /
   *  \         /
   *   \   |   /
   *    \     /
   *     \ | /
   *      \ /
   *       B
   *
   * Each of these edges may need flip fixing!
   */
    P2tRPoint *S = p2tr_edge_get_start (e);
    P2tRPoint *E = p2tr_edge_get_end (e);
    P2tRPoint *A = NULL, *B = NULL;

    P2tRTriangle *SptA = NULL, *ptEA = NULL, *SptB = NULL, *ptEB = NULL;

    gboolean constrained = e->constrained;
    /* TODO: T remove me */
    p2tr_validate_edge (e);
    
    P2tREdge *Spt, *ptE;

    p2tr_debug ("Checking if can split\n");

    if (p2tr_points_are_same (pt, S) || p2tr_points_are_same (pt, E))
      {
        dest[0] = e;
        dest[1] = NULL;
        return 1;
      }

    p2tr_debug ("Splitting!\nAdding (%f,%f) between (%f,%f) and (%f,%f)\n", pt->x,pt->y,S->x,S->y,E->x,E->y);

    if (patchHoles)
      {
        if (e->tri != NULL)
          A = p2tr_triangle_opposite_point (e->tri, e);

        if (e->mirror->tri != NULL)
          B = p2tr_triangle_opposite_point (e->mirror->tri, e);
      }

    p2tr_edge_remove (e, T);

    Spt = p2tr_edge_new (S, pt);
    ptE = p2tr_edge_new (pt,E);

    p2tr_edge_set_constrained (Spt, constrained);
    p2tr_edge_set_constrained (ptE, constrained);

    if (patchHoles)
      {
        if (A != NULL)
          {
            P2tREdge *ptA = p2tr_point_edge_to (pt, A);
            P2tRTriangle *SptA = p2tr_triangle_new (Spt, ptA, p2tr_point_edge_to (A, S), T);
            P2tRTriangle *ptEA = p2tr_triangle_new (ptE, p2tr_point_edge_to (E, A), ptA->mirror,T);
          }

        if (B != NULL)
          {
            P2tREdge* ptB = p2tr_point_edge_to (pt, B);
            P2tRTriangle *SptB = p2tr_triangle_new (Spt, ptB, p2tr_point_edge_to (B, S), T);
            P2tRTriangle *ptEB = p2tr_triangle_new (ptE, p2tr_point_edge_to (E, B), ptB->mirror,T);
          }
      }

    if (flipFix)
      {
        P2tREdge *temp;

        /* We do not want the edges resulting from the split to be flipped! */
        p2tr_edge_set_delaunay (Spt, TRUE);
        p2tr_edge_set_delaunay (ptE, TRUE);

        if (A != NULL)
          {
            p2tr_triangulation_legalize (T, SptA);
            p2tr_triangulation_legalize (T, ptEA);
          }
        if (B != NULL)
          {
            p2tr_triangulation_legalize (T, SptB);
            p2tr_triangulation_legalize (T, ptEB);
          }

        p2tr_edge_set_delaunay (Spt, FALSE);
        p2tr_edge_set_delaunay (ptE, FALSE);
      }

    // Error - if the triangles were remove, unreffing them would cause problems
    // So for now, I'm commenting these out
    // TODO: FIXME!
//    p2tr_triangle_unref (SptA);
//    p2tr_triangle_unref (ptEA);

//    p2tr_triangle_unref (SptB);
//    p2tr_triangle_unref (ptEB);

    if (dest != NULL)
      {
        /* We shouldn't decrese the reference to Spt and ptE, since it's 1
         * right now, and we pass the reference to them back to the caller through
         * dest */
        dest[0] = Spt;
        dest[1] = ptE;
      }
    else
      {
        p2tr_edge_unref (Spt);
        p2tr_edge_unref (ptE);
      }
    return 2;
}

/**
 * Insert a point into a triangulation. If it's outside of the triangulation or
 * if it merges with an existing point it will not be inserted. If it's on an
 * existing edge, the edge will be split and then there will be a flipfix to
 * make the triangulation CDT again. If it's inside a triangle, the triangle
 * will be subdivided and flipfixing will be applied to maintain the CDT
 * property.
 *
 * @param pt the point to insert
 * @param T the triangulation to insert into
 *
 * @return TRUE if the point was inserted, FALSE otherwise
 */
gboolean
p2tr_triangulation_insert_pt (P2tRPoint *pt, P2tRTriangulation *T)
{
    p2tr_debug ("@insertPt\n");
    P2tRTriangle *Tri = p2tr_triangulation_locate_point (T, pt);
    
    gint i;
    P2tREdge *temp;

    p2tr_validate_triangulation (T);
    if (Tri == NULL)
      {
        p2tr_debug ("Warning - tried to insert point outside of domain\n");
        return FALSE;
      }

    P2tREdge *ab = Tri->edges[0];
    P2tREdge *bc = Tri->edges[1];
    P2tREdge *ca = Tri->edges[2];

    P2tRPoint *a = p2tr_edge_get_end (ca);
    P2tRPoint *b = p2tr_edge_get_end (ab);
    P2tRPoint *c = p2tr_edge_get_end (bc);

    if (p2tr_points_are_same (pt, a)
        || p2tr_points_are_same (pt, b)
        || p2tr_points_are_same (pt, c))
      {
        p2tr_debug ("Not inserting point on existing point!\n");
        return FALSE;
      }

    p2tr_validate_triangulation (T);
  /* Check if the point is on any of the edges */
  for (i = 0; i < 3; i++)
    {
      P2tRPoint *S = p2tr_edge_get_start (Tri->edges[i]);
      P2tRPoint *E = p2tr_edge_get_end (Tri->edges[i]);

      /* Is the point on the edge? */
      if (p2tr_math_orient2d (pt,S,E) == COLLINEAR)
        {
          gint j;
          gint splitCount;
          P2tREdge *splits[2];

          /* If so, flip the edge */
          p2tr_validate_triangulation (T);
          splitCount = p2tr_split_edge (Tri->edges[i], pt, T, TRUE, splits, TRUE);
          p2tr_validate_triangulation (T);
          /* Then, flipfix each of the resulting edges */
          for (j = 0; j < splitCount; j++)
            {
              p2tr_triangulation_flip_fix (splits[j], T);
              /* We don't use the edge after this point, so unref */
              p2tr_edge_unref (splits[j]);
            }

          /* If the point is on one edge then it's not on any other edge (unless
           * it is in fact one of the triangle vertices but we already fixed
           * that case earlier). So by splitting the edge and flipfixing the
           * result, we are in fact done. */
          return TRUE;
        }
    }

    /* If reached here, then the point was not on any of the edges. So just
     * insert it into the triangle */

    p2tr_validate_triangulation (T);
    p2tr_triangle_subdivide (Tri, pt, T, NULL);
    p2tr_validate_triangulation (T);

    /* IMPORTANT: YOU CAN NOT FLIP AB/BC/CA BECAUSE THEY MAY NOT EXIST!
     * Even if there is an edge from a to b for example, it may not be ab we got
     * earlier! It can be there as a result of different flip fixing which
     * deleted and then created it! So, take the edge returned from has_edge_to
     * (which is the updated one) and flip-fix if it exists.
     */
    if ((temp = p2tr_point_has_edge_to (a,b)) != NULL)
      p2tr_triangulation_flip_fix (temp, T);
    p2tr_validate_triangulation (T);

    if ((temp = p2tr_point_has_edge_to (b,c)) != NULL)
      p2tr_triangulation_flip_fix (temp, T);
    p2tr_validate_triangulation (T);

    if ((temp = p2tr_point_has_edge_to (c,a)) != NULL)
      p2tr_triangulation_flip_fix (temp, T);
    p2tr_validate_triangulation (T);
    
    return TRUE;
}

typedef gboolean (*deltafunc) (P2tRTriangle *);


/*
 * This function returns negative if triangle a is uglier than b, 0 if same, and
 * 1 if b is uglier
 */
gint
ugliness_comparision (gconstpointer a,
                      gconstpointer b,
                      gpointer ignored)
{
  if (a == b)
    return 0; // Fast path exit

  gdouble shortestA = p2tr_triangle_shortest_edge_len ((P2tRTriangle*)a);
  gdouble shortestB = p2tr_triangle_shortest_edge_len ((P2tRTriangle*)b);

  gdouble longestA = p2tr_triangle_longest_edge_len ((P2tRTriangle*)a);
  gdouble longestB = p2tr_triangle_longest_edge_len ((P2tRTriangle*)b);

  gdouble smallestA = p2tr_triangle_smallest_non_seperating_angle ((P2tRTriangle*)a);
  gdouble smallestB = p2tr_triangle_smallest_non_seperating_angle ((P2tRTriangle*)b);

// Bad
//  return (gint) ((smallestA - smallestB) / M_PI) ;//+ (longestA - longestB) / MAX (longestA, longestB);

// Seems Good
  return (gint) (1 - longestA/longestB + ((smallestA - smallestB) / M_PI));

// Not sure?
//  return (gint) (longestB/shortestB - longestA/shortestA);

// Trivial, reasonable
//  return (gint) (smallestA - smallestB);
}


void
NewVertex (P2tRPoint *v, gdouble theta, deltafunc delta,
           GQueue *Qs, GSequence *Qt)
{
  GList *iter;

  p2tr_debug ("NewVertex: After inserting ");
  p2tr_debug_point (v, TRUE);

  foreach (iter, v->edges)
  {
    P2tREdge *ed = (P2tREdge*) iter->data;
    P2tRTriangle *t = ed->tri;

    if (t != NULL)
      {
        p2tr_debug ("NewVertex: Checking tri ");
        p2tr_debug_tri (t, TRUE);

        P2tREdge *e = p2tr_triangle_opposite_edge (t, v);
        if (e->mirror->tri == NULL && p2tr_edge_is_encroached_by (e, v))
          g_queue_push_tail (Qs, e);
        else if (delta (t) || p2tr_triangle_smallest_non_seperating_angle (t) < theta)
          g_sequence_insert_sorted (Qt, t, ugliness_comparision, NULL);
      }
  }
}

void
SplitEncroachedSubsegments (gdouble            theta,
                            deltafunc          delta,
                            P2tRTriangulation *T,
                            GQueue            *Qs,
                            GSequence         *Qt)
{
  while (! g_queue_is_empty (Qs))
    {
      p2tr_debug ("Handling edge\n");
      P2tREdge *s = g_queue_pop_head (Qs);
      gboolean isin = FALSE;

      P2trHashSetIter iter;
      P2tRTriangle *t;
      p2tr_hash_set_iter_init (&iter, T->tris);

      while (p2tr_hash_set_iter_next (&iter, (gpointer*)&t))
        {
          if (t->edges[0] == s || t->edges[1] == s || t->edges[2] == s)
            {
              isin = TRUE;
              break;
            }
        }

      if (isin)
        {
          if (p2tr_edge_len_sq (s) >= 1)
            {
              P2tRPoint *v = p2tr_edge_concentric_center (s);
              P2tREdge *splits[2];
              gint splitCount = p2tr_split_edge (s, v, T, TRUE, splits, TRUE), i;

              for (i = 0; i < splitCount; i++)
                {
                  p2tr_triangulation_flip_fix (splits[i], T);
                }

              NewVertex (v, theta, delta, Qs, Qt);
              for (i = 0; i < splitCount; i++)
                {
                  if (p2tr_edge_is_encroached (splits[i]))
                    g_queue_push_tail (Qs, splits[i]);
                }
            }
        }
    }
}


gboolean
p2tr_false_delta (P2tRTriangle *t)
{
  return FALSE;
}

const gdouble POW2_EPS = 0;
const gdouble LENGTHSQ_EPS = 0;

gboolean
SplitPermitted (P2tREdge *s, gdouble d)
{
    p2tr_debug ("@SplitPermitted\n");
    if (! (p2tr_point_is_in_cluster (p2tr_edge_get_start(s), s) ^ p2tr_point_is_in_cluster (p2tr_edge_get_end(s), s->mirror)))
      {
        p2tr_debug ("Criteria 1\n");
        return TRUE;
      }
    gdouble l = p2tr_edge_len_sq (s);
    gdouble temp = log2 (l) / 2;
    if (ABS (round (temp) - temp) < POW2_EPS)
      {
        p2tr_debug ("Criteria 2\n");
        return TRUE;
      }

    gdouble phi_min;
    GList *S = p2tr_point_get_cluster (p2tr_edge_get_start (s), s, &phi_min);
    GList *s1;

    foreach(s1, S)
    {
      if (p2tr_edge_len_sq ((P2tREdge*)s1->data) < 1 * (1 + LENGTHSQ_EPS))
        {
          p2tr_debug ("Criteria 3\n");
          return TRUE;
        }
    }

    gdouble r_min = sqrt(l) * sin (phi_min / 2);

    if (r_min >= d)
      {
        p2tr_debug ("Criteria 4\n");
        return TRUE;
      }

    p2tr_debug ("Split not permitted\n");
    return FALSE;
}

#define MIN_MAX_EDGE_LEN 0

void
DelaunayTerminator (P2tRTriangulation *T,
                    GList             *XEs,
                    gdouble            theta,
                    deltafunc          delta,
                    int                max_refine_steps)
{
  const gint STEPS = max_refine_steps;
  GSequence *Qt;
  GQueue Qs;

  GList *Liter, Liter2;
  P2trHashSetIter Hiter;

  p2tr_debug("Max refine point count is %d\n", max_refine_steps);
  
  p2tr_validate_triangulation (T);

  P2tRTriangle *t;

  g_queue_init (&Qs);
  Qt = g_sequence_new (NULL);

  if (STEPS == 0)
    return;
  
  foreach (Liter, XEs)
    {
      P2tREdge *s = (P2tREdge*) (Liter->data);
      if (p2tr_edge_is_encroached (s))
        {
          g_queue_push_tail (&Qs, s);
        }
    }

  SplitEncroachedSubsegments(0, p2tr_false_delta,T,&Qs,Qt);

  p2tr_validate_triangulation (T);

  p2tr_hash_set_iter_init (&Hiter, T->tris);
  while (p2tr_hash_set_iter_next (&Hiter, (gpointer*)&t))
    if (delta (t) || p2tr_triangle_smallest_non_seperating_angle (t) < theta)
      g_sequence_insert_sorted (Qt, t, ugliness_comparision, NULL);

  gint STOP = STEPS - 1; /* The above split was a step */

  while (g_sequence_get_length (Qt) > 0 && STOP > 0)
    {
      p2tr_validate_triangulation (T);
      p2tr_debug ("Restarting main loop\n");
      STOP--;

      do
        {
          GSequenceIter *siter = g_sequence_get_begin_iter (Qt);
          t = g_sequence_get (siter);
          g_sequence_remove (siter);
          if (g_sequence_get_length (Qt) == 0) break;
        }
      while (! p2tr_hash_set_contains (T->tris, t));
      if (g_sequence_get_length (Qt) == 0) break;

      if (p2tr_triangle_longest_edge_len (t) < MIN_MAX_EDGE_LEN)
        {
          continue;
        }

      /* Possibly make more efficient by adding an "erased" field */
      if (p2tr_hash_set_contains (T->tris, t))
        {
          P2tRCircle cc;
          P2tRPoint *c;
          P2tRHashSet *E;

          p2tr_debug ("Found a triangle that still needs fixing\n");

          p2tr_triangle_circumcircle (t, &cc);
          c = p2tr_point_new (cc.x,cc.y);

          E = p2tr_triangulation_get_encroaches_upon (c, T);
          p2tr_validate_triangulation (T);

          if (g_hash_table_size (E) == 0)
            {
              p2tr_debug ("It's circumcircle encraoches upon nothing!\n");
              /* Check if the point is in the domain and inserted OK */
              if (p2tr_triangulation_insert_pt (c, T))
                {
                  p2tr_validate_triangulation (T);
                  NewVertex (c,theta,delta,&Qs,Qt);
                  p2tr_validate_triangulation (T);
                }
              else
                {
                  int k;
                  /* In the other cases, split the edge crossing the
                   * line to the point
                   */
                  g_assert (! p2tr_triangle_contains_pt (t, c));
                  P2tRPoint *M = p2tr_triangle_median_pt (t);
                  P2tREdge *MC = p2tr_point_edge_to (M, c);
                  p2tr_validate_triangulation (T);
                  for (k = 0; k < 3; k++)
                    {
                      P2tREdge *e = t->edges[k];
                      if (p2tr_edges_intersect (e, MC))
                        {
                          P2tREdge *splits[2];
                          P2tRPoint *splitPoint = p2tr_edge_concentric_center (e);
                          gint count = p2tr_split_edge (e, splitPoint, T, TRUE, splits, TRUE);
                          p2tr_validate_triangulation (T);
                          gint j;

                          for (j = 0; j < count; j++)
                            {
                              p2tr_triangulation_flip_fix (splits[j], T);
                              p2tr_validate_triangulation (T);
                            }

                          p2tr_point_unref (splitPoint);
                          break;
                        }
                    }
                }
            }
          else
            {
              P2tREdge *s;
              p2tr_debug ("It's circumcircle encraoches %d edges\n", g_hash_table_size (E));
              gdouble d = p2tr_triangle_shortest_edge_len (t);

              p2tr_hash_set_iter_init (&Hiter, E);
              while (p2tr_hash_set_iter_next (&Hiter, (gpointer*)&s))
                {
                  if (delta (t) || SplitPermitted(s,d))
                    {
                      p2tr_debug ("Appending an edge for splitting\n");
                      g_queue_push_tail (&Qs, s);
                    }
                }
              if (! g_queue_is_empty (&Qs))
                {
                  g_sequence_insert_sorted (Qt, t, ugliness_comparision, NULL);
                  p2tr_debug ("Will now try splitting\n");
                  SplitEncroachedSubsegments(theta,delta,T,&Qs,Qt);
                }
            }

          p2tr_point_unref (c);
        }

      // Why not to legalize here:
      // 1. Because it should have no effect if we did maintain the CDT
      // 2. Because if it does have an effect, since legalizations checks only
      //    vialoations with adjacent neighbours, it may simply violate the CDT
      //    property with remote ones!

      // Flip fixing actually adds more triangles that may have to be fixed.
      // Untill we do it preoperly, add them here:
      /*
      P2trHashSetIter  triter;
      P2tRTriangle    *trit;

      p2tr_hash_set_iter_init (&triter,T->tris);
      while (p2tr_hash_set_iter_next (&triter, (gpointer*)&trit))
        {
          if (p2tr_triangle_smallest_non_seperating_angle (trit) < theta)
            g_sequence_insert_sorted (Qt, t, ugliness_comparision, NULL);
        }
      */
     }
}

/*
 * Input must be a GPtrArray of P2tRPoint*
 */
P2tRTriangulation*
p2tr_triangulate_and_refine (GPtrArray *pts, int max_refine_steps)
{
  gint i, N = pts->len;
  GList *XEs = NULL, *iter;
  P2tRTriangulation *T;

  for (i = 0; i < pts->len; i++)
    {
      P2tREdge *E = p2tr_point_edge_to ((P2tRPoint*)g_ptr_array_index_cyclic (pts, i),
                                        (P2tRPoint*)g_ptr_array_index_cyclic (pts, i+1));
      p2tr_edge_set_constrained (E, TRUE);
      XEs = g_list_prepend (XEs, E);
    }

  T = p2tr_triangulateA ((P2tRPoint**)pts->pdata ,pts->len);
  DelaunayTerminator (T,XEs,M_PI/6,p2tr_false_delta, max_refine_steps);

  foreach (iter, XEs)
  {
    P2tREdge *E = (P2tREdge*) iter->data;
    p2tr_edge_unref (E);
  }

  g_list_free (XEs);

  return T;
}
