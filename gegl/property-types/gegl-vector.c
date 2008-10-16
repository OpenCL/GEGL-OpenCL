/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-buffer-private.h"
#include "gegl-vector.h"
#include "gegl-color.h"
#include "gegl-utils.h"
#include <glib/gprintf.h>



/* ###################################################################### */
/* path-list code originating in horizon */

typedef struct _Path Path;
#define BEZIER_SEGMENTS 32
#define AA 3

#include <glib.h>
#include <math.h>

/* the path is a series of commands in a linked list,
 * the first node is the path state, it contains the pens last coordinates.
 *
 * The second node is always a move to, and specifies the start of the
 * first segment. Curves are automatically turned into lines upon generation.
 *
 * s: 40, 50
 * m: 100, 100
 * l: 200, 100
 * l: 200, 50
 */



struct _Path
{
  GeglVectorKnot d;
  Path          *next;
};

typedef struct KnotInfo
{
  gchar  type;
  gint   pairs;
  gchar *name;
  Path *(*flatten) (Path *head, Path *prev, Path *self);
} KnotInfo;

#if 0
/* it would be possible to compile paths like this into the library */
static GeglVectorKnot test[] = 
      {{'m', {{0.0, 0.0}}},
       {'l', {{20.0, 20.0}}},
       {'C', {{20.0, 20.0}, {40, 40}, {50, 50}}}
  };
#endif

static Path *flatten_copy      (Path *head, Path *prev, Path *self);
static Path *flatten_rel_copy  (Path *head, Path *prev, Path *self);
static Path *flatten_nop       (Path *head, Path *prev, Path *self);
static Path *flatten_curve     (Path *head, Path *prev, Path *self);

/* FIXME: handling of relative commands should be moved to the flattening stage */

/* this table should be possible to replace at runtime */

static KnotInfo knot_types[]=
{
  {'M',  1, "move to",              flatten_copy},
  {'L',  1, "line to",              flatten_copy},
  {'C',  3, "curve to",             flatten_curve},

  {'m',  1, "rel move to",          flatten_rel_copy},
  {'l',  1, "rel line to",          flatten_rel_copy},
  {'c',  3, "rel curve to",         flatten_rel_copy},

  {'s',  0, "sentinel",             flatten_nop},
  {'z',  0, "sentinel",             flatten_nop},
  {'\0', 0, "marker for end of",    flatten_copy},
};

static KnotInfo *find_knot_type(gchar type)
{
  gint i;
  for (i=0; knot_types[i].type != '\0'; i++)
    if (knot_types[i].type == type)
      return &knot_types[i];
  return NULL;
}

static Path *
path_append (Path  *head,
             Path **res);

static Path *flatten_nop (Path *head, Path *prev, Path *self)
{
  return head;
}

static Path *flatten_copy (Path *head, Path *prev, Path *self)
{
  Path *newp;
  head = path_append (head, &newp);
  newp->d = self->d;
  return head;
}

static Path *flatten_rel_copy (Path *head, Path *prev, Path *self)
{
  Path *newp;
  gint i;
  head = path_append (head, &newp);
  newp->d = self->d;
  for (i=0;i<4;i++)
    {
      newp->d.point[i].x += prev->d.point[0].x;
      newp->d.point[i].y += prev->d.point[0].y;
    }
  switch (newp->d.type)
    {
      case 'l':
        newp->d.type = 'L';
        break;
      case 'm':
        newp->d.type = 'M';
        break;
      case 'c':
        newp->d.type = 'C';
        break;
    }
  return head;
}

/* linear interpolation between two points */
static void
lerp (Point *dest,
      Point *a,
      Point *b,
      gfloat   t)
{
  dest->x = a->x + (b->x-a->x) * t;
  dest->y = a->y + (b->y-a->y) * t;
}

static gfloat
point_dist (Point *a,
            Point *b)
{
  return sqrt ((a->x-b->x)*(a->x-b->x) +
               (a->y-b->y)*(a->y-b->y));
}

static void
bezier2 (Path  *prev,/* we need the previous node as well when rendering a path */
         Path  *curve,
         Point *dest,
         gfloat t)
{
  Point ab,bc,cd,abbc,bccd;

  if (prev->d.type == 'c')
    lerp (&ab, &prev->d.point[2], &curve->d.point[0], t);
  else
    lerp (&ab, &prev->d.point[0], &curve->d.point[0], t);
  lerp (&bc, &curve->d.point[0], &curve->d.point[1], t);
  lerp (&cd, &curve->d.point[1], &curve->d.point[2], t);
  lerp (&abbc, &ab, &bc,t);
  lerp (&bccd, &bc, &cd,t);
  lerp (dest, &abbc, &bccd, t);
}

static Path *
path_add1 (Path   *head,
           gchar   type,
           gfloat  x,
           gfloat  y);

