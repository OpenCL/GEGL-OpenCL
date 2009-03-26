/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2003, 2004, 2006, 2007, 2008 Øyvind Kolås
 */

#include "config.h"
#include "gegl-path-spiro.h"
#include <gegl.h>
#include <math.h>

#include <spiroentrypoints.h>

struct {
	/* Called by spiro to start a contour */
    void (*moveto)(bezctx *bc, double x, double y, int is_open);

	/* Called by spiro to move from the last point to the next one on a straight line */
    void (*lineto)(bezctx *bc, double x, double y);

	/* Called by spiro to move from the last point to the next along a quadratic bezier spline */
	/* (x1,y1) is the quadratic bezier control point and (x2,y2) will be the new end point */
    void (*quadto)(bezctx *bc, double x1, double y1, double x2, double y2);

	/* Called by spiro to move from the last point to the next along a cubic bezier spline */
	/* (x1,y1) and (x2,y2) are the two off-curve control point and (x3,y3) will be the new end point */
    void (*curveto)(bezctx *bc, double x1, double y1, double x2, double y2,
		    double x3, double y3);

	/* I'm not entirely sure what this does -- I just leave it blank */
    void (*mark_knot)(bezctx *bc, int knot_idx);
    GeglPathList *path;
} bezcontext;

static void moveto (bezctx *bc, double x, double y, int is_open)
{
  bezcontext.path = gegl_path_list_append (bezcontext.path, 'M', x, y);
}
static void lineto (bezctx *bc, double x, double y)
{
  bezcontext.path = gegl_path_list_append (bezcontext.path, 'L', x, y);
}
static void quadto (bezctx *bc, double x1, double y1, double x2, double y2)
{
  g_print ("%s\n", G_STRFUNC);
}
static void curveto(bezctx *bc,
                    double x1, double y1,
                    double x2, double y2,
 		    double x3, double y3)
{
  bezcontext.path = gegl_path_list_append (bezcontext.path, 'C', x1, y1, x2, y2, x3, y3);
}

static GeglPathList *gegl_path_spiro_flatten (GeglPathList *original)
{
  GeglPathList *iter;
  spiro_cp *points;
  gboolean is_spiro = TRUE;
  gboolean closed = FALSE;
  gint count;
  gint i;
  /* first we do a run through the path checking it's length
   * and determining whether we can flatten the incoming path
   */
  for (count=0,iter = original; iter; iter=iter->next)
    {
      switch (iter->d.type)
        {
          case 'z':
            closed = TRUE;
          case 'v':
          case 'o':
          case 'O':
          case '[':
          case ']':
          case '{':
            break;
          default:
            is_spiro=FALSE;
            break;
        }
      count ++;
    }


  if (!is_spiro)
    {
      return original;
    }

  points = g_new0 (spiro_cp, count);

  iter = original;

  for (i=0; iter; iter=iter->next, i++)
    {
      if (iter->d.type == 'z')
        continue;
      points[i].x = iter->d.point[0].x;
      points[i].y = iter->d.point[0].y;
      switch (iter->d.type)
        {
          case 'C':
            points[i].x = iter->d.point[2].x;
            points[i].y = iter->d.point[2].y;
            points[i].ty = SPIRO_G4;
            break;
          case 'L':
            points[i].ty = SPIRO_G4;
            break;
          case 'v':
            points[i].ty = SPIRO_CORNER;
            break;
          case 'o':
            points[i].ty = SPIRO_G4;
            break;
          case 'O':
            points[i].ty = SPIRO_G2;
            break;
          case '[':
            points[i].ty = SPIRO_LEFT;
            break;
          case ']':
            points[i].ty = SPIRO_RIGHT;
            break;
          case '{':
            points[i].ty = SPIRO_OPEN_CONTOUR;
            break;
          case '0':
            points[i].ty = SPIRO_G2;
            break;
          case 'V':
            points[i].ty = SPIRO_CORNER;
            break;
          case 'z':
            break;
          /*case '}':
            points[i].ty = SPIRO_END_CONTOUR;
            break;*/
          default:
            points[i].ty = SPIRO_G4;
            break;
        }
    }

  bezcontext.moveto = moveto;
  bezcontext.lineto = lineto;
  bezcontext.curveto = curveto;
  bezcontext.quadto = quadto;
  bezcontext.path = NULL;
  SpiroCPsToBezier(points,count - (closed?1:0), closed, (void*)&bezcontext);
  g_free (points);

  return bezcontext.path;
}

void gegl_path_spiro_init (void)
{
  static gboolean done = FALSE;
  if (done)
    return;
  done = TRUE;
  gegl_path_add_type ('v', 2, "spiro corner");
  gegl_path_add_type ('o', 2, "spiro g4");
  gegl_path_add_type ('O', 2, "spiro g2");
  gegl_path_add_type ('[', 2, "spiro left");
  gegl_path_add_type (']', 2, "spiro right");

  gegl_path_add_flattener (gegl_path_spiro_flatten);
}
