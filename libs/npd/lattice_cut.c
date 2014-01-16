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

#include "lattice_cut.h"
#include "npd_common.h"
#include "graphics.h"
#include <glib.h>
#include "npd_math.h"

#define NPD_SWAP_INTS(i,j) { gint tmp = i; i = j; j = tmp; }

/* only works for straight lines */
static gboolean
npd_is_edge_empty (NPDImage *image,
                   gint      X1,
                   gint      Y1,
                   gint      X2,
                   gint      Y2)
{
  gint x, y;
  NPDColor color;

  if (Y1 > Y2) NPD_SWAP_INTS (Y1, Y2);
  if (X1 > X2) NPD_SWAP_INTS (X1, X2);

  for (y = Y1; y <= Y2; y++)
  for (x = X1; x <= X2; x++)
    {
      npd_get_pixel_color (image, x, y, &color);
      if (!npd_is_color_transparent (&color))
        return FALSE;
    }

  return TRUE;
}

GList**
npd_find_edges (NPDImage *image,
                gint      count_x,
                gint      count_y,
                gint      square_size)
{
  gint i, j;
  gint ow = count_x + 1,
       oh = count_y + 1;
  GList **empty_edges = g_new0 (GList*, ow * oh);

  for (j = 1; j <= count_y; j++)
  for (i = 1; i <= count_x; i++)
    {
#define NPD_TEST_EMPTY(from_op_x,from_op_y,to_op_x,to_op_y)                         \
      if (npd_is_edge_empty (image,                                                 \
                             (from_op_x) * square_size, (from_op_y) * square_size,  \
                             (to_op_x)   * square_size, (to_op_y)   * square_size)) \
        {                                                                           \
          gint from_op_id = (from_op_y) * ow + (from_op_x),                         \
               to_op_id   = (to_op_y)   * ow + (to_op_x);                           \
          empty_edges[from_op_id] = g_list_append (empty_edges[from_op_id],         \
                                                   GINT_TO_POINTER (to_op_id));     \
          empty_edges[to_op_id]   = g_list_append (empty_edges[to_op_id],           \
                                                   GINT_TO_POINTER (from_op_id));   \
        }

      if (j != count_y) NPD_TEST_EMPTY (i, j, i - 1, j);
      if (i != count_x) NPD_TEST_EMPTY (i, j, i,     j - 1);
#undef NPD_TEST_EMPTY
    }

  return empty_edges;
}