static Path *flatten_curve (Path *head, Path *prev, Path *self)
{ /* create piecevise linear approximation of bezier curve */
  gfloat f;

  for (f=0; f<1.0; f += 1.0 / BEZIER_SEGMENTS)
    {
      Point res;

      bezier2 (prev, self, &res, f);

      head = path_add1 (head, 'L', res.x, res.y);
    }
  head = path_add1 (head, 'L', self->d.point[2].x, self->d.point[2].y);
  return head;
}

static Path *path_flatten (Path *original)
{
  Path *iter;
  Path *prev = NULL;
  Path *self = NULL;

  Path *endp = NULL;
  
  for (iter=original; iter; iter=iter->next)
    {
      self = find_knot_type (iter->d.type)->flatten (self, endp, iter);
      if (!endp)
        endp = self;
      while (endp && endp->next)
        endp=endp->next;
      prev = iter;
    }
  return self;
}


static Path   *path_flatten (Path *original);
static gdouble path_get_length (Path *path);

typedef struct _Path Head;


#if 0
static gint
path_last_x (Path   *path)
{
  if (path)
    return path->d.point[0].x;
  return 0;
}

static gint
path_last_y (Path   *path)
{
  if (path)
    return path->d.point[0].y;
  return 0;
}
#endif

static Path *
path_append (Path  *head,
             Path **res)
{
  Path *iter = head;
  while (iter && iter->next)
    iter=iter->next;

  if (iter)
    {
      iter->next = g_slice_new0 (Path);
      iter = iter->next;
    }
  else /* creating new path */
    {
      head = g_slice_new0 (Head);
      iter=head;
    }
  g_assert (res);
    *res = iter;

  return head;
}

static Path *
path_add4 (Path   *head,
           gchar   type,
           gfloat  x0,
           gfloat  y0,
           gfloat  x1,
           gfloat  y1,
           gfloat  x2,
           gfloat  y2,
           gfloat  x3,
           gfloat  y3)
{
  Path *iter;

  head = path_append (head, &iter);

  iter->d.type = type;
  iter->d.point[0].x =x0;
  iter->d.point[0].y =y0;
  iter->d.point[1].x =x1;
  iter->d.point[1].y =y1;
  iter->d.point[2].x =x2;
  iter->d.point[2].y =y2;
  iter->d.point[3].x =x3;
  iter->d.point[3].y =y3;

  return head;
}


static Path *
path_add1 (Path   *head,
           gchar   type,
           gfloat  x,
           gfloat  y)
{
  return path_add4 (head, type, x, y, 0, 0, 0, 0, 0, 0);
}

static Path *
path_add2 (Path   *head,
           gchar   type,
           gfloat  x,
           gfloat  y,
           gfloat  x1,
           gfloat  y1)
{
  return path_add4 (head, type, x, y, x1, y1, 0, 0, 0, 0);
}

static Path *
path_add3 (Path   *head,
           gchar   type,
           gfloat  x,
           gfloat  y,
           gfloat  x1,
           gfloat  y1,
           gfloat  x2,
           gfloat  y2)
{
  return path_add4 (head, type, x, y, x1, y1, x2, y2, 0, 0);
}

static Path *
path_destroy (Path *path)
{
  g_slice_free_chain (Path, path, next);

  return NULL;
}

static Path *
path_move_to (Path   *path,
              gfloat  x,
              gfloat  y)
{
  return path_add1 (path, 'M', x, y);
}

static Path *
path_line_to (Path   *path,
              gfloat  x,
              gfloat  y)
{
  return path_add1 (path, 'L', x, y);
}

static Path *
path_curve_to (Path   *path,
               gfloat  x1,
               gfloat  y1,
               gfloat  x2,
               gfloat  y2,
               gfloat  x3,
               gfloat  y3)
{
  return path_add3 (path, 'C', x1, y1, x2, y2, x3, y3);
}

static Path *
path_rel_move_to (Path   *path,
                  gfloat  x,
                  gfloat  y)
{
  return path_add1 (path, 'm', x, y);
}

static Path *
path_rel_line_to (Path   *path,
                  gfloat  x,
                  gfloat  y)
{
  return path_add1 (path, 'l', x, y);
}

static Path *
path_rel_curve_to (Path   *path,
                   gfloat  x1,
                   gfloat  y1,
                   gfloat  x2,
                   gfloat  y2,
                   gfloat  x3,
                   gfloat  y3)
{
  return path_add3 (path, 'c', x1, y1, x2, y2, x3, y3);
}



