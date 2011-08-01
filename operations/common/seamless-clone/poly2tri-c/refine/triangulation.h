/*
 * This file is a part of Poly2Tri-C - The C port of the Poly2Tri library
 * Porting to C done by (c) Barak Itkin <lightningismyname@gmail.com>
 * http://code.google.com/p/poly2tri-c/
 *
 * Poly2Tri Copyright (c) 2009-2010, Poly2Tri Contributors
 * http://code.google.com/p/poly2tri/
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * * Neither the name of Poly2Tri nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without specific
 *   prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __POLY2TRI_C_REFINE_TRIANGULATION_H__
#define	__POLY2TRI_C_REFINE_TRIANGULATION_H__

#ifdef	__cplusplus
extern "C"
{
#endif

#include <glib.h>
#include "../common/utils.h"
#include "utils.h"
#include <stdio.h>

#define EPSILON2 (1e-6)

#define p2tr_debug //g_printerr

#define p2tr_assert_and_explain(expr,err)  \
do {                                       \
  if (!(expr))                             \
    {                                      \
      g_warning (err);                     \
      g_assert (FALSE);                    \
    }                                      \
} while (FALSE)


typedef struct P2tRTriangle_       P2tRTriangle;
typedef struct P2tREdge_           P2tREdge;
typedef struct P2tRPoint_          P2tRPoint;
typedef struct P2tRTriangulation_  P2tRTriangulation;
typedef struct P2tRCircle_         P2tRCircle;

/* ########################################################################## */
/*                              Common math                                   */
/* ########################################################################## */

gdouble p2tr_math_normalize_angle (gdouble angle);

/* TODO: try to somehow merge with the matching functions in utils.c */
typedef P2tOrientation P2tROrientation;

typedef enum
{
  INCIRCLE_ON,
  INCIRCLE_INSIDE,
  INCIRCLE_OUTSIDE
} P2tRInCircle;

P2tROrientation p2tr_math_orient2d (P2tRPoint* pa, P2tRPoint* pb, P2tRPoint* pc);

P2tRInCircle    p2tr_math_incircle (P2tRPoint* a, P2tRPoint* b, P2tRPoint* c, P2tRPoint* d);

gdouble p2tr_math_edge_len_sq (gdouble x1, gdouble y1, gdouble x2, gdouble y2);

/* ########################################################################## */
/*                           Triangulation struct                             */
/* ########################################################################## */

struct P2tRTriangulation_
{
  P2tRHashSet *pts;
  P2tRHashSet *edges;
  P2tRHashSet *tris;
};

void p2tr_triangulation_remove_pt (P2tRTriangulation *self, P2tRPoint *pt);
void p2tr_triangulation_remove_ed (P2tRTriangulation *self, P2tREdge *ed);
void p2tr_triangulation_remove_tr (P2tRTriangulation *self, P2tRTriangle *tr);

void p2tr_triangulation_add_pt (P2tRTriangulation *self, P2tRPoint *pt);
void p2tr_triangulation_add_ed (P2tRTriangulation *self, P2tREdge *ed);
void p2tr_triangulation_add_tr (P2tRTriangulation *self, P2tRTriangle *tr);

/* ########################################################################## */
/*                               Point struct                                 */
/* ########################################################################## */

struct P2tRPoint_
{
  gdouble   x;
  gdouble   y;
  GList    *edges;
  guint     _refcount;
};

static void p2tr_point_init (P2tRPoint *self, gdouble x, gdouble y);

P2tRPoint* p2tr_point_new (gdouble x, gdouble y);

static void p2tr_point_add_edge (P2tRPoint *self, P2tREdge  *edge);

void p2tr_point_remove_edge (P2tRPoint *self, P2tREdge  *edge);

void p2tr_point_remove (P2tRPoint *self, P2tRTriangulation *T);

P2tREdge* p2tr_point_edge_to (P2tRPoint *self, P2tRPoint *end);

P2tREdge* p2tr_point_has_edge_to (P2tRPoint *self, P2tRPoint *end);

P2tREdge* p2tr_point_edgeccw (P2tRPoint *self, P2tREdge *edge);

P2tREdge* p2tr_point_edgecw (P2tRPoint *self, P2tREdge *edge);

gboolean p2tr_point_is_in_cluster (P2tRPoint *self, P2tREdge *e);

GList* p2tr_point_get_cluster (P2tRPoint *self, P2tREdge *e, gdouble *angle);

gboolean p2tr_point_is_fully_in_domain (P2tRPoint *self);

void p2tr_point_free (P2tRPoint *self);

#define p2tr_point_ref(pt) ((pt)->_refcount++)

#define p2tr_point_unref(pt)    \
do {                            \
  if ((--(pt)->_refcount) == 0) \
    {                           \
      p2tr_point_free ((pt));   \
    }                           \
} while (FALSE)


/* ########################################################################## */
/*                               Edge struct                                  */
/* ########################################################################## */

#define P2TR_EDGE(e) ((P2tREdge*)(e))

struct P2tREdge_
{
  P2tRPoint    *end;
  gdouble       angle;

  P2tREdge     *mirror;
  P2tRTriangle *tri;

  gboolean      delaunay;
  gboolean      constrained;

  gboolean      removed;

  /* Note that this count does not include the pointing from the mirror edge */
  guint     _refcount;
};

P2tREdge* p2tr_edge_new (P2tRPoint *start, P2tRPoint *end);

