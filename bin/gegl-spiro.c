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

#include "gegl-spiro.h"
#include <gegl/gegl.h>
#include "gegl-path.h"
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
  gint count;
  gint i;
  /* first we do a run through the path checking it's length
   * and determining whether we can flatten the incoming path
   */
  for (count=0,iter = original; iter; iter=iter->next)
    {
      switch (iter->d.type)
        {
          case 'v':
          case 'o':
          case 'O':
          case '[':
          case ']':
          case 'z':
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
  for (i=0, iter = original; iter; iter=iter->next, i++)
    {
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
          case 'z':
            points[i].ty = SPIRO_END;
            break;
          case '{':
            points[i].ty = SPIRO_OPEN_CONTOUR;
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
  SpiroCPsToBezier(points,count, FALSE, (void*)&bezcontext);
  g_free (points);

  return bezcontext.path;
}

static GeglPathList *
points_to_bezier_path (gdouble  coord_x[],
                       gdouble  coord_y[],
                       gint     n_coords)
{
  GeglPathList *ret = NULL;
  gint    i;
  gdouble smooth_value;
 
  smooth_value  = 0.8;

  if (!n_coords)
    return NULL;

  ret = gegl_path_list_append (ret, 'M', coord_x[0], coord_y[0]);

  for (i=1;i<n_coords;i++)
    {
      gdouble x2 = coord_x[i];
      gdouble y2 = coord_y[i];

      gdouble x0,y0,x1,y1,x3,y3;

      if (i==1)
        {
          x0=coord_x[i-1];
          y0=coord_y[i-1];
          x1 = coord_x[i-1];
          y1 = coord_y[i-1];
        }
      else
        {
          x0=coord_x[i-2];
          y0=coord_y[i-2];
          x1 = coord_x[i-1];
          y1 = coord_y[i-1];
        }

      if (i+1 < n_coords)
        {
          x3 = coord_x[i+1];
          y3 = coord_y[i+1];
        }
      else
        {
          x3 = coord_x[i];
          y3 = coord_y[i];
        }

      {
        gdouble xc1 = (x0 + x1) / 2.0;
        gdouble yc1 = (y0 + y1) / 2.0;
        gdouble xc2 = (x1 + x2) / 2.0;
        gdouble yc2 = (y1 + y2) / 2.0;
        gdouble xc3 = (x2 + x3) / 2.0;
        gdouble yc3 = (y2 + y3) / 2.0;
        gdouble len1 = sqrt( (x1-x0) * (x1-x0) + (y1-y0) * (y1-y0) );
        gdouble len2 = sqrt( (x2-x1) * (x2-x1) + (y2-y1) * (y2-y1) );
        gdouble len3 = sqrt( (x3-x2) * (x3-x2) + (y3-y2) * (y3-y2) );
        gdouble k1 = len1 / (len1 + len2);
        gdouble k2 = len2 / (len2 + len3);
        gdouble xm1 = xc1 + (xc2 - xc1) * k1;
        gdouble ym1 = yc1 + (yc2 - yc1) * k1;
        gdouble xm2 = xc2 + (xc3 - xc2) * k2;
        gdouble ym2 = yc2 + (yc3 - yc2) * k2;
        gdouble ctrl1_x = xm1 + (xc2 - xm1) * smooth_value + x1 - xm1;
        gdouble ctrl1_y = ym1 + (yc2 - ym1) * smooth_value + y1 - ym1;
        gdouble ctrl2_x = xm2 + (xc2 - xm2) * smooth_value + x2 - xm2;
        gdouble ctrl2_y = ym2 + (yc2 - ym2) * smooth_value + y2 - ym2;

        if (i==n_coords-1)
          {
            ctrl2_x = x2;
            ctrl2_y = y2;
          }

        ret = gegl_path_list_append (ret, 'C', ctrl1_x, ctrl1_y,
                                              ctrl2_x, ctrl2_y,
                                              x2,      y2);
      }
   }
  return ret;
}


static GeglPathList *gegl_path_spiro_flatten2 (GeglPathList *original)
{
  GeglPathList *ret;
  GeglPathList *iter;
  gdouble *coordsx;
  gdouble *coordsy;
  gboolean is_spiro = TRUE;
  gint count;
  gint i;
  /* first we do a run through the path checking it's length
   * and determining whether we can flatten the incoming path
   */
  for (count=0,iter = original; iter; iter=iter->next)
    {
      switch (iter->d.type)
        {
          case '*':
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

  coordsx = g_new0 (gdouble, count);
  coordsy = g_new0 (gdouble, count);

  for (i=0, iter = original; iter; iter=iter->next, i++)
    {
      coordsx[i] = iter->d.point[0].x;
      coordsy[i] = iter->d.point[0].y;
    }
  
  ret = points_to_bezier_path (coordsx, coordsy, count);

  g_free (coordsx);
  g_free (coordsy);

  return ret;
}

void gegl_spiro_init (void)
{
  static gboolean done = FALSE;
  if (done)
    return;
  done = TRUE;
  gegl_path_add_type ('v', 1, "spiro corner");
  gegl_path_add_type ('o', 1, "spiro g4");
  gegl_path_add_type ('O', 1, "spiro g2");
  gegl_path_add_type ('[', 1, "spiro left");
  gegl_path_add_type (']', 1, "spiro right");

  gegl_path_add_type ('*', 1, "path");

  gegl_path_add_flattener (gegl_path_spiro_flatten);
  gegl_path_add_flattener (gegl_path_spiro_flatten2);
}