static void
path_calc (Path       *path,
           gdouble     pos,
           gdouble    *xd,
           gdouble    *yd)
{
  Path *iter = path;
  gfloat traveled_length = 0;
  gfloat need_to_travel = 0;
  gfloat x = 0, y = 0;

  gboolean had_move_to = FALSE;

  while (iter)
    {
      //fprintf (stderr, "%c, %i %i\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);
      switch (iter->d.type)
        {
          case 'M':
            x = iter->d.point[0].x;
            y = iter->d.point[0].y;
            need_to_travel = 0;
            traveled_length = 0;
            had_move_to = TRUE;
            break;

          case 'L':
            {
              Point a,b;

              gfloat spacing;
              gfloat local_pos;
              gfloat distance;
              gfloat offset;
              gfloat leftover;


              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              spacing = 0.2;

              distance = point_dist (&a, &b);

              leftover = need_to_travel - traveled_length;
              offset = spacing - leftover;

              local_pos = offset;

              if (distance > 0)
                for (;
                     local_pos <= distance;
                     local_pos += spacing)
                  {
                    Point spot;
                    gfloat ratio = local_pos / distance;

                    lerp (&spot, &a, &b, ratio);

                    traveled_length += spacing;
                    if (traveled_length > pos)
                      {
                        *xd = spot.x;
                        *yd = spot.y;
                        return;
                      }
                  }

              need_to_travel += distance;

              x = b.x;
              y = b.y;
            }

            break;
          case 's':
            break;
          default:
            g_warning ("can't compute length for instruction: %c\n", iter->d.type);
            break;
        }
      iter=iter->next;
    }
}

static void
path_calc_values (Path    *path,
                  guint    num_samples,
                  gdouble *xs,
                  gdouble *ys)
{
  gdouble length = path_get_length (path);
  gint i;
  for (i=0; i<num_samples; i++)
    {
      gdouble x, y;
      path_calc (path, (i*1.0)/num_samples * length, &x, &y);

      xs[i] = x;
      ys[i] = y;
    }
}

static gdouble
path_get_length (Path *path)
{
  Path *iter = path;
  gfloat traveled_length = 0;
  gfloat x = 0, y = 0;

  while (iter)
    {
      switch (iter->d.type)
        {
          case 'M':
            x = iter->d.point[0].x;
            y = iter->d.point[0].y;
            break;
          case 'L':
            {
              Point a,b;
              gfloat distance;

              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              distance = point_dist (&a, &b);
              traveled_length += distance;

              x = b.x;
              y = b.y;
            }
            break;
          case 'u':
            break;
          case 's':
            break;
          default:
            g_warning ("can't compute length for instruction: %c\n", iter->d.type);
            return traveled_length;
            break;
        }
      iter=iter->next;
    }
  return traveled_length;
}



/******************/


typedef struct _GeglVectorPrivate GeglVectorPrivate;
typedef struct _VectorNameEntity  VectorNameEntity;

struct _GeglVectorPrivate
{
  Path     *path;
  Path     *flat_path;
  gboolean  flat_path_clean;

  Path *axis[8];
  gint  n_axes;

  GeglRectangle dirtied;
};

enum
{
  PROP_0,
};


enum
{
  GEGL_VECTOR_CHANGED,
  GEGL_VECTOR_LAST_SIGNAL
};

guint gegl_vector_signals[GEGL_VECTOR_LAST_SIGNAL] = { 0 };


static void finalize     (GObject      *self);
static void set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec);
static void get_property (GObject      *gobject,
                          guint         prop_id,
                          GValue       *value,
                          GParamSpec   *pspec);

G_DEFINE_TYPE (GeglVector, gegl_vector, G_TYPE_OBJECT);

#define GEGL_VECTOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o),\
                                   GEGL_TYPE_VECTOR, GeglVectorPrivate))



#include <gegl-buffer.h>

static gint compare_ints (gconstpointer a,
                          gconstpointer b)
{
  return GPOINTER_TO_INT (a)-GPOINTER_TO_INT (b);
}

static void gegl_buffer_accumulate (GeglBuffer    *buffer,
                                    GeglRectangle *roi,
                                    const gfloat  *col)
{
  static Babl   *format = NULL;
  static gfloat *buf = NULL;
  static gint len = 0;
  gint i;

  if (!format)
    format = babl_format ("RaGaBaA float");

  if (!buf || len < roi->width)
    {
      len = roi->width;
      if (buf)
        g_free (buf);
      buf = g_malloc (len * 4 *4);
      /* final scratch buf will be "leaked" */
    }

  gegl_buffer_get (buffer, 1.0, roi, format, buf, 0);
  for (i=0; i< roi->width; i++)
    {
      gint j;
      for (j=0; j<4; j++)
        buf[i*4 + j] += col[j];
    }
  gegl_buffer_set (buffer, roi, format, buf, 0);
}

static void ensure_flattened (GeglVector *vector)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  if (priv->flat_path_clean)
    return;
  if (priv->flat_path)
    path_destroy (priv->flat_path);
  priv->flat_path = path_flatten (priv->path);
  priv->flat_path_clean = TRUE;
}

static void
path_calc_values (Path *path,
                  guint      num_samples,
                  gdouble   *xs,
                  gdouble   *ys);


