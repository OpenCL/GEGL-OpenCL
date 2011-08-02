#include <math.h>

#include "../common/utils.h"
#include "triangulation.h"
#include "../poly2tri.h"

static void p2tr_edge_init (P2tREdge *self, P2tRPoint *start, P2tRPoint *end);

static void p2tr_edge_init_private (P2tREdge *self, P2tRPoint *start, P2tRPoint *end, gboolean mirror);

static void p2tr_edge_remove_private (P2tREdge *self, P2tRTriangulation *T);

static void p2tr_point_init (P2tRPoint *self, gdouble x, gdouble y);

static void p2tr_point_add_edge (P2tRPoint *self, P2tREdge  *edge);


/* ########################################################################## */
/*                              Common math                                   */
/* ########################################################################## */
gdouble
p2tr_math_normalize_angle (gdouble angle)
{
  while (angle < -M_PI)
    angle += 2 * M_PI_2;
  while (angle > M_PI)
    angle -= 2 * M_PI;
  return angle;
}

/* NOTE: exact copy of p2t_orient2d, with different EPSILON2
 * TODO: merge somehow
 */
P2tROrientation
p2tr_math_orient2d (P2tRPoint* pa,
               P2tRPoint* pb,
               P2tRPoint* pc)
{
  double detleft = (pa->x - pc->x) * (pb->y - pc->y);
  double detright = (pa->y - pc->y) * (pb->x - pc->x);
  double val = detleft - detright;
//  p2tr_debug ("orient2d val is %f\n",val);
  if (val > -EPSILON2 && val < EPSILON2)
    {
//      p2tr_debug ("COLLINEAR!\n");
      return COLLINEAR;
    }
  else if (val > 0)
    {
//      p2tr_debug ("CCW!\n");
      return CCW;
    }
//  p2tr_debug ("CW!\n");
  return CW;
}

/* TODO: check relation with p2t_utils_in_scan_area */
/* This function is based on an article by Jonathan Richard Shewchuk
 * "Robust Adaptive Floating-Point Geometric Predicates"
 *
 * | ax-dx  ay-dy  (ax-dx)^2+(ay-dy)^2 |
 * | bx-dx  by-dy  (bx-dx)^2+(by-dy)^2 |
 * | cx-dx  cy-dy  (cx-dx)^2+(cy-dy)^2 |
 *
 * A, B AND C MUST BE IN CCW ORDER!
 */
P2tRInCircle
p2tr_math_incircle (P2tRPoint* a,
               P2tRPoint* b,
               P2tRPoint* c,
               P2tRPoint* d)
{
    gdouble ad_dx = a->x - d->x;
    gdouble ad_dy = a->y - d->y;
    gdouble ad_dsq = ad_dx * ad_dx + ad_dy * ad_dy;

    gdouble bd_dx = b->x - d->x;
    gdouble bd_dy = b->y - d->y;
    gdouble bd_dsq = bd_dx * bd_dx + bd_dy * bd_dy;

    gdouble cd_dx = c->x - d->x;
    gdouble cd_dy = c->y - d->y;
    gdouble cd_dsq = cd_dx * cd_dx + cd_dy * cd_dy;

    gdouble calc = + ad_dx * (bd_dy * cd_dsq - cd_dy * bd_dsq)
           - ad_dy * (bd_dx * cd_dsq - cd_dx * bd_dsq)
           + ad_dsq * (bd_dx * cd_dy - cd_dx * bd_dy);

    if (calc > EPSILON2)
        return INCIRCLE_INSIDE;
    else if (calc < EPSILON2)
        return INCIRCLE_OUTSIDE;
    else
        return INCIRCLE_ON;
}

gdouble
p2tr_math_edge_len_sq (gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
  gdouble dx = x2 - x1;
  gdouble dy = y2 - y1;
  return dx * dx + dy * dy;
}

/* ########################################################################## */
/*                           Triangulation struct                             */
/* ########################################################################## */

void
p2tr_triangulation_remove_pt (P2tRTriangulation *self, P2tRPoint *pt)
{
  if (p2tr_hash_set_remove (self->pts, pt))
    p2tr_point_unref (pt);
}

void
p2tr_triangulation_remove_ed (P2tRTriangulation *self, P2tREdge *ed)
{
  if (p2tr_hash_set_remove (self->edges, ed))
    p2tr_edge_unref (ed);
}

void
p2tr_triangulation_remove_tr (P2tRTriangulation *self, P2tRTriangle *tr)
{
  if (p2tr_hash_set_remove (self->tris, tr))
    p2tr_triangle_unref (tr);
}

void
p2tr_triangulation_add_pt (P2tRTriangulation *self, P2tRPoint *pt)
{
  if (p2tr_hash_set_contains (self->pts, pt))
    return;

  p2tr_hash_set_insert (self->pts, pt);
  p2tr_point_ref (pt);
}
void
p2tr_triangulation_add_ed (P2tRTriangulation *self, P2tREdge *ed)
{
  if (p2tr_hash_set_contains (self->edges, ed))
    return;

  p2tr_hash_set_insert (self->edges, ed);
  p2tr_edge_ref (ed);
}
void
p2tr_triangulation_add_tr (P2tRTriangulation *self, P2tRTriangle *tr)
{
  if (p2tr_hash_set_contains (self->tris, tr))
    return;

  p2tr_hash_set_insert (self->tris, tr);
  p2tr_triangle_ref (tr);
}

void
p2tr_triangulation_get_points (P2tRTriangulation *self, GPtrArray *dest)
{
  P2trHashSetIter  iter;
  P2tRTriangle    *tr;
  P2tRHashSet     *pts = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
  gint             i;
  
  p2tr_hash_set_iter_init (&iter, self->tris);
  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&tr))
    {
	  for (i = 0; i < 3; i++)
	    {
		  P2tRPoint *pt = p2tr_edge_get_end (tr->edges[i]);
	      if (! p2tr_hash_set_contains (pts, pt))
	        {
			  p2tr_point_ref (pt);
			  g_ptr_array_add (dest, pt);
			  p2tr_hash_set_insert (pts, pt);
		    }
		}
    }

  g_hash_table_free (pts);
}
/* ########################################################################## */
/*                               Point struct                                 */
/* ########################################################################## */

static void
p2tr_point_init (P2tRPoint *self,
                 gdouble    x,
                 gdouble    y)
{
  self->x = x;
  self->y = y;
  self->edges = NULL;
  self->_refcount = 1;
}