GList*
npd_cut_edges (GList **edges,
               gint    ow,
               gint    oh)
{
  gint i, r, col, width = ow - 1;
  GList *ops = NULL;
  gint neighbors[4];

  for (r = 0; r < oh; r++)
  for (col = 0; col < ow; col++)
    {
      gint index = r * ow + col;
      GList *op = edges[index];
      gint num_of_neighbors = g_list_length (op);

      if (num_of_neighbors == 0) continue;

#define NPD_ADD_COUNT(count) ops = g_list_prepend (ops, GINT_TO_POINTER (count))
#define NPD_ADD_P(r,col,point)                                                 \
      if ((r) > -1 && (r) < (oh - 1) && (col) > -1 && (col) < (ow - 1))        \
        {                                                                      \
          ops = g_list_prepend (ops, GINT_TO_POINTER ((r) * width + (col)));   \
          ops = g_list_prepend (ops, GINT_TO_POINTER (point));                 \
        }

      for (i = 0; i < num_of_neighbors; i++)
        neighbors[i] = GPOINTER_TO_INT (g_list_nth_data (op, i));

      if (num_of_neighbors == 1)
        {
          gboolean border = FALSE;
          if (r == 0 || col == 0 || r == (oh - 1) || col == (ow - 1))
            border = TRUE;

          if (border) NPD_ADD_COUNT (1);
          else        NPD_ADD_COUNT (4);
          NPD_ADD_P (r - 1, col - 1, 2);
          NPD_ADD_P (r - 1, col,     3);
          NPD_ADD_P (r,     col,     0);
          NPD_ADD_P (r,     col - 1, 1);
          if (border) ops = g_list_insert (ops, GINT_TO_POINTER (1), 2);
#undef NPD_ADD_P
        }
      else
      if (num_of_neighbors == 2)
        {
          gboolean x_differs = FALSE, y_differs = FALSE;
          gint a, b, c, d;

#define NPD_OP_X(op) ((op) % ow)
#define NPD_OP_Y(op) ((op) / ow)
          if (NPD_OP_X (neighbors[0]) != NPD_OP_X (neighbors[1])) x_differs = TRUE;
          if (NPD_OP_Y (neighbors[0]) != NPD_OP_Y (neighbors[1])) y_differs = TRUE;

          if (x_differs && y_differs)
            {
              /* corner */
              gint B = neighbors[0], C = neighbors[1];

              if (NPD_OP_X (index) == NPD_OP_X (neighbors[0]))
                { B = neighbors[1]; C = neighbors[0]; }

              if (NPD_OP_Y (index) < NPD_OP_Y (C))
                {
                  if (NPD_OP_X (index) < NPD_OP_X (B))
                    { /* IV. quadrant */  a = 2; b = 3; c = 1; d = 0; }
                  else
                    { /* III. quadrant */ a = 2; b = 3; c = 0; d = 1; }
                }
              else
                {
                  if (NPD_OP_X (index) < NPD_OP_X (B))
                    { /* I. quadrant */   a = 2; b = 0; c = 1; d = 3; }
                  else
                    { /* II. quadrant */  a = 0; b = 3; c = 1; d = 2; }
                }

#define NPD_OP2SQ(op) (op == 0 ? ( r      * width + col) :                     \
                      (op == 1 ? ( r      * width + col - 1) :                 \
                      (op == 2 ? ((r - 1) * width + col - 1) :                 \
                                 ((r - 1) * width + col))))
#define NPD_ADD_P(square,point)                                                \
              ops = g_list_prepend (ops, GINT_TO_POINTER (square));            \
              ops = g_list_prepend (ops, GINT_TO_POINTER (point));

              NPD_ADD_COUNT (3);
              NPD_ADD_P (NPD_OP2SQ (a), a);
              NPD_ADD_P (NPD_OP2SQ (b), b);
              NPD_ADD_P (NPD_OP2SQ (c), c);
              NPD_ADD_COUNT (1);
              NPD_ADD_P (NPD_OP2SQ (d), d);
            }
          else
            {
              /* segment */
              a = 0; b = 1; c = 2; d = 3;
              if (y_differs) { a = 1; b = 2; c = 0; d = 3; }

              NPD_ADD_COUNT (2);
              NPD_ADD_P (NPD_OP2SQ (a), a);
              NPD_ADD_P (NPD_OP2SQ (b), b);
              NPD_ADD_COUNT (2);
              NPD_ADD_P (NPD_OP2SQ (c), c);
              NPD_ADD_P (NPD_OP2SQ (d), d);
            }
        }
      else
      if (num_of_neighbors == 3)
        {
          gint B = neighbors[0], C = neighbors[1], D = neighbors[2];
          gint a = 2, b = 1, c = 3, d = 0;

          if ((NPD_OP_X (B) != NPD_OP_X (C)) && (NPD_OP_Y (B) != NPD_OP_Y (C)))
            {
              /* B and C form corner */
              B = neighbors[2]; D = neighbors[0]; /* swap B and D */

              if ((NPD_OP_X (B) != NPD_OP_X (C)) && (NPD_OP_Y (B) != NPD_OP_Y (C)))
                {
                  /* (new) B and C form corner */
                  C = neighbors[0]; D = neighbors[1]; /* swap C and D */
                }
            }

          /* B and C form segment */
          if (NPD_OP_X (B) == NPD_OP_X (C))
            {
              if (NPD_OP_X (B) < NPD_OP_X (D))
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
          else if (NPD_OP_Y (B) == NPD_OP_Y (C))
            {
              if (NPD_OP_Y (B) < NPD_OP_Y (D))
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

          NPD_ADD_COUNT (2);
          NPD_ADD_P (NPD_OP2SQ (a), a);
          NPD_ADD_P (NPD_OP2SQ (b), b);
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (c), c);
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (d), d);
        }
      else
      if (num_of_neighbors == 4)
        {
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (0), 0);
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (1), 1);
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (2), 2);
          NPD_ADD_COUNT (1);
          NPD_ADD_P (NPD_OP2SQ (3), 3);
        }
    }
#undef NPD_ADD_P
#undef NPD_OP2SQ
#undef NPD_OP_X
#undef NPD_OP_Y
#undef NPD_ADD_COUNT

  /* because of efficiency, we've used g_list_prepend instead of g_list_append,
   * thus at the end, we have to reverse the list */
  return g_list_reverse (ops);
}