void gegl_vector_fill (GeglBuffer *buffer,
                       GeglVector *vector,
                       GeglColor  *color,
                       gboolean    winding)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  gdouble xmin, xmax, ymin, ymax;
  GeglRectangle extent;
  gfloat  horsub = AA;
  gint    versubi = horsub;
  gfloat  versub = versubi;
  gint    samples;

  if (!vector)
    return;

  ensure_flattened (vector);
  samples = path_get_length (priv->flat_path);
  gegl_vector_get_bounds (vector, &xmin, &xmax, &ymin, &ymax);

  extent.x = floor (xmin);
  extent.y = floor (ymin);
  extent.width = ceil (xmax) - extent.x;
  extent.height = ceil (ymax) - extent.y;

  {
    GSList *scanlines[extent.height * versubi];

    gdouble xs[samples];
    gdouble ys[samples];

    gint i;
    gdouble prev_x;
    gint    prev_y;
    gdouble first_x;
    gint    first_y;
    gint    lastline=-1;
    gint    lastdir=-2;

    path_calc_values (priv->flat_path, samples, xs, ys);

    /* clear scanline intersection lists */
    for (i=0; i < extent.height * versub; i++)
      scanlines[i]=NULL;

    first_x = prev_x = xs[0] * horsub;
    first_y = prev_y = ys[0] * versub;
    
    /* saturate scanline intersection list */
    for (i=1; i<samples; i++)
      {
        gint dest_x = xs[i] * horsub;
        gint dest_y = ys[i] * versub;
        gint ydir;
        gint dx;
        gint dy;
        gint y;
fill_close:  /* label used for goto to close last segment */
        dx = dest_x - prev_x;
        dy = dest_y - prev_y;

        if (dy < 0)
          ydir = -1;
        else
          ydir = 1;

        /* do linear interpolation between vertexes */
        for (y=prev_y; y!= dest_y; y += ydir)
          {
            if (y-extent.y * versub >= 0 &&
                y-extent.y * versub < extent.height * versub &&
                lastline != y)
              {
                gint x = prev_x + (dx * (y-prev_y)) / dy;
                /* XXX: * clamp the spans to the width/height of the buffer ? */

                scanlines[ y - extent.y * versubi]=
                  g_slist_insert_sorted (scanlines[ y - extent.y * versubi],
                                         GINT_TO_POINTER(x),
                                         compare_ints);
                if (ydir != lastdir &&
                    lastdir != -2)
                  scanlines[ y - extent.y * versubi]=
                    g_slist_insert_sorted (scanlines[ y - extent.y * versubi],
                                           GINT_TO_POINTER(x),
                                           compare_ints);
                lastdir = ydir;
                lastline = y;
              }
          }

        prev_x = dest_x;
        prev_y = dest_y;

        /* if we're on the last knot, fake the first vertex being a next one */
        if (i+1 == samples)
          {
            dest_x = first_x;
            dest_y = first_y;
            i++; /* to make the loop finally end */
            goto fill_close;
          }
      }

    /* Fill the spans */
{
    const gfloat *colc = gegl_color_float4 (color);
    gfloat col[4] = {colc[0],colc[1],colc[2],colc[3]};
    gfloat factor = 1.0/(horsub * versub);

    col[0] *= factor;
    col[1] *= factor;
    col[2] *= factor;
    col[3] *= factor;

    if (gegl_buffer_is_shared (buffer))
    while (!gegl_buffer_try_lock (buffer));

    for (i=0; i < extent.height * versub; i++)
      {
        GSList *iter = scanlines[i];
        while (iter)
          {
            GSList *next = iter->next;
            gint    j;
            gint startx, endx;
            if (!next)
              break;

            startx = GPOINTER_TO_INT (iter->data);
            endx   = GPOINTER_TO_INT (next->data);

            for (j=0;j<horsub;j++)
            {
              GeglRectangle roi={(startx+j)/horsub, extent.y + i/versub, (endx - startx-j) / horsub, 1};
              gegl_buffer_accumulate (buffer, &roi, col);
            }

            iter = next->next;
          }
        if (scanlines[i])
          g_slist_free (scanlines[i]);
      }

    if (gegl_buffer_is_shared (buffer))
    gegl_buffer_unlock (buffer);
  }
  }
}

typedef struct StampStatic {
  gboolean  valid;
  Babl     *format;
  gfloat   *buf;
  gdouble   radius;
}StampStatic;

void gegl_vector_stamp (GeglBuffer *buffer,
                        gdouble     x,
                        gdouble     y,
                        gdouble     radius,
                        gdouble     hardness,
                        GeglColor  *color);