P2tRPoint*
p2tr_point_new (gdouble x,
                gdouble y)
{
  P2tRPoint *self = g_slice_new (P2tRPoint);
  p2tr_point_init (self, x, y);
  return self;
}

static void
p2tr_point_destroy (P2tRPoint *self)
{
  GList *iter;

  g_assert (self->_refcount == 0);

  foreach (iter, self->edges)
    {
      P2tREdge *e = (P2tREdge*) iter->data;
      p2tr_edge_unref (e);
    }

  g_list_free (self->edges);
}

void
p2tr_point_free (P2tRPoint *self)
{
  p2tr_point_destroy (self);
  g_slice_free (P2tRPoint, self);
}
/* DBG: Differs from original
 *
 * THIS FUNCTION SHOULD NOT BE USED DIRECTLY! IT IS JUST A CONVINIENCE FUNCTION
 * USED BY THE CONSTRUCTOR OF THE EDGE OBJECT TO ACTUALLY REFERENCE THE EDGE
 * FROM THE POINT!
 */
static void
p2tr_point_add_edge (P2tRPoint *self,
                     P2tREdge  *edge)
{
  GList *iter = self->edges;
  for (iter = self->edges; iter != NULL; iter = iter->next)
    {
      if (!(P2TR_EDGE(iter->data)->angle < edge->angle))
        break;
    }

  /* Inserting before NULL will insert at the end of the list */
  self->edges = g_list_insert_before (self->edges, iter, edge);

  /* We now hold a reference to the edge! */
  p2tr_edge_ref (edge);
}

void
p2tr_point_remove_edge (P2tRPoint *self,
                        P2tREdge  *edge)
{
  p2tr_assert_and_explain (g_list_find (self->edges, edge) != NULL, "can't remove non existing edge");
  self->edges = g_list_remove (self->edges, edge);
  p2tr_edge_unref (edge);
}

void
p2tr_point_remove (P2tRPoint         *self,
                   P2tRTriangulation *T)
{
  GList *iter = self->edges;
  for (iter = self->edges; iter != NULL; iter = iter->next)
    {
      p2tr_edge_remove (P2TR_EDGE(iter->data), T);
    }
  p2tr_triangulation_remove_pt (T, self);
}

/* This function will return an edge from the current
 * point to a given other point. If no such edge exists,
 * it will be created
 */
P2tREdge*
p2tr_point_edge_to (P2tRPoint *self, P2tRPoint *end)
{
  GList *iter = self->edges;

  for (iter = self->edges; iter != NULL; iter = iter->next)
    if (P2TR_EDGE(iter->data)->end == end)
      {
        p2tr_edge_ref (P2TR_EDGE(iter->data));
        return P2TR_EDGE (iter->data);
      }

  /* Don't decrease reference - this is returned to someone else */
  return p2tr_edge_new (self, end);
}

P2tREdge*
p2tr_point_has_edge_to (P2tRPoint *self, P2tRPoint *end)
{
  GList *iter = self->edges;

  for (iter = self->edges; iter != NULL; iter = iter->next)
    if (P2TR_EDGE(iter->data)->end == end)
      return P2TR_EDGE(iter->data);

  return NULL;
}

P2tREdge*
p2tr_point_edgeccw (P2tRPoint *self, P2tREdge *edge)
{
  GList *index = g_list_find (self->edges, edge);
  return p2tr_edgelist_ccw (self->edges, index);
}

P2tREdge*
p2tr_point_edgecw (P2tRPoint *self, P2tREdge *edge)
{
  GList *index = g_list_find (self->edges, edge);
  return p2tr_edgelist_cw (self->edges, index);
}

/*
 *        ^ e1 = CCW Neighbor of e
 *       /
 *      /  e1.tri
 *     /
 *  P *------> e
 *     \
 *      \  e.tri
 *       \
 *       v e2 = CW Neighbor of e
 *
 *  e is in a cluster defined by P, if any of the
 *  following conditions hold:
 *  - The angle between e and e2 is less than 60 degrees,
 *    and the triangle of e1 is not None (i.e. the area
 *    between the edges is in the triangulation domain)
 *  - Same with e and e1
 */
gboolean
p2tr_point_is_in_cluster (P2tRPoint *self, P2tREdge *e)
{
  /* TODO: make more efficient instead of two searches */
  GList *eL = g_list_find (self->edges, e);
  P2tREdge *e1 = p2tr_edgelist_ccw (self->edges, eL);
  P2tREdge *e2 = p2tr_edgelist_cw  (self->edges, eL);

  gdouble Ae1e = p2tr_math_normalize_angle (e1->angle - e->angle);
  gdouble Aee2 = p2tr_math_normalize_angle (e->angle - e2->angle);

  return (e1->tri != NULL && ABS (Ae1e) <= M_PI/3)
               || (e->tri != NULL && ABS (Aee2) <= M_PI/3);
}

/*
 * DBG: different from original
 */
GList*
p2tr_point_get_cluster (P2tRPoint *self, P2tREdge *e, gdouble *angle)
{
  gdouble A = M_PI, a;
  GList *eI = g_list_find (self->edges, e);
  GList *temp;

  GList *S = g_list_append (NULL, e);

  GList *ePrev = g_list_cyclic_prev (self->edges,eI);
  GList *eNext = g_list_cyclic_next (self->edges,eI);

  a = p2tr_math_normalize_angle (e->angle - P2TR_EDGE (ePrev->data)->angle);
  while (a <= M_PI / 3 && ePrev != eI)
    {
      A = MIN(a, A);
      S = g_list_prepend (S, P2TR_EDGE (ePrev->data));
      temp = ePrev;
      ePrev = g_list_cyclic_prev (self->edges, ePrev);
      a = p2tr_math_normalize_angle (P2TR_EDGE (temp->data)->angle - P2TR_EDGE (ePrev->data)->angle);
    }

  a = p2tr_math_normalize_angle (P2TR_EDGE (eNext->data)->angle - e->angle);
  while (a <= M_PI / 3 && eNext->data != S->data)
    {
      A = MIN(a, A);
      S = g_list_append (S, P2TR_EDGE (eNext->data));
      temp = eNext;
      eNext = g_list_cyclic_next (self->edges, eNext);
      a = p2tr_math_normalize_angle (P2TR_EDGE (eNext->data)->angle - P2TR_EDGE (temp->data)->angle);
    }

  if (angle != NULL)
    *angle = A;

  return S;
}

