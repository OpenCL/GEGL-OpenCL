#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glib.h>

#include "../refine/triangulation.h"

#include "svg-plot.h"

void
p2tr_plot_svg_plot_group_start (const gchar *Name, FILE *outfile)
{
  if (Name == NULL)
    fprintf (outfile, "<g>" "\n");
  else
    fprintf (outfile, "<g name=\"%s\">" "\n", Name);
}

void
p2tr_plot_svg_plot_group_end (FILE *outfile)
{
  fprintf (outfile, "</g>" "\n");
}

void
p2tr_plot_svg_plot_line (gdouble x1, gdouble y1, gdouble x2, gdouble y2, const gchar *color, FILE *outfile)
{
  fprintf (outfile, "<line x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"" "\n", x1, y1, x2, y2);
  fprintf (outfile, "style=\"stroke: %s; stroke-width: %f\" />" "\n", color, PLOT_LINE_WIDTH);
  fprintf (outfile, "" "\n");
}

void
p2tr_plot_svg_plot_arrow (gdouble x1, gdouble y1, gdouble x2, gdouble y2, const gchar* color, FILE *outfile)
{
  p2tr_plot_svg_plot_line (x1, y1, x2, y2, color, outfile);

  gdouble dy = y2 - y1;
  gdouble dx = x2 - x1;
  gdouble angle = atan2 (dy, dx);

  gdouble temp = angle - ARROW_SIDE_ANGLE;
  p2tr_plot_svg_plot_line (x2, y2, x2 - ARROW_HEAD_SIZE * cos (temp), y2 - ARROW_HEAD_SIZE * sin (temp), color, outfile);

  temp = angle + ARROW_SIDE_ANGLE;
  p2tr_plot_svg_plot_line (x2, y2, x2 - ARROW_HEAD_SIZE * cos (temp), y2 - ARROW_HEAD_SIZE * sin (temp), color, outfile);
}

void
p2tr_plot_svg_fill_triangle (gdouble x1, gdouble y1, gdouble x2, gdouble y2, gdouble x3, gdouble y3, const gchar *color, FILE *outfile)
{
  fprintf (outfile, "<polyline points=\"%f,%f %f,%f %f,%f\"" "\n", x1, y1, x2, y2, x3, y3);
  fprintf (outfile, "style=\"fill: %s\" />" "\n", color);
  fprintf (outfile, "" "\n");
}

void
p2tr_plot_svg_fill_point (gdouble x1, gdouble y1, const gchar* color, FILE *outfile)
{
  fprintf (outfile, "<circle cx=\"%f\" cy=\"%f\" r=\"%f\"" "\n", x1, y1, MAX (1, PLOT_LINE_WIDTH));
  fprintf (outfile, "style=\"fill: %s; stroke: none\" />" "\n", color);
  fprintf (outfile, "" "\n");
}

void
p2tr_plot_svg_plot_circle (gdouble xc, gdouble yc, gdouble R, const gchar* color, FILE *outfile)
{
  fprintf (outfile, "<circle cx=\"%f\" cy=\"%f\" r=\"%f\"" "\n", xc, yc, R);
  fprintf (outfile, "style=\"stroke: %s; stroke-width: %f; fill: none\" />" "\n", color, PLOT_LINE_WIDTH);
  fprintf (outfile, "" "\n");
}

void
p2tr_plot_svg_plot_end (FILE *outfile)
{
  fprintf (outfile, "</g>" "\n");
  fprintf (outfile, "</svg>" "\n");
}

void
p2tr_plot_svg_plot_init (FILE *outfile)
{
  fprintf (outfile, "<?xml version=\"1.0\" standalone=\"no\"?>" "\n");
  fprintf (outfile, "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\"" "\n");
  fprintf (outfile, "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" "\n");
  fprintf (outfile, "<svg width=\"100%%\" height=\"100%%\" version=\"1.1\"" "\n");
  fprintf (outfile, "xmlns=\"http://www.w3.org/2000/svg\">" "\n");
  fprintf (outfile, "" "\n");
  fprintf (outfile, "<defs>" "\n");
  fprintf (outfile, "  <marker id=\"arrow\" viewBox=\"0 0 10 10\" refX=\"10\" refY=\"5\"" "\n");
  fprintf (outfile, "     markerUnits=\"strokeWidth\" orient=\"auto\"" "\n");
  fprintf (outfile, "     markerWidth=\"12\" markerHeight=\"9\">" "\n");
  fprintf (outfile, "" "\n");
  fprintf (outfile, "     <polyline points=\"0,0 10,5 0,10\" fill=\"none\" stroke-width=\"2px\" stroke=\"inherit\" />" "\n");
  fprintf (outfile, "  </marker>" "\n");
  fprintf (outfile, "</defs>" "\n");
  fprintf (outfile, "" "\n");
  fprintf (outfile, "<g transform=\"translate(%f,%f)  scale(%f,-%f)\">" "\n", X_TRANSLATE, Y_TRANSLATE, X_SCALE, Y_SCALE);

  p2tr_plot_svg_plot_arrow (-20, 0, 100, 0, "black", outfile);
  p2tr_plot_svg_plot_arrow (0, -20, 0, 100, "black", outfile);
}

void
p2tr_plot_svg_plot_edge (P2tREdge *self, const gchar* color, FILE* outfile)
{
  gdouble x1 = p2tr_edge_get_start (self)->x;
  gdouble y1 = p2tr_edge_get_start (self)->y;
  gdouble x2 = p2tr_edge_get_end (self)->x;
  gdouble y2 = p2tr_edge_get_end (self)->y;
  gdouble R = sqrt ((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2)) / 2;

  p2tr_plot_svg_plot_line (x1, y1, x2, y2, color, outfile);


//  if (p2tr_edge_is_encroached (self))
//    p2tr_plot_svg_plot_circle ((x1 + x2) / 2, (y1 + y2) / 2, R, "red", outfile);
}

void
p2tr_plot_svg_plot_triangle (P2tRTriangle *self, const gchar* color, FILE* outfile)
{
  P2tRCircle c;
  p2tr_triangle_circumcircle (self, &c);
  p2tr_plot_svg_plot_edge (self->edges[0], color, outfile);
  p2tr_plot_svg_plot_edge (self->edges[1], color, outfile);
  p2tr_plot_svg_plot_edge (self->edges[2], color, outfile);
  p2tr_plot_svg_plot_circle (c.x, c.y, c.radius, "green", outfile);
  p2tr_plot_svg_fill_point (self->edges[0]->end->x, self->edges[0]->end->y, "blue", outfile);
  p2tr_plot_svg_fill_point (self->edges[1]->end->x, self->edges[1]->end->y, "blue", outfile);
  p2tr_plot_svg_fill_point (self->edges[2]->end->x, self->edges[2]->end->y, "blue", outfile);
}

void
CCWTest (FILE* outfile)
{
  P2tRPoint *A = p2tr_point_new (50, 50);
  gdouble C[8][2] = {
    {0, 0},
    {50, 0},
    {100, 0},
    {100, 50},
    {100, 100},
    {50, 100},
    {0, 100},
    {0, 50}
  };

  gint lenC = sizeof (C) / (2 * sizeof (gdouble));

  gint j;
  for (j = 0; j < lenC; j++)
    {
      gint k;
      do
        {
          k = rand () % lenC;
        }
      while (C[k][0] == INFINITY && C[k][1] == INFINITY);

      gdouble *h = C[k];

      p2tr_point_edge_to (A, p2tr_point_new (h[0], h[1]));

      h[0] = h[1] = INFINITY;
    }

  gint i = 0;
  GList *iter;

  foreach (iter, A->edges)
  {
    gchar color[18];
    P2tREdge *e = (P2tREdge*) iter->data;
    gint val = i * 255 / lenC;

    sprintf (color, "#%02x%02x%02x", val, val, val);
    p2tr_plot_svg_plot_edge (e, color, outfile);
    i++;
  }
}

void
TriangleClockwiseTest (FILE* outfile)
{

  const gchar * ecolors[3] = {"#ff0000", "#00ff00", "#0000ff"};
  const gchar * mecolors[3] = {"#770000", "#007700", "#000077"};
  const gchar * ptcolors[3] = {"#ff00ff", "#ffff00", "#00ffff"};

  const gchar * names[3] = {"A", "B", "C"};

  P2tRPoint * pts[3];
  int i;
  for (i = 0; i < 3; i++)
    {
      pts[i] = p2tr_point_new (r (), r ());
    }

  P2tREdge * edges[3];
  for (i = 0; i < 3; i++)
    {
      edges[i] = p2tr_edge_new (pts[i], pts[(i + 1) % 3]);
    }

  P2tRTriangle *tri = p2tr_triangle_new (edges[0], edges[1], edges[2], NULL);

  for (i = 0; i < 3; i++)
    {
      p2tr_plot_svg_plot_edge (tri->edges[i]->mirror, mecolors[i], outfile);
    }

  for (i = 0; i < 3; i++)
    {
      p2tr_plot_svg_plot_edge (tri->edges[i], ecolors[i], outfile);
    }

  for (i = 0; i < 3; i++)
    {
      P2tRPoint *P = p2tr_edge_get_start (tri->edges[i]);
      p2tr_plot_svg_fill_point (P->x, P->y, ptcolors[i], outfile);
    }
}

void
refineTest(FILE* outfile)
{
  /*
    gdouble RAW[5][2] = {{10,10},{50,50},{55,130},{0,100},{30,50}};
    P2tRPoint *X[5];
    gint N = 5;
  */
    gdouble RAW[10][2] = {{10,10},{30,30},{50,50},{52.5,90},{55,130},{27.5,115},{0,100},{15,75},{30,50},{20,30}};
    P2tRPoint *X[10];
    gint N = 10;

    GList *XEs = NULL;
    int i;

  for (i = 0; i < N; i++)
    {
      X[i] = p2tr_point_new (RAW[i][0], RAW[i][1]);
      p2tr_plot_svg_fill_point (RAW[i][0], RAW[i][1], "blue", outfile);
    }

    fprintf (stderr, "Preparing to work on %d points\n", N);
  for (i = 0; i < N; i++)
    {
      P2tREdge *E = p2tr_edge_new (X[N-i-1], X[N-1-((i+1)%N)]);
      XEs = g_list_prepend (XEs, E);
      p2tr_edge_set_constrained (E, TRUE);
    }

    P2tRTriangulation *T = p2tr_triangulateA (X,N);

    {
      GList *liter;
      foreach (liter, XEs)
        p2tr_validate_edge ((P2tREdge*)liter->data);
    }
    
    DelaunayTerminator (T,XEs,M_PI/6,p2tr_false_delta);

    {
      P2trHashSetIter iter;
      P2tRTriangle *t;
      p2tr_hash_set_iter_init (&iter, T->tris);
      while (p2tr_hash_set_iter_next (&iter, (gpointer*)&t))
        {
          p2tr_assert_and_explain (t != NULL, "NULL triangle found!\n");
          p2tr_assert_and_explain (t->edges[0] != NULL && t->edges[1] != NULL && t->edges[2] != NULL,
                                   "Removed triangle found!\n");
          p2tr_plot_svg_plot_triangle (t, "black", outfile);

          p2tr_validate_edge (t->edges[0]);
          p2tr_validate_edge (t->edges[1]);
          p2tr_validate_edge (t->edges[2]);

        }
    }

#if FALSE
    GPtrArray* points = mvc_findEdgePoints (T);
    //PlotPoints (points);

    P2tRPoint *testX = p2tr_point_new (40, 45);
    p2tr_plot_svg_fill_point (testX->x, testX->y, "red");
    /* Give special care for the part after the last point - it may have less
     * points than other parts */
    gint div = points->len / 16;
    P2tRHashSet *allPts = p2tr_hash_set_set_new (g_direct_hash, g_direct_equal, NULL);
    for (i = 0; i < 16; i++)
      {
        gint index1 = i * div;
        gint index2 = MIN ((i + 1) * div, points->len); /* In the last iteration, take the last */
        mvc_makePtList (testX, points, index1, index2, allPts);
      }

    {
      gint count = 0;
      P2trHashSetIter iter;
      P2tRPoint *pt;
      p2tr_hash_set_iter_init (&iter, allPts);
      while (p2tr_hash_set_iter_next (&iter, (gpointer*)&pt))
        {
          p2tr_plot_svg_fill_point (pt->x, pt->y, "orange");
          count++;
        }
      fprintf (stderr, "In total, had %d sample points\n", count);
    }
#endif
}


void
p2tr_plot_svg (P2tRTriangulation *T, FILE *outfile)
{
  P2trHashSetIter  siter;
  P2tRTriangle    *tr;

  p2tr_debug ("Starting to write SVG output\n");
  p2tr_plot_svg_plot_init (outfile);

  p2tr_hash_set_iter_init (&siter, T->tris);
  while (p2tr_hash_set_iter_next (&siter, (gpointer*)&tr))
    p2tr_plot_svg_plot_triangle (tr, "black", outfile);

  p2tr_plot_svg_plot_end (outfile);
  p2tr_debug ("Finished writing SVG output\n");
}