P2tREdge* p2tr_edge_new_private (P2tRPoint *start, P2tRPoint *end, gboolean mirror);

static void p2tr_edge_init (P2tREdge *self, P2tRPoint *start, P2tRPoint *end);

static void p2tr_edge_init_private (P2tREdge *self, P2tRPoint *start, P2tRPoint *end, gboolean mirror);

void p2tr_edge_remove (P2tREdge *self, P2tRTriangulation *T);

static void p2tr_edge_remove_private (P2tREdge *self, P2tRTriangulation *T);

gboolean p2tr_edge_is_encroached_by (P2tREdge *self, P2tRPoint *other);

gboolean p2tr_edge_is_encroached (P2tREdge *self);

gboolean p2tr_edge_diametral_lens_contains (P2tREdge *self, P2tRPoint *W);

gboolean p2tr_edge_diametral_circle_contains (P2tREdge *self, P2tRPoint *pt);

gdouble p2tr_edge_len_sq (P2tREdge *self);

void p2tr_edge_set_constrained (P2tREdge *self, gboolean b);

void p2tr_edge_set_delaunay (P2tREdge *self, gboolean b);

/* Note that you can't really free one edge; Freeing will happen when both
 * have no references to
 */
void p2tr_edge_free (P2tREdge *self);

#define p2tr_edge_ref(ed) ((ed)->_refcount++)

#define p2tr_edge_unref(ed)     \
do {                            \
  if ((--(ed)->_refcount) == 0) \
    {                           \
      p2tr_edge_free ((ed));    \
    }                           \
} while (FALSE)

#define p2tr_edgelist_ccw(elist,e) P2TR_EDGE(g_list_cyclic_next((elist),(e))->data)
#define p2tr_edgelist_cw(elist,e)  P2TR_EDGE(g_list_cyclic_prev((elist),(e))->data)

#define p2tr_edge_get_start(e) ((e)->mirror->end)
#define p2tr_edge_get_end(e)   ((e)->end)

/* ########################################################################## */
/*                            Triangle struct                                 */
/* ########################################################################## */

struct P2tRTriangle_
{
  P2tREdge     *edges[3];
  guint         _refcount;
};

struct P2tRCircle_
{
  gdouble x;
  gdouble y;

  gdouble radius;
};

gdouble p2tr_angle_between (P2tREdge *e1, P2tREdge *e2);

void p2tr_triangle_init (P2tRTriangle *self, P2tREdge *e1, P2tREdge *e2, P2tREdge *e3, P2tRTriangulation *T);

P2tRTriangle* p2tr_triangle_new (P2tREdge *e1, P2tREdge *e2, P2tREdge *e3, P2tRTriangulation *T);

void p2tr_triangle_free (P2tRTriangle *self);

P2tRPoint* p2tr_triangle_opposite_point (P2tRTriangle *self, P2tREdge *e);

P2tREdge* p2tr_triangle_opposite_edge (P2tRTriangle *self, P2tRPoint *pt);

gdouble p2tr_triangle_smallest_non_seperating_angle (P2tRTriangle *self);

gdouble p2tr_triangle_shortest_edge_len (P2tRTriangle *self);

gdouble p2tr_triangle_longest_edge_len (P2tRTriangle *self);

void p2tr_triangle_angles (P2tRTriangle *self, gdouble dest[3]);

gdouble p2tr_triangle_get_angle_at (P2tRTriangle *self, P2tRPoint* pt);

void p2tr_triangle_remove (P2tRTriangle *self, P2tRTriangulation *T);

void p2tr_triangle_circumcircle (P2tRTriangle *self, P2tRCircle *dest);

gboolean p2tr_triangle_is_circumcenter_inside (P2tRTriangle *self);

void p2tr_triangle_subdivide (P2tRTriangle *self, P2tRPoint *C, P2tRTriangulation *T, P2tRTriangle *dest_new[3]);

gboolean p2tr_triangle_contains_pt (P2tRTriangle *self, P2tRPoint *P);

void p2tr_triangulation_flip_fix (P2tREdge *e, P2tRTriangulation *T);

gboolean p2tr_edges_intersect (P2tREdge *e1, P2tREdge *e2);

P2tRPoint *p2tr_triangle_median_pt (P2tRTriangle *self);

P2tRPoint *p2tr_edge_concentric_center (P2tREdge *e);

P2tRTriangulation* p2tr_triangulate (GList *p2trpoints);

P2tRTriangulation* p2tr_triangulateA (P2tRPoint **p2trpoints, gint count);

gboolean p2tr_validate_triangulation (P2tRTriangulation* T);

gboolean p2tr_validate_edge (P2tREdge* e);

gboolean p2tr_false_delta (P2tRTriangle *t);

#define p2tr_triangle_ref(tr) ((tr)->_refcount++)

#define p2tr_triangle_unref(tr)  \
do {                             \
  if ((--(tr)->_refcount) == 0)  \
    {                            \
      p2tr_triangle_free ((tr)); \
    }                            \
} while (FALSE)





P2tRTriangulation*
p2tr_triangulation_free (P2tRTriangulation *self);

P2tRTriangulation*
p2tr_triangulation_new ();




void p2tr_debug_point (P2tRPoint* pt, gboolean newline);

void p2tr_debug_edge (P2tREdge* ed, gboolean newline);

void p2tr_debug_tri (P2tRTriangle* tr, gboolean newline);


#ifdef	__cplusplus
}
#endif

#endif	/* TRIANGULATION_H */