gboolean
p2tr_point_is_fully_in_domain (P2tRPoint *self)
{
  GList *iter = self->edges;

  for (iter = self->edges; iter != NULL; iter = iter->next)
    if (P2TR_EDGE(iter->data)->end == NULL)
      return FALSE;

  return TRUE;
}

/* ########################################################################## */
/*                               Edge struct                                  */
/* ########################################################################## */

P2tREdge*
p2tr_edge_new (P2tRPoint *start, P2tRPoint *end)
{
  return p2tr_edge_new_private (start, end, FALSE);
}

P2tREdge*
p2tr_edge_new_private (P2tRPoint *start, P2tRPoint *end, gboolean mirror)
{
  P2tREdge *self = g_slice_new (P2tREdge);
  p2tr_edge_init_private (self, start, end, mirror);
  return self;
}

static void
p2tr_edge_init (P2tREdge *self, P2tRPoint *start, P2tRPoint *end)
{
  p2tr_edge_init_private (self, start, end, FALSE);
}

static void
p2tr_edge_init_private (P2tREdge *self, P2tRPoint *start, P2tRPoint *end, gboolean mirror)
{
  self->_refcount = 0;
  self->removed = FALSE;
  self->end = end;
  self->tri = NULL;

  self->angle = atan2 (end->y - start->y, end->x - start->x);

  self->delaunay = FALSE;
  self->constrained = FALSE;

  /* Important! Add the edge only once we have an angle!
   * This function also adds a reference to the edge, since the point now points
   * at the edge. */
  p2tr_point_add_edge (start, self);

  if (!mirror)
    {
      self->mirror = p2tr_edge_new_private (end, start, TRUE);
      self->mirror->mirror = self;
    }

  /* The edge that was created should have it's reference count increased by 1,
   * since it will be returned (so mark the person who gets it holds a ref).
   * Note that we don't count the reference loop between two mirror edges */
  if (!mirror)
    p2tr_edge_ref (self);

  /* Finally, the edge now references it's end point */
  p2tr_point_ref (self->end);

  p2tr_assert_and_explain (p2tr_point_has_edge_to (start, end), "edge connectivity");
  p2tr_assert_and_explain (p2tr_point_has_edge_to (end, start), "edge connectivity");
}

/* Note that you can't really free one edge - since the edge and it's mirror are
 * tightly coupled. By freeing one of them, you will also free the other - so
 * beware of double freeing!
 *
 * The best way is not to free directly, but to use the unref function.
 *
 * TODO: Merge some logic and fill in missing part with the edge_remove function
 */
void
p2tr_edge_free (P2tREdge *self)
{
  g_assert (self->_refcount == 0);

  if (self->mirror->_refcount != 0)
    return;

  self->removed = self->mirror->removed = TRUE;

  p2tr_point_unref (self->mirror->end);
  p2tr_point_unref (self->end);

  p2tr_triangle_unref (self->mirror->tri);
  p2tr_triangle_unref (self->tri);

  g_slice_free (P2tREdge, self->mirror);
  g_slice_free (P2tREdge, self);
}

/* You can't remove just one edge from a triangulation. When you remove an edge,
 * it's mirror is also removed! Calling this will also remove the edge from the
 * points that form the edge.
 *
 * TODO: Merge some logic and fill in missing part with the edge_free function
 */
void
p2tr_edge_remove (P2tREdge *self, P2tRTriangulation *T)
{
  p2tr_edge_remove_private (self, T);
  p2tr_validate_triangulation (T);
}

static void
p2tr_edge_remove_private (P2tREdge *self, P2tRTriangulation *T)
{
  P2tREdge *mir = self->mirror;

  if (self->removed)
    return;
  
  if (self->tri != NULL)
    p2tr_triangle_remove (self->tri, T);
  p2tr_validate_triangulation (T);

  if (mir->tri != NULL)
    p2tr_triangle_remove (mir->tri, T);
  p2tr_validate_triangulation (T);

  self->removed = mir->removed = TRUE;

  p2tr_validate_triangulation (T);
  p2tr_point_remove_edge (p2tr_edge_get_start (self), self);
  p2tr_validate_triangulation (T);
  p2tr_point_remove_edge (p2tr_edge_get_start (mir), mir);
  p2tr_validate_triangulation (T);

  p2tr_triangulation_remove_ed (T, self);
  p2tr_triangulation_remove_ed (T, mir);
}

/*
 * DBG: different from source
 */
gboolean
p2tr_edge_is_encroached_by (P2tREdge *self, P2tRPoint *other)
{
  if (p2tr_edge_diametral_circle_contains (self, other))
    {
      p2tr_debug ("The point ");
      p2tr_debug_point (other, FALSE);
      p2tr_debug (" encroaches ");
      p2tr_debug_edge (self, TRUE);
      return TRUE;
    }
  return FALSE;
}

/*
 * DBG: different from source
 */
gboolean
p2tr_edge_is_encroached (P2tREdge *self)
{
  if (self->tri != NULL)
    {
      P2tRPoint *w = p2tr_triangle_opposite_point (self->tri, self);
      if (p2tr_edge_is_encroached_by (self, w))
        return TRUE;
    }

  if (self->mirror->tri != NULL)
    {
      P2tRPoint *w = p2tr_triangle_opposite_point (self->mirror->tri, self);
      if (p2tr_edge_is_encroached_by (self, w))
        return TRUE;
    }

  p2tr_debug ("The edge ");
  p2tr_debug_edge (self, FALSE);
  p2tr_debug (" is not encroached!\n");

  return FALSE;
}

gboolean
p2tr_edge_diametral_circle_contains (P2tREdge *self, P2tRPoint *pt)
{
  /* a-----O-----b
   *      /
   *     /
   *    pt
   *
   * pt is in the diametral circle only if it's distance from O (the center of
   * a and b) is equal/less than a's distance from O (that's the radius).
   */
  P2tRPoint *a = p2tr_edge_get_start (self);
  P2tRPoint *b = p2tr_edge_get_end (self);

  gdouble Ox = (a->x + b->x) / 2;
  gdouble Oy = (a->y + b->y) / 2;

  return (Ox - a->x) * (Ox - a->x) + (Oy - a->y) * (Oy - a->y)
          >= (Ox - pt->x) * (Ox - pt->x) + (Oy - pt->y) * (Oy - pt->y);
  /*
  return    (pt->y - a->y) * (pt->x - a->x)
          * (pt->y - b->y) * (pt->x - b->x) <= 0;
   */
}