void gegl_vector_stamp (GeglBuffer *buffer,
                        gdouble     x,
                        gdouble     y,
                        gdouble     radius,
                        gdouble     hardness,
                        GeglColor  *color)
{
  const gfloat *col = gegl_color_float4 (color);
  static StampStatic s = {FALSE,}; /* XXX: 
                                      we will ultimately leak the last valid
                                      cached brush. */

  GeglRectangle roi = {floor(x-radius),
                       floor(y-radius),
                       ceil (x+radius) - floor (x-radius),
                       ceil (y+radius) - floor (y-radius)};

  GeglRectangle temp;

  /* bail out if we wouldn't leave a mark on the buffer */
  if (!gegl_rectangle_intersect (&temp, &roi, gegl_buffer_get_extent (buffer)))
      return;

  if (s.format == NULL)
    s.format = babl_format ("RGBA float");

  if (s.buf == NULL ||
      s.radius != radius)
    {
      if (s.buf != NULL)
        g_free (s.buf);
      /* allocate a little bit more, just in case due to rounding errors and
       * such */
      s.buf = g_malloc (4*4* (roi.width + 2 ) * (roi.height + 2));
      s.radius = radius;
      s.valid = TRUE;  
    }
  g_assert (s.buf);

  gegl_buffer_get (buffer, 1.0, &roi, s.format, s.buf, 0);

  {
    gint u, v;
    gint i=0;

    gfloat radius_squared = radius * radius;
    gfloat inner_radius_squared = (radius * hardness)*(radius * hardness);
    gfloat soft_range = radius_squared - inner_radius_squared;

    for (v= roi.y; v < roi.y + roi.height ; v++)
    {
      gfloat vy2 = (v-y)*(v-y);
      for (u= roi.x; u < roi.x + roi.width; u++)
        {
          gfloat o = (u-x) * (u-x) + vy2;

          if (o < inner_radius_squared)
             o = col[3];
          else if (o < radius_squared)
            {
              o = (1.0 - (o-inner_radius_squared) / (soft_range)) * col[3];
            }
          else
            {
              o=0.0;
            }
         if (o!=0.0)
           {
             gint c;
             for (c=0;c<4;c++)
               s.buf[i*4+c] = (s.buf[i*4+c] * (1.0-o) + col[c] * o);
           }
         i++;
        }
    }
  }
  gegl_buffer_set (buffer, &roi, s.format, s.buf, 0);
}


void gegl_vector_stroke (GeglBuffer *buffer,
                         GeglVector *vector,
                         GeglColor  *color,
                         gdouble     linewidth,
                         gdouble     hardness)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  GeglRectangle bufext;
  gfloat traveled_length = 0;
  gfloat need_to_travel = 0;
  gfloat x = 0,y = 0;
  gboolean had_move_to = FALSE;
  Path *iter;
  gdouble       xmin, xmax, ymin, ymax;
  GeglRectangle extent;

  if (!vector)
    return;

  ensure_flattened (vector);

  iter = priv->flat_path;
  gegl_vector_get_bounds (vector, &xmin, &xmax, &ymin, &ymax);
  extent.x = floor (xmin);
  extent.y = floor (ymin);
  extent.width = ceil (xmax) - extent.x;
  extent.height = ceil (ymax) - extent.y;

  bufext = *gegl_buffer_get_extent (buffer);

  if (gegl_buffer_is_shared (buffer))
  while (!gegl_buffer_try_lock (buffer));

  if (!gegl_rectangle_intersect (&extent, &bufext, &bufext))
    return;
  gegl_buffer_clear (buffer, &extent);

  while (iter)
    {
      //fprintf (stderr, "%c, %i %i\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);
      switch (iter->d.type)
        {
          case 'M':
            x = iter->d.point[0].x;
            y = iter->d.point[0].y;
            need_to_travel = 0;
            traveled_length = 0;
            had_move_to = TRUE;
            break;
          case 'L':
            {
              Point a,b;

              gfloat spacing;
              gfloat local_pos;
              gfloat distance;
              gfloat offset;
              gfloat leftover;
              gfloat radius = linewidth / 2.0;


              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              spacing = 0.2 * radius;

              distance = point_dist (&a, &b);

              leftover = need_to_travel - traveled_length;
              offset = spacing - leftover;

              local_pos = offset;

              if (distance > 0)
                for (;
                     local_pos <= distance;
                     local_pos += spacing)
                  {
                    Point spot;
                    gfloat ratio = local_pos / distance;
                    gfloat radius = linewidth / 2;
                                 /* horizon used to refetch the radius
                                  * for each step from the tool, to be
                                  * able to have variable line width
                                  */
                    lerp (&spot, &a, &b, ratio);

                    gegl_vector_stamp (buffer,
                      spot.x, spot.y, radius, hardness, color);

                    traveled_length += spacing;
                  }

              need_to_travel += distance;

              x = b.x;
              y = b.y;
            }

            break;
          case 'u':
            g_error ("stroking uninitialized path\n");
            break;
          case 's':
            break;
          default:
            g_error ("can't stroke for instruction: %i\n", iter->d.type);
            break;
        }
      iter=iter->next;
    }

  if (gegl_buffer_is_shared (buffer))
  gegl_buffer_unlock (buffer);
}


static void
gegl_vector_init (GeglVector *self)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
}

static void
gegl_vector_class_init (GeglVectorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_type_class_add_private (klass, sizeof (GeglVectorPrivate));

  gegl_vector_signals[GEGL_VECTOR_CHANGED] =
    g_signal_new ("changed", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0 /* class offset */,
                  NULL /* accumulator */,
                  NULL /* accu_data */,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, /*return type */
                  1, G_TYPE_POINTER);
}

