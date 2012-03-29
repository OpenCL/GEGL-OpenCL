/* This file is an image processing operation for GEGL
 *
 * find-outline.c
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

/* This file is part 1 of 3 in the seamless clone operation. The logic
 * in this file allows to find the outline of a given image. A pixel is
 * considered an outline pixel, if it's alpha is larger than 0.5 and one
 * of it's neighbors has alpha lower than that.
 *
 * Currently, the logic in this file allows for finding one outline with
 * no holes, and it currently assumse that only one "island" of high
 * alpha exists
 */

#include <gegl.h>
#include <stdio.h> /* TODO: get rid of this debugging way! */

#include "seamless-clone.h"
#include "poly2tri-c/poly2tri.h"
#include "poly2tri-c/refine/triangulation.h"

/* Define the directions
 *
 *          0
 *         
 *    7     N     1
 *          ^
 *          |
 *          |           
 * 6  W<----+---->E  2
 *          |              =====>X
 *          |             ||
 *          v             ||
 *    5     S     3       ||
 *                        vv
 *          4             Y
 */
typedef enum {
  D_N = 0,    /* 000 */
  D_NE = 1,   /* 001 */
  D_E = 2,    /* 010 */
  D_SE = 3,   /* 011 */
  D_S = 4,    /* 100 */
  D_SW = 5,   /* 101 */
  D_W = 6,    /* 110 */
  D_NW = 7    /* 111 */
} OUTLINE_DIRECTION;

#define cwdirection(t)       (((t)+1)%8)
#define ccwdirection(t)      (((t)+7)%8)
#define oppositedirection(t) (((t)+4)%8)

#define isSouth(s) (((s) == D_S) || ((s) == D_SE) || ((s) == D_SW))
#define isNorth(s) (((s) == D_N) || ((s) == D_NE) || ((s) == D_NW))

#define isEast(s) (((s) == D_NE) || ((s) == D_E) || ((s) == D_SE))
#define isWest(s) (((s) == D_NW) || ((s) == D_W) || ((s) == D_SW))

/**
 * Add a COPY of the given point into the array pts. The original point CAN
 * be freed later!
 */
static inline void
add_point (GPtrArray* pts, ScPoint *pt)
{
  ScPoint *cpy = g_slice_new (ScPoint);
  cpy->x = pt->x;
  cpy->y = pt->y;

  g_ptr_array_add (pts, cpy);
}

static inline void
spoint_move (ScPoint *pt, OUTLINE_DIRECTION t, ScPoint *dest)
{
  dest->x = pt->x + (isEast(t) ? 1 : (isWest(t) ? -1 : 0));
  dest->y = pt->y + (isSouth(t) ? 1 : (isNorth(t) ? -1 : 0));
}

static inline gboolean
in_range(gint val,gint min,gint max)
{
  return (((min) <= (val)) && ((val) <= (max)));
}

static inline gfloat
sc_sample_alpha (GeglBuffer *buf, gint x, gint y, Babl *format)
{
  gfloat col[4] = {0, 0, 0, 0};
  gegl_buffer_sample (buf, x, y, NULL, col, format, GEGL_SAMPLER_NEAREST);
  return col[3];
}

static inline gboolean
is_opaque (GeglRectangle *rect,
           GeglBuffer    *pixels,
           Babl          *format,
           ScPoint       *pt)
{
  g_assert (pt != NULL);
  g_assert (rect != NULL);

  return in_range(pt->x, rect->x, rect->x + rect->width - 1)
         && in_range(pt->y, rect->y, rect->y + rect->height - 1)
         && (sc_sample_alpha (pixels, pt->x, pt->y, format) >= 0.5f);
}

/* This function receives a white pixel (pt) and the direction of the movement
 * that lead to it (prevdirection). It will return the direction leading to the
 * next white pixel (while following the edges in CW order), and the pixel
 * itself would be stored in dest.
 *
 * The logic is simple:
 * 1. Try to continue in the same direction that lead us to the current pixel
 * 2. Dprev = oppposite(prevdirection)
 * 3. Dnow  = cw(Dprev)
 * 4. While moving to Dnow is white:
 * 4.1. Dprev = Dnow
 * 4.2. Dnow  = cw(D)
 * 5. Return the result - moving by Dprev
 */
static inline OUTLINE_DIRECTION
outline_walk_cw (GeglRectangle      *rect,
                 GeglBuffer         *pixels,
                 Babl               *format,
                 OUTLINE_DIRECTION   prevdirection,
                 ScPoint            *pt,
                 ScPoint            *dest)
{
  OUTLINE_DIRECTION Dprev = oppositedirection(prevdirection);
  OUTLINE_DIRECTION Dnow = cwdirection (Dprev);
 
  ScPoint ptN, ptP;

  spoint_move (pt, Dprev, &ptP);
  spoint_move (pt, Dnow, &ptN);

  while (is_opaque (rect, pixels, format, &ptN))
    {
       Dprev = Dnow;
       ptP.x = ptN.x;
       ptP.y = ptN.y;
       Dnow = cwdirection (Dprev);
       spoint_move (pt, Dnow, &ptN);
    }

  dest->x = ptP.x;
  dest->y = ptP.y;
  return Dprev;
}

#define pteq(pt1,pt2) (((pt1)->x == (pt2)->x) && ((pt1)->y == (pt2)->y))

GPtrArray*
sc_outline_find_ccw (const GeglRectangle *rect,
                     const GeglBuffer    *pixels)
{
  Babl      *format = babl_format("RGBA float");
  GPtrArray *points = g_ptr_array_new ();
  
  gint x = rect->x, y;

  gboolean found = FALSE;
  ScPoint START, pt, ptN;
  OUTLINE_DIRECTION DIR, DIRN;

  /* First of all try to find a white pixel */
  for (y = rect->y; y < rect->y + rect->height; y++)
    {
      for (x = rect->x; x < rect->x + rect->width; x++)
        {
          if (sc_sample_alpha (pixels, x, y, format) >= 0.5f)
            {
               found = TRUE;
               break;
            }
        }
      if (found) break;
    }

  if (!found)
    return points;
  
  pt.x = START.x = x;
  pt.y = START.y = y;
  DIR = D_NW;

  add_point (points, &START);

  DIRN = outline_walk_cw (rect, pixels, format, DIR,&pt,&ptN);

  while (! pteq(&ptN,&START))
    {
      add_point (points, &ptN);
      pt.x = ptN.x;
      pt.y = ptN.y;
      DIR = DIRN;
      DIRN = outline_walk_cw (rect, pixels, format, DIR,&pt,&ptN);
    }

  return points;
}

void
sc_outline_free (ScOutline *self)
{
	GPtrArray *real = (GPtrArray*) self;
	gint i;
	for (i = 0; i < real->len; i++)
	  g_slice_free (ScPoint, g_ptr_array_index (self, i));
	g_ptr_array_free (real, TRUE);
}