gboolean
p2tr_edge_diametral_lens_contains (P2tREdge *self, P2tRPoint *W)
{
  /*
   *       W
   *      /|\
   *     / | \
   *    /60|60\
   *   /   |   \
   *  /30  |  30\
   * X-----O-----Y
   *    L     L
   *
   * Non-Efficient: Calculate XW, and XO=YO
   * 
   *                 |XO|         ===> |XO|    sqrt(3) ===> |XO|^2    3
   * Make sure asin (----) >= 60° ===> ---- >= ------- ===> ------ >= -
   *                 |XW|         ===> |XW|      2     ===> |XW|^2    4
   *
   *                 |YO|         ===> |YO|    sqrt(3) ===> |YO|^2    3
   * Make sure asin (----) >= 60° ===> ---- >= ------- ===> ------ >= -
   *                 |YW|         ===> |YW|      2     ===> |YW|^2    4
   */

  P2tRPoint *X = p2tr_edge_get_start (self);
  P2tRPoint *Y = p2tr_edge_get_end (self);

  gdouble XO2 = p2tr_edge_len_sq (self) / 4, YO2 = XO2;

  gdouble XW2 = p2tr_math_edge_len_sq (X->x, X->y, W->x, W->y);
  gdouble YW2 = p2tr_math_edge_len_sq (Y->x, Y->y, W->x, W->y);

  return (XO2 / XW2 >= 0.75) && (YO2 / YW2 >= 0.75);
}

gdouble
p2tr_edge_len_sq (P2tREdge *self)
{
  P2tRPoint *S = p2tr_edge_get_start (self);
  P2tRPoint *E = p2tr_edge_get_end (self);
  return p2tr_math_edge_len_sq (S->x,S->y,E->x,E->y);
}

void
p2tr_edge_set_constrained (P2tREdge *self, gboolean b)
{
  self->constrained = self->mirror->constrained = b;
}

void
p2tr_edge_set_delaunay (P2tREdge *self, gboolean b)
{
  self->delaunay = self->mirror->delaunay = b;
}

/* a = abs(E1.angle)
 * b = abs(E2.angle)
 *
 * A is the angle we wish to find. Note the fact that we want
 * to find the angle so that the edges go CLOCKWISE around it.
 *
 * Case 1: Signs of E1.angle and | Case 2: Signs of E1.angle and
 *         E2.angle agree        |         E2.angle disagree
 *                               |
 * A=a'+b=180-a+b                | A=180-a-b
 *
 *             ^^                |
 *         E2 //                 |         *
 *           //                  |       ^^ \\
 *          // \                 |      // A \\
 *         *  A|                 |  E1 // \_/ \\ E2
 *        /^^ /                  |    //       \\
 *       / ||                    |   //a        vv
 *      /  || E1                 | ------------------
 *     /   ||                    |                \b
 *    /b a'||a                   |                 \
 * --------------                |                  \
 * PRECONDITION: e1.getEnd() == e2.getStart()
 */
gdouble
p2tr_angle_between (P2tREdge *e1, P2tREdge *e2)
{
  gdouble RET;
  
  g_assert (p2tr_edge_get_end (e1) == p2tr_edge_get_start (e2));
  
  if (e1->angle < 0)
    {
      if (e2->angle > 0)
        return ABS (e1->angle) + ABS (e2->angle) - M_PI;
      else
        return ABS (e1->angle) - (ABS (e2->angle) - M_PI);
    }
  else
    {
      if (e2->angle > 0)
        return M_PI - ABS (e1->angle) + ABS (e2->angle);
      else
        return M_PI - ABS (e1->angle) - ABS (e2->angle);
    }
  
  /* g_assert (RET + EPSILON2 >= 0 && RET <= M_PI + EPSILON2); */
  
  return RET;
}

void
p2tr_triangle_init (P2tRTriangle *self, P2tREdge *e1, P2tREdge *e2, P2tREdge *e3, P2tRTriangulation *T)
{
  P2tRPoint *A = p2tr_edge_get_end (e1);
  P2tRPoint *B = p2tr_edge_get_end (e2);
  P2tRPoint *C = p2tr_edge_get_end (e3);

  /* Assert that edges do form a loop! */
  p2tr_assert_and_explain (A == p2tr_edge_get_start (e2)
          && B == p2tr_edge_get_start (e3)
          && C == p2tr_edge_get_start (e1), "Edges form a loop!");

  P2tROrientation o = p2tr_math_orient2d (A, B, C);

  /*
   *    ^
   *   | \           |    ^ \
   * a |  \ c   ==>  | a' |  \ c'
   *   v   \         |    |   \
   *   ----->        |    <--- v
   *      b          |       b'
   *
   * When found a CCW triangle with edges a-b-c, change it to
   * a'-c'-b' and not a'-b'-c' !!!
   */

  p2tr_assert_and_explain (o != COLLINEAR, "No support for collinear points!");

   if (o == CCW)
     {
       self->edges[0] = e1->mirror;
       self->edges[1] = e3->mirror;
       self->edges[2] = e2->mirror;
     }
   else
     {
       self->edges[0] = e1;
       self->edges[1] = e2;
       self->edges[2] = e3;
     }

  /* The triangle is now referenced by the person who requested it, and also by
   * 3 edges. Also, the 3 edges are now referenced by the triangle.
   */
  self->edges[0]->tri = self->edges[1]->tri = self->edges[2]->tri = self;
  self->_refcount = 4;
  p2tr_edge_ref (self->edges[0]);
  p2tr_edge_ref (self->edges[1]);
  p2tr_edge_ref (self->edges[2]);

  if (T != NULL)
    {
      p2tr_triangulation_add_tr (T, self);
    }
}

P2tRTriangle*
p2tr_triangle_new (P2tREdge *e1, P2tREdge *e2, P2tREdge *e3, P2tRTriangulation *T)
{
  P2tRTriangle *self = g_slice_new (P2tRTriangle);
  p2tr_triangle_init (self, e1, e2, e3, T);
  return self;
}