static void
gegl_vector_emit_changed (GeglVector          *vector,
                          const GeglRectangle *bounds)
{
  GeglRectangle rect;
  if (!bounds)
    {
       gdouble min_x;
       gdouble max_x;
       gdouble min_y;
       gdouble max_y;
       gegl_vector_get_bounds (vector, &min_x, &max_x, &min_y, &max_y);
       rect.x = min_x;
       rect.y = min_y;
       rect.width = max_x - min_x;
       rect.height = max_y - min_y;
       bounds = &rect;
    }
  g_signal_emit (vector, gegl_vector_signals[GEGL_VECTOR_CHANGED], 0,
                 bounds, NULL);
}

static void
finalize (GObject *gobject)
{
  GeglVector        *self = GEGL_VECTOR (gobject);
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);

  self = NULL;
  if (priv->path)
    path_destroy (priv->path);
  if (priv->flat_path)
    path_destroy (priv->flat_path);
  priv = NULL;

  G_OBJECT_CLASS (gegl_vector_parent_class)->finalize (gobject);
}

static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

static void
get_property (GObject    *gobject,
              guint       property_id,
              GValue     *value,
              GParamSpec *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}

GeglVector *
gegl_vector_new (void)
{
  GeglVector        *self = GEGL_VECTOR (g_object_new (GEGL_TYPE_VECTOR, NULL));
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);

  gegl_vector_init (self);
  priv->flat_path_clean = FALSE;

  return self;
}

static void gen_rect (GeglRectangle *r,
                      gdouble x1, gdouble y1, gdouble x2, gdouble y2)
{
  if (x1>x2)
    {
      gint t;
      t=x1;
      x1=x2;
      x2=x1;
    }
  if (y1>y2)
    {
      gint t;
      t=y1;
      y1=y2;
      y2=y1;
    }
  x1=floor (x1);
  y1=floor (y1);
  x2=ceil (x2);
  y2=ceil (y2);
  r->x=x1;
  r->y=y1;
  r->width=x2-x1;
  r->height=y2-y1;
}

void
gegl_vector_line_to (GeglVector *self,
                     gdouble     x,
                     gdouble     y)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);

  if (priv->path)
  gen_rect (&priv->dirtied, x, y, priv->path->d.point[0].x,
                         priv->path->d.point[0].y);
  priv->path = path_line_to (priv->path, x, y);

  if (priv->path)
    gegl_vector_emit_changed (self, &priv->dirtied);
  priv->flat_path_clean = FALSE;
}

void
gegl_vector_move_to (GeglVector *self,
                     gdouble     x,
                     gdouble     y)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_move_to (priv->path, x, y);
  /*gegl_vector_emit_changed (self);*/
  priv->flat_path_clean = FALSE;
}

void
gegl_vector_curve_to (GeglVector *self,
                      gdouble     x1,
                      gdouble     y1,
                      gdouble     x2,
                      gdouble     y2,
                      gdouble     x3,
                      gdouble     y3)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_curve_to (priv->path, x1, y1, x2, y2, x3, y3);
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (self, NULL);
}


void
gegl_vector_rel_line_to (GeglVector *self,
                         gdouble     x,
                         gdouble     y)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_rel_line_to (priv->path, x, y);
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (self, NULL);
}

void
gegl_vector_rel_move_to (GeglVector *self,
                         gdouble     x,
                         gdouble     y)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_rel_move_to (priv->path, x, y);
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (self, NULL);
}

void
gegl_vector_rel_curve_to (GeglVector *self,
                          gdouble     x1,
                          gdouble     y1,
                          gdouble     x2,
                          gdouble     y2,
                          gdouble     x3,
                          gdouble     y3)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_rel_curve_to (priv->path, x1, y1, x2, y2, x3, y3);
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (self, NULL);
}

gdouble
gegl_vector_get_length (GeglVector *self)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);
  if (!self)
    return 0.0;
  ensure_flattened (self);
  return path_get_length (priv->flat_path);
}

void         gegl_vector_get_bounds   (GeglVector   *self,
                                       gdouble      *min_x,
                                       gdouble      *max_x,
                                       gdouble      *min_y,
                                       gdouble      *max_y)
{
  GeglVectorPrivate *priv;
  Path *iter;

  *min_x = 256.0;
  *min_y = 256.0;
  *max_x = -256.0;
  *max_y = -256.0;

  if (!self)
    return;

  priv = GEGL_VECTOR_GET_PRIVATE (self);
 

  ensure_flattened (self);
  iter = priv->flat_path;

  if (*max_x < *min_x)
    *max_x = *min_x;
  if (*max_y < *min_y)
    *max_y = *min_y;

  while (iter)
    {
      gint i;
      gint max = 0;

      if (iter->d.type == 'M')
        max = 1;
      else if (iter->d.type == 'L')
        max = 1;
      else if (iter->d.type == 'C')
        max = 3;

      for (i=0;i<max;i++)
        {
          if (iter->d.point[i].x < *min_x)
            *min_x = iter->d.point[i].x;
          if (iter->d.point[i].x > *max_x)
            *max_x = iter->d.point[i].x;
          if (iter->d.point[i].y < *min_y)
            *min_y = iter->d.point[i].y;
          if (iter->d.point[i].y > *max_y)
            *max_y = iter->d.point[i].y;
        }
      iter=iter->next;
    }
}