/* TODO: merge logic with the remove function */
void
p2tr_triangle_free (P2tRTriangle *self)
{
  g_assert (self->_refcount == 0);

  p2tr_edge_unref (self->edges[0]);
  p2tr_edge_unref (self->edges[1]);
  p2tr_edge_unref (self->edges[2]);

  g_slice_free (P2tRTriangle, self);
}

/*
 *     e0.end
 *       ^
 *       | \
 *       |  \           e0 <=> e1.end
 *    e0 |   \ e1       e1 <=> e2.end
 *       |    \         e2 <=> e0.end
 *       <---- v
 * e2.end   e2   e1.end
 */
P2tRPoint*
p2tr_triangle_opposite_point (P2tRTriangle *self, P2tREdge *e)
{
  if (self->edges[0] == e || self->edges[0] == e->mirror)
    return p2tr_edge_get_end (self->edges[1]);
  else if (self->edges[1] == e || self->edges[1] == e->mirror)
    return p2tr_edge_get_end (self->edges[2]);
  else if (self->edges[2] == e || self->edges[2] == e->mirror)
    return p2tr_edge_get_end (self->edges[0]);

  p2tr_assert_and_explain (FALSE, "Edge not in in triangle!");
}

P2tREdge*
p2tr_triangle_opposite_edge (P2tRTriangle *self, P2tRPoint *pt)
{
  if (p2tr_edge_get_end (self->edges[0]) == pt)
    return self->edges[2];
  else if (p2tr_edge_get_end (self->edges[1]) == pt)
    return self->edges[0];
  else if (p2tr_edge_get_end (self->edges[2]) == pt)
    return self->edges[1];

  p2tr_assert_and_explain (FALSE, "Point not in in triangle!");
}

/* Return the smallest angle not seperating two input segments (Input segments
 * can not be disconnected, so we can't fix a small angle between them)
 */
gdouble
p2tr_triangle_smallest_non_seperating_angle (P2tRTriangle *self)
{
    gdouble minA = M_PI;

    P2tREdge *e0 = self->edges[0];
    P2tREdge *e1 = self->edges[1];
    P2tREdge *e2 = self->edges[2];

    if (! e0->mirror->constrained && ! e1->mirror->constrained)
        minA = MIN (minA, p2tr_angle_between(self->edges[0], self->edges[1]));

    if (! e1->mirror->constrained && ! e2->mirror->constrained)
        minA = MIN (minA, p2tr_angle_between(self->edges[1], self->edges[2]));

    if (! e2->mirror->constrained && ! e0->mirror->constrained)
        minA = MIN (minA, p2tr_angle_between(self->edges[2], self->edges[0]));

    return minA;
}

gdouble
p2tr_triangle_shortest_edge_len (P2tRTriangle *self)
{
  gdouble l0 = p2tr_edge_len_sq (self->edges[0]);
  gdouble l1 = p2tr_edge_len_sq (self->edges[1]);
  gdouble l2 = p2tr_edge_len_sq (self->edges[2]);

  return sqrt (MIN (l0, MIN (l1, l2)));
}

gdouble
p2tr_triangle_longest_edge_len (P2tRTriangle *self)
{
  gdouble l0 = p2tr_edge_len_sq (self->edges[0]);
  gdouble l1 = p2tr_edge_len_sq (self->edges[1]);
  gdouble l2 = p2tr_edge_len_sq (self->edges[2]);

  return sqrt (MAX (l0, MAX (l1, l2)));
}

void
p2tr_triangle_angles (P2tRTriangle *self, gdouble dest[3])
{
  dest[0] = p2tr_angle_between(self->edges[0], self->edges[1]);
  dest[1] = p2tr_angle_between(self->edges[1], self->edges[2]);
  dest[2] = p2tr_angle_between(self->edges[2], self->edges[0]);
}

gdouble
p2tr_triangle_get_angle_at (P2tRTriangle *self, P2tRPoint* pt)
{
  if (pt == self->edges[0]->end)
    return p2tr_angle_between(self->edges[0], self->edges[1]);
  else if (pt == self->edges[1]->end)
    return p2tr_angle_between(self->edges[1], self->edges[2]);
  else if (pt == self->edges[2]->end)
    return p2tr_angle_between(self->edges[2], self->edges[0]);
  else
    p2tr_assert_and_explain (FALSE, "Trying to get the angle at a point not in the tri!\n");
}

/*
 * TODO: merge logic with the free function
 */
void
p2tr_triangle_remove (P2tRTriangle *self, P2tRTriangulation *T)
{
  p2tr_debug ("Removing a triangle\n");

  if (T != NULL)
    p2tr_triangulation_remove_tr (T, self);

  self->edges[0]->tri = NULL;
  p2tr_triangle_unref (self);

  self->edges[1]->tri = NULL;
  p2tr_triangle_unref (self);

  self->edges[2]->tri = NULL;
  p2tr_triangle_unref (self);

}

void
p2tr_triangle_circumcircle (P2tRTriangle *self, P2tRCircle *dest)
{
    P2tRPoint *A = p2tr_edge_get_end (self->edges[2]);
    P2tRPoint *B = p2tr_edge_get_end (self->edges[0]);
    P2tRPoint *C = p2tr_edge_get_end (self->edges[1]);

    gdouble Anorm = A->x * A->x + A->y * A->y;
    gdouble Bnorm = B->x * B->x + B->y * B->y;
    gdouble Cnorm = C->x * C->x + C->y * C->y;

    gdouble D = 2*(A->x * (B->y - C->y) + B->x * (C->y - A->y) + C->x * (A->y - B->y));

    dest->x =  (Anorm * (B->y - C->y) + Bnorm * (C->y - A->y) + Cnorm * (A->y - B->y)) / D;
    dest->y = -(Anorm * (B->x - C->x) + Bnorm * (C->x - A->x) + Cnorm * (A->x - B->x)) / D;

    dest->radius = sqrt (p2tr_math_edge_len_sq (A->x, A->y, dest->x, dest->y));
}

gboolean
p2tr_triangle_is_circumcenter_inside (P2tRTriangle *self)
{
  gdouble angles[3];

  p2tr_triangle_angles (self, angles);

  return MAX (angles[0], MAX (angles[1], angles[2])) < M_PI / 2;
}


/*
 *       P0
 *      ^  \
 *     /    \
 *  e2/      \e0
 *   /   *C   \
 *  /          v
 * P2 <------- P1
 *      e1
 *
 * DBG: different from source
 */
void
p2tr_triangle_subdivide (P2tRTriangle *self, P2tRPoint *C, P2tRTriangulation *T, P2tRTriangle *dest_new[3])
{
  P2tREdge *e0 = self->edges[0];
  P2tREdge *e1 = self->edges[1];
  P2tREdge *e2 = self->edges[2];

  P2tREdge *CP0, *CP1, *CP2;

  P2tRPoint *P0 = p2tr_edge_get_end (e2);
  P2tRPoint *P1 = p2tr_edge_get_end (e0);
  P2tRPoint *P2 = p2tr_edge_get_end (e1);

  P2tRTriangle *t0, *t1, *t2;

  if (C == NULL)
    {
      P2tRCircle cc;
      p2tr_triangle_circumcircle (self, &cc);
      C = p2tr_point_new (cc.x, cc.y);
    }

  g_assert (p2tr_triangle_contains_pt (self, C));

  p2tr_validate_triangulation (T);
  p2tr_triangle_remove (self, T);
  p2tr_validate_triangulation (T);

  CP0 = p2tr_point_edge_to (C, P0);
  CP1 = p2tr_point_edge_to (C, P1);
  CP2 = p2tr_point_edge_to (C, P2);

  t0 = p2tr_triangle_new (CP0, e0, CP1->mirror, T);
  p2tr_validate_triangulation (T);
  t1 = p2tr_triangle_new (CP1, e1, CP2->mirror, T);
  p2tr_validate_triangulation (T);
  t2 = p2tr_triangle_new (CP2, e2, CP0->mirror, T);
  p2tr_validate_triangulation (T);

  if (dest_new != NULL)
    {
      dest_new[0] = t0;
      dest_new[1] = t1;
      dest_new[2] = t2;
    }
  else
    {
      p2tr_triangle_unref (t0);
      p2tr_triangle_unref (t1);
      p2tr_triangle_unref (t2);
    }
}

/* Based on http://www.blackpawn.com/texts/pointinpoly/default.html */
gboolean
p2tr_triangle_contains_pt (P2tRTriangle *self, P2tRPoint *P)
{
  P2tRPoint *A = p2tr_edge_get_end (self->edges[2]);
  P2tRPoint *B = p2tr_edge_get_end (self->edges[0]);
  P2tRPoint *C = p2tr_edge_get_end (self->edges[1]);

  gdouble v0x = C->x - A->x;
  gdouble v0y = C->y - A->y;
  gdouble v1x = B->x - A->x;
  gdouble v1y = B->y - A->y;
  gdouble v2x = P->x - A->x;
  gdouble v2y = P->y - A->y;

  /* Compute dot products */
  gdouble dot00 = v0x * v0x + v0y * v0y;
  gdouble dot01 = v0x * v1x + v0y * v1y;
  gdouble dot02 = v0x * v2x + v0y * v2y;
  gdouble dot11 = v1x * v1x + v1y * v1y;
  gdouble dot12 = v1x * v2x + v1y * v2y;

  /* Compute barycentric coordinates */
  gdouble invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
  gdouble u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  gdouble v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  /* Check if point is in triangle */
  return (u > -EPSILON2) && (v > -EPSILON2) && (u + v < 1 + EPSILON2);
}


gboolean
p2tr_triangulation_legalize (P2tRTriangulation *T,
                             P2tRTriangle      *tr)
{
  /* Remember! Edges in each triangle are ordered counter clockwise!
   *   q
   *   *-----------*a    shared_tr = p->q = tr->edges[i]
   *   |\         /      shared_ot = q->p = tr->edges[i]->mirror
   *   | \       /       ot = shared_ot->tri
   *   |  \ tr  /
   *   |   \   /
   *   | ot \ /
   *   *-----*
   *   b      p
   * 
   * Also note that we can not flip in cases of concave quads! We must check
   * that the angles at p and at q are both smaller than 180°.
   */
  gint i;

  /* First, make sure this triangle still exists */
  if (! p2tr_hash_set_contains (T->tris, tr))
    return FALSE;
  
  /* To legalize a triangle we start by finding if any of the three edges
   * violate the Delaunay condition
   */
  for (i = 0; i < 3; i++)
    {
      P2tREdge *shared_tr = tr->edges[i];
      P2tREdge *shared_ot = shared_tr->mirror;
      P2tRTriangle* ot = shared_ot->tri;

      // If this is a Constrained Edge or a Delaunay Edge (only during
      // recursive legalization) then we should not try to legalize
      if (shared_tr->delaunay || shared_tr->constrained)
        continue;

      if (ot)
        {
          P2tRPoint* p = p2tr_edge_get_end (shared_ot);
          P2tRPoint* q = p2tr_edge_get_end (shared_tr);
          P2tRPoint* a = p2tr_triangle_opposite_point (tr, shared_tr);
          P2tRPoint* b = p2tr_triangle_opposite_point (ot, shared_ot);
          // We already checked if it's constrained or delaunay for the case of
          // skipping this tri

          // Check for concave quads
          if (p2tr_triangle_get_angle_at (tr, p) + p2tr_triangle_get_angle_at (ot, p) >= M_PI
              || p2tr_triangle_get_angle_at (tr, q) + p2tr_triangle_get_angle_at (ot, q) >= M_PI)
            continue;

          gboolean inside = p2tr_math_incircle (p, a, q, b);

          if (inside)
            {
              P2tREdge *ab;
              P2tREdge *bq, *qa, *pb, *ap;
              P2tRTriangle *abq, *pba;

              // First of all, remove the edge
              p2tr_edge_remove (shared_tr, T);

              // Create a new matching rotated edge
              ab = p2tr_edge_new (a, b);

              // Mark it as Delaunay
              p2tr_edge_set_delaunay (ab, TRUE);

              // Create the triangles
              bq = p2tr_point_edge_to (b,q);
              qa = p2tr_point_edge_to (q,a);
              abq = p2tr_triangle_new (ab, bq, qa, T);

              pb = p2tr_point_edge_to (p,b);
              ap = p2tr_point_edge_to (a,p);
              pba = p2tr_triangle_new (pb, ab->mirror, ap, T);

              // Unref stuff
              p2tr_edge_unref (bq);
              p2tr_edge_unref (qa);

              p2tr_edge_unref (pb);
              p2tr_edge_unref (ap);

              // Now, legalize these two triangles:
              p2tr_triangulation_legalize (T, abq);
              p2tr_triangle_unref (abq);

              // Legalization may have killed the other triangle, but that's OK
              // since legalization makes sure the triangle exists
              p2tr_triangulation_legalize (T, pba);
              p2tr_triangle_unref (pba);

              // Reset the Delaunay edges, since they only are valid Delaunay edges
              // until we add a new triangle or point.
              // XXX: need to think about this. Can these edges be tried after we
              //      return to previous recursive level?
              p2tr_edge_set_delaunay (ab, FALSE);

              // If triangle have been legalized no need to check the other edges since
              // the recursive legalization will handles those so we can end here.
              return TRUE;
            }
        }
    }
  return FALSE;
}