void
gegl_vector_calc (GeglVector *self,
                  gdouble     pos,
                  gdouble    *xd,
                  gdouble    *yd)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);
  if (!self)
    return;
  ensure_flattened (self);
  return path_calc (priv->flat_path, pos, xd, yd);
}


void
gegl_vector_calc_values (GeglVector *self,
                         guint      num_samples,
                         gdouble   *xs,
                         gdouble   *ys)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);
  if (!self)
    return;
  ensure_flattened (self);
  return path_calc_values (priv->flat_path, num_samples, xs, ys);
}


/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglVector as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_VECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_VECTOR, GeglParamVector))
#define GEGL_IS_PARAM_VECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_VECTOR))

typedef struct _GeglParamVector GeglParamVector;

struct _GeglParamVector
{
  GParamSpec parent_instance;

  GeglVector *default_vector;
};

static void
gegl_param_vector_init (GParamSpec *self)
{
  GEGL_PARAM_VECTOR (self)->default_vector = NULL;
}

static void
gegl_param_vector_finalize (GParamSpec *self)
{
  GeglParamVector  *param_vector  = GEGL_PARAM_VECTOR (self);
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (
                                                       GEGL_TYPE_PARAM_VECTOR));

  if (param_vector->default_vector)
    {
      g_object_unref (param_vector->default_vector);
      param_vector->default_vector = NULL;
    }

  parent_class->finalize (self);
}

static void
value_set_default (GParamSpec *param_spec,
                   GValue     *value)
{
  GeglParamVector *gegl_vector = GEGL_PARAM_VECTOR (param_spec);

  g_value_set_object (value, gegl_vector->default_vector);
}

GType
gegl_param_vector_get_type (void)
{
  static GType param_vector_type = 0;

  if (G_UNLIKELY (param_vector_type == 0))
    {
      static GParamSpecTypeInfo param_vector_type_info = {
        sizeof (GeglParamVector),
        0,
        gegl_param_vector_init,
        0,
        gegl_param_vector_finalize,
        value_set_default,
        NULL,
        NULL
      };
      param_vector_type_info.value_type = GEGL_TYPE_VECTOR;

      param_vector_type = g_param_type_register_static ("GeglParamVector",
                                                       &param_vector_type_info);
    }

  return param_vector_type;
}

const GeglRectangle *gegl_vector_changed_rect (GeglVector *vector);

const GeglRectangle *gegl_vector_changed_rect (GeglVector *vector)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  return &priv->dirtied;
}


void
gegl_operation_invalidate (GeglOperation       *operation,
                            const GeglRectangle *roi);

void
gegl_operation_vector_prop_changed (GeglVector          *vector,
                                    const GeglRectangle *roi,
                                     GeglOperation      *operation);

void
gegl_operation_vector_prop_changed (GeglVector          *vector,
                                    const GeglRectangle *roi,
                                    GeglOperation       *operation)
{
  /* In the end forces a re-render, should be adapted to
   * allow a smaller region to be forced for re-rendering
   * when the vector is incrementally grown
   * */
  /* g_object_notify (G_OBJECT (operation), "vector"); */
  GeglRectangle rect = *roi;
  gint radius = 8;

  radius = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (operation), "vector-radius"));

  rect.width += radius * 2;
  rect.height += radius * 2;
  rect.x -= radius;
  rect.y -= radius;

    {
      gint align = 127;
      gint x= rect.x & (0xffff-align);

      rect.width +=(rect.x-x);
      x=rect.width & align;
      if (x)
        rect.width += (align-x);
    }

  gegl_operation_invalidate (operation, &rect);
}



GParamSpec *
gegl_param_spec_vector (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        GeglVector  *default_vector,
                        GParamFlags  flags)
{
  GeglParamVector *param_vector;

  param_vector = g_param_spec_internal (GEGL_TYPE_PARAM_VECTOR,
                                       name, nick, blurb, flags);

  param_vector->default_vector = default_vector;

  return G_PARAM_SPEC (param_vector);
}

static const gchar *parse_float_pair (const gchar *p,
                                      gdouble *x,
                                      gdouble *y)
{
  gchar *t = (void*) p;
  while (*t && ((*t<'0' || *t > '9') && *t!='-')) t++;
  if (!t)
    return p;
  *x = g_ascii_strtod (t, &t); 
  while (*t && ((*t<'0' || *t > '9') && *t!='-')) t++;
  if (!t)
    return p;
  *y = g_ascii_strtod (t, &t); 
  return t;
}


/* this code is generic and should also work for extensions providing 
 * new knot types to the infrastructure
 */