/*       * A2
 *      / \
 *     /   \
 *    / T2  \
 * B *------>* C   e = B->C
 *    ^ T1  /
 *     \   /
 *      \ v
 *       * A1
 */
void
p2tr_triangulation_flip_fix (P2tREdge *e, P2tRTriangulation *T)
{
  P2tRTriangle *T1 = e->tri;
  P2tRTriangle *T2 = e->mirror->tri;

  /* Removed edges do not need fixing, constrained edges can not be flipped, and
   * edges that were already flipped should not be flipped again.
   *
   * Finally, edges must have a triangle on both sides of them (such a situation
   * can occur during the execution of the algorithms, without the edge being
   * constrained) in order to be flipped
   */
  if (e->removed || e->constrained || e->delaunay || T1 == NULL || T2 == NULL)
    return;

  P2tRPoint *A1 = p2tr_triangle_opposite_point (T1, e);
  P2tRPoint *A2 = p2tr_triangle_opposite_point (T2, e);
  P2tRPoint *B = p2tr_edge_get_start (e);
  P2tRPoint *C = p2tr_edge_get_end (e);

  /* We can't do a flip fix in cases where the two triangles form together a
   * concave quad!
   *
   * To check this, see if the sum of the two angles at any of the edges is
   * larger than 180 degress.
   */
  P2tREdge *BC = p2tr_point_edge_to (B, C);

  p2tr_validate_triangulation (T);
  P2tREdge *CA2 = p2tr_point_edge_to (C, A2);
  p2tr_validate_triangulation (T);
  P2tREdge *CA1 = p2tr_point_edge_to (C, A1);
  p2tr_validate_triangulation (T);

  P2tREdge *BA2 = p2tr_point_edge_to (B, A2);
  p2tr_validate_triangulation (T);
  P2tREdge *BA1 = p2tr_point_edge_to (B, A1);
  p2tr_validate_triangulation (T);

  gdouble BCA2 = p2tr_angle_between (CA2->mirror,BC->mirror);
  gdouble BCA1 = p2tr_angle_between (BC,CA1);

  gdouble CBA2 = p2tr_angle_between (BC->mirror,BA2);
  gdouble CBA1 = p2tr_angle_between (BA1->mirror,BC);

  if (BCA2 + BCA1 > M_PI - EPSILON2 || CBA2 + CBA1 > M_PI - EPSILON2)
    {
      p2tr_debug ("Won't fix concave quads!\n");

      /* In the case of concave quads, mark the edge as non flip-able */
      p2tr_edge_set_delaunay (BC, TRUE);

      /* Now fix the other edges */
      p2tr_triangulation_flip_fix (BA2,T);
      p2tr_triangulation_flip_fix (BA1,T);
      p2tr_triangulation_flip_fix (CA2,T);
      p2tr_triangulation_flip_fix (CA1,T);

      /* Restore */
      p2tr_edge_set_delaunay (BC, FALSE);
    }

  /* Check if empty circle property does not hold */
  else if (p2tr_math_incircle (A1,C,B,A2) != INCIRCLE_OUTSIDE
           || p2tr_math_incircle (A2,B,C,A1) != INCIRCLE_OUTSIDE)
    {
      P2tREdge *A2A1;
      P2tRTriangle *Tn1, *Tn2;

      p2tr_validate_triangulation (T);
      p2tr_edge_remove (e, T);
      p2tr_validate_triangulation (T);

      A2A1 = p2tr_point_edge_to (A2, A1);
      p2tr_validate_triangulation (T);

      Tn1 = p2tr_triangle_new (A2A1, BA1->mirror, BA2, T);
      p2tr_validate_triangulation (T);
      Tn2 = p2tr_triangle_new (A2A1, CA1->mirror, CA2, T);
      p2tr_validate_triangulation (T);

      p2tr_edge_set_delaunay (A2A1, TRUE);

      p2tr_triangulation_flip_fix (BA2,T);
      p2tr_triangulation_flip_fix (CA2,T);

      p2tr_triangulation_flip_fix (BA1->mirror,T);
      p2tr_triangulation_flip_fix (CA1->mirror,T);

      p2tr_edge_set_delaunay (A2A1, FALSE);
    }
}

/* TODO: UNEFFICIENT LIKE HELL! */
gboolean
p2tr_edges_intersect (P2tREdge *e1, P2tREdge *e2)
{
  P2tRPoint *e1S = p2tr_edge_get_start (e1);
  P2tRPoint *e1E = p2tr_edge_get_end (e1);
  P2tRPoint *e2S = p2tr_edge_get_start (e1);
  P2tRPoint *e2E = p2tr_edge_get_end (e1);

  return p2tr_math_orient2d (e1S,e1E,e2S) != p2tr_math_orient2d (e1S,e1E,e2E)
         && p2tr_math_orient2d (e2S,e2E,e1S) != p2tr_math_orient2d (e2S,e2E,e1E);
}

P2tRPoint *
p2tr_triangle_median_pt (P2tRTriangle *self)
{
  P2tRPoint *A = p2tr_edge_get_end (self->edges[2]);
  P2tRPoint *B = p2tr_edge_get_end (self->edges[0]);
  P2tRPoint *C = p2tr_edge_get_end (self->edges[1]);

  return p2tr_point_new ((A->x+B->x+C->x)/3,(A->y+B->y+C->y)/3);
}

/* TODO: this computation can be much optimized using math rules! */
P2tRPoint *
p2tr_edge_concentric_center (P2tREdge *e)
{
  gdouble x0 = p2tr_edge_get_start(e)->x,  y0 = p2tr_edge_get_start(e)->y;
  gdouble x1 = p2tr_edge_get_end(e)->x,  y1 = p2tr_edge_get_end(e)->y;
  gdouble fraction;

  gdouble l = sqrt (p2tr_edge_len_sq (e));
  /* Note that in the braces below, it's L and not 1 */
  gdouble lPart = pow (2, round (log2 (l/2)));

  while (lPart >= l)
    lPart /= 2
            ;
  fraction = lPart / l;

  return p2tr_point_new (x0 + fraction * (x1 - x0), y0 + fraction * (y1 - y0));
}

P2tRTriangulation*
p2tr_triangulate (GList *p2trpoints)
{
  P2tPointPtrArray D = g_ptr_array_new ();
  GList *iter;

  P2tRTriangulation *T = p2tr_triangulation_new ();

  GHashTable *map = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, NULL);

  foreach (iter, p2trpoints)
  {
    P2tRPoint *pt = (P2tRPoint*) iter->data;
    P2tPoint *opt = p2t_point_new_dd (pt->x, pt->y);

    g_hash_table_insert (map, opt, pt);
    g_ptr_array_add (D, opt);
  }

  P2tCDT *cdt = p2t_cdt_new (D);
  p2t_cdt_triangulate (cdt);

  P2tTrianglePtrArray pt = p2t_cdt_get_triangles (cdt);
  int i;
  for (i = 0; i < pt->len; i++)
    {
      P2tTriangle *t = triangle_index (pt,i);
      P2tRPoint *p0 = g_hash_table_lookup (map, p2t_triangle_get_point (t, 0));
      P2tRPoint *p1 = g_hash_table_lookup (map, p2t_triangle_get_point (t, 1));
      P2tRPoint *p2 = g_hash_table_lookup (map, p2t_triangle_get_point (t, 2));
      p2tr_triangle_new (p2tr_point_edge_to (p0, p1),
                         p2tr_point_edge_to (p1, p2),
                         p2tr_point_edge_to (p2, p0), T);
    }

  p2t_cdt_free (cdt);

  for (i = 0; i < D->len; i++)
    p2t_point_free (point_index (D, i));

  g_ptr_array_free (D, TRUE);

  g_hash_table_destroy (map);
  
  return T;

}

P2tRTriangulation *
p2tr_triangulation_new ()
{
  P2tRTriangulation *self = g_slice_new (P2tRTriangulation);
  self->tris = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
  self->pts = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
  self->edges = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
  return self;
}

P2tRTriangulation*
p2tr_triangulation_free (P2tRTriangulation *self)
{
  P2trHashSetIter siter;

  P2tRTriangle *tr;
  P2tREdge     *ed;
  P2tRPoint    *pt;

  p2tr_hash_set_iter_init (&siter, self->tris);
  while (p2tr_hash_set_iter_next (&siter, (gpointer*) &tr))
    {
      p2tr_triangle_unref (tr);
    }

  p2tr_hash_set_iter_init (&siter, self->edges);
  while (p2tr_hash_set_iter_next (&siter, (gpointer*) &ed))
    {
      p2tr_edge_unref (ed);
    }

  p2tr_hash_set_iter_init (&siter, self->pts);
  while (p2tr_hash_set_iter_next (&siter, (gpointer*) &pt))
    {
      p2tr_point_unref (pt);
    }

  g_hash_table_destroy (self->tris);
  g_hash_table_destroy (self->edges);
  g_hash_table_destroy (self->pts);

  g_slice_free (P2tRTriangulation, self);
}

P2tRTriangulation*
p2tr_triangulateA (P2tRPoint **p2trpoints, gint count)
{
  GList *A = NULL;
  P2tRTriangulation *T;
  int i;
  for (i = 0; i < count; i++)
    {
      A = g_list_prepend (A, p2trpoints[count-i-1]);
    }

  T = p2tr_triangulate (A);

  g_list_free (A);

  return T;

}

#define DEBUG FALSE
gboolean
p2tr_validate_triangulation (P2tRTriangulation* T)
{
#if DEBUG
  P2trHashSetIter iter;
  GList *L;
  P2tRTriangle *t;

  p2tr_hash_set_iter_init (&iter, T->tris);

  while (p2tr_hash_set_iter_next (&iter, (gpointer*)&t))
    {
      int i;
      for (i = 0; i < 3; i++)
        {
          P2tRPoint *S = p2tr_edge_get_start (t->edges[i]);
          P2tRPoint *E = p2tr_edge_get_end (t->edges[i]);

          p2tr_assert_and_explain (p2tr_point_has_edge_to (S, E), "edge connectivity");
          p2tr_assert_and_explain (p2tr_point_has_edge_to (E, S), "edge connectivity");
          p2tr_assert_and_explain (t->edges[i]->tri == t, "triangle <-> edge");
        }
    }
#endif
  return TRUE;

}

gboolean
p2tr_validate_edge (P2tREdge* e)
{
  if (e->constrained != e->mirror->constrained)
    {
      G_BREAKPOINT();
      g_assert (FALSE);
    }
  if (e->tri == NULL && ! e->constrained)
    {
      G_BREAKPOINT();
      g_assert (FALSE);
    }

  if (e->mirror->constrained != e->mirror->mirror->constrained)
    {
      G_BREAKPOINT();
      g_assert (FALSE);
    }
  if (e->mirror->tri == NULL && ! e->mirror->constrained)
    {
      G_BREAKPOINT();
      g_assert (FALSE);
    }

  return TRUE;

}


void
p2tr_debug_point (P2tRPoint* pt, gboolean newline)
{
  p2tr_debug ("@PT(%g,%g)", pt->x, pt->y);
  if (newline) p2tr_debug ("\n");
}

void
p2tr_debug_edge (P2tREdge* ed, gboolean newline)
{
  p2tr_debug ("@ED(");
  p2tr_debug_point (p2tr_edge_get_start (ed), FALSE);
  p2tr_debug ("->");
  p2tr_debug_point (p2tr_edge_get_end (ed), FALSE);
  p2tr_debug (")");
  if (newline) p2tr_debug ("\n");
}

void
p2tr_debug_tri (P2tRTriangle* tr, gboolean newline)
{
  p2tr_debug ("@TR(");
  p2tr_debug_edge (tr->edges[0], FALSE);
  p2tr_debug ("~");
  p2tr_debug_edge (tr->edges[1], FALSE);
  p2tr_debug ("~");
  p2tr_debug_edge (tr->edges[2], FALSE);
  p2tr_debug (")");
  if (newline) p2tr_debug ("\n");
}