gchar *
gegl_vector_to_svg_path (GeglVector  *vector)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  GString *str = g_string_new ("");
  gchar *ret;
  Path *iter;
  for (iter = priv->path; iter; iter=iter->next)
    {
      gint i;
      KnotInfo *info = find_knot_type(iter->d.type);

      g_string_append_c (str, iter->d.type);
      for (i=0;i<info->pairs;i++)
        {
          gchar buf[16];
          gchar *eptr;
          g_sprintf (buf, "%f", iter->d.point[i].x);

          for (eptr = &buf[strlen(buf)-1];eptr != buf && (*eptr=='0');eptr--)
              *eptr='\0';
          if (*eptr=='.')
            *eptr='\0';
          g_string_append_printf (str, "%s,", buf);
          sprintf (buf, "%f", iter->d.point[i].y);

          for (eptr = &buf[strlen(buf)-1];eptr != buf && (*eptr=='0');eptr--)
              *eptr='\0';
          if (*eptr=='.')
            *eptr='\0';

          g_string_append_printf (str, "%s ", buf);
        }
    }
  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}

void gegl_vector_clear (GeglVector *vector)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  if (priv->path)
    path_destroy (priv->path);
  priv->path = NULL;
}

void gegl_vector_parse_svg_path (GeglVector *vector,
                                 const gchar *path)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  const gchar *p = path;
  gdouble x0, y0, x1, y1, x2, y2;

  while (*p)
    {
      gchar     type = *p;
      KnotInfo *info = find_knot_type(type);
      if (info)
        switch (info->pairs)
          {
            case 0:
              priv->path = path_add1 (priv->path, type, x0, y0);
              /* coordinates are ignored, all of these could have used add3)*/
              break;
            case 1:
              p = parse_float_pair (p, &x0, &y0);
              priv->path = path_add1 (priv->path, type, x0, y0);
              break;
            case 2:
              p = parse_float_pair (p, &x0, &y0);
              p = parse_float_pair (p, &x1, &y1);
              priv->path = path_add2 (priv->path, type, x0, y0, x1, y1);
              break;
            case 3:
              p = parse_float_pair (p, &x0, &y0);
              p = parse_float_pair (p, &x1, &y1);
              p = parse_float_pair (p, &x2, &y2);
              priv->path = path_add3 (priv->path, type, x0, y0, x1, y1, x2, y2);
              break;
          }
      p++;
    }

  priv->flat_path_clean = FALSE;
  {
   gegl_vector_emit_changed (vector, NULL);
  }
}



gint
gegl_vector_get_knot_count  (GeglVector *vector)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      count ++;
    }
  return count;

}

const GeglVectorKnot *
gegl_vector_get_knot (GeglVector *vector,
                      gint        pos)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  GeglVectorKnot *last = NULL;
  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      last=&iter->d;
      if (count == pos)
        return last;
      count ++;
    }
  if (count==-1)
    {
      return last;
    }
  return NULL;
}

/* -1 means last */

/* pos = 0, pushes the existing 0 if any to 1,
 * passing -1 means add at end
 */

void  gegl_vector_remove_knot  (GeglVector *vector,
                                gint        pos)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  Path *prev = NULL;
  Path *last = NULL;

  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          prev->next = iter->next;
          g_free (iter);
        }
      prev = iter;
      if (prev->next)
        last=iter;
      count ++;
    }
  if (count==-1)
    {
      last->next = NULL;
      g_free (prev);
    }
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (vector, NULL);
}

void  gegl_vector_add_knot     (GeglVector           *vector,
                                gint                  pos,
                                const GeglVectorKnot *knot)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  Path *prev = NULL;

  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          Path *new = g_new0(Path, 1);
          new->d = *knot;
          new->next = iter->next;
          if (prev)
            prev->next = new;
          priv->flat_path_clean = FALSE;
          gegl_vector_emit_changed (vector, NULL);
          return;
        }
      prev = iter;
      count ++;
    }
  if (count==-1)
    {
      Path *new = g_new0(Path, 1);
      new->d = *knot;
      new->next = iter->next;
      if (prev)
        prev->next = new;
      else
        priv->path = new;
    }
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (vector, NULL);
}

void  gegl_vector_replace_knot (GeglVector           *vector,
                                gint                  pos,
                                const GeglVectorKnot *knot)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  Path *prev = NULL;

  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          iter->d = *knot;
          priv->flat_path_clean = FALSE;
          gegl_vector_emit_changed (vector, NULL);
          return;
        }
      prev = iter;
      count ++;
    }
  if (count==-1)
    {
      if (prev)
        prev->d = *knot;
    }
  priv->flat_path_clean = FALSE;
  gegl_vector_emit_changed (vector, NULL);
}

void  gegl_vector_knot_foreach (GeglVector *vector,
                                void (*func) (GeglVectorKnot *knot,
                                              gpointer  data),
                                gpointer    data)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (vector);
  Path *iter;
  for (iter = priv->path; iter; iter=iter->next)
    {
      func (&iter->d, data);
    }
}

#if 0
const GeglMatrix *gegl_vector_get_matrix (GeglVector *vector)
{
}
GeglMatrix gegl_vector_set_matrix (GeglVector *vector,
                                   const GeglMatrix *matrix)
{
}
#endif
