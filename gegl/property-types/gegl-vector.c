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
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 * Spline Code Copyright 1997 David Mosberger
 */

#include <glib.h>
#include <glib/gprintf.h>
#include <glib-object.h>
#include <string.h>
#include <stdlib.h>

#include "gegl-types.h"

#include "gegl-vector.h"

/* ###################################################################### */
/* path code copied from horizon */

typedef struct _Path Path;
#define BEZIER_SEGMENTS 10

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

typedef struct Point
{
  gint x;
  gint y;
} Point;

struct _Path
{
  Point point;
  Path *next;
  char  type;
};

typedef struct _Head Head;

struct _Head
{
  Path  path;
};

/*** fixed point subdivision bezier ***/

/* linear interpolation between two point */
static void
lerp (Point *dest,
      Point *a,
      Point *b,
      gint t)
{
  dest->x = a->x + (b->x-a->x) * t / 65536;
  dest->y = a->y + (b->y-a->y) * t / 65536;
}

#define iter1(N) \
    try = root + (1 << (N)); \
    if (n >= try << (N))   \
    {   n -= try << (N);   \
        root |= 2 << (N); \
    }

static inline guint isqrt (guint n)
{
    guint root = 0, try;
    iter1 (15);    iter1 (14);    iter1 (13);    iter1 (12);
    iter1 (11);    iter1 (10);    iter1 ( 9);    iter1 ( 8);
    iter1 ( 7);    iter1 ( 6);    iter1 ( 5);    iter1 ( 4);
    iter1 ( 3);    iter1 ( 2);    iter1 ( 1);    iter1 ( 0);
    return root >> 1;
}

static gint
point_dist (Point *a,
            Point *b)
{
  return isqrt ((a->x-b->x)*(a->x-b->x) +
                (a->y-b->y)*(a->y-b->y));
}


/* evaluate a point on a bezier-curve.
 * t goes from 0 to 65536
 *
 * curve contains four control points
 */
static void
bezier (Point **curve,
        Point *dest,
        gint   t)
{
  Point ab,bc,cd,abbc,bccd;
 
  lerp (&ab, curve[0], curve[1], t);
  lerp (&bc, curve[1], curve[2], t);
  lerp (&cd, curve[2], curve[3], t);
  lerp (&abbc, &ab, &bc,t);
  lerp (&bccd, &bc, &cd,t);
  lerp (dest, &abbc, &bccd, t);
}


gint
path_last_x (Path   *path)
{
  if (path)
    return path->point.x;
  return 0;
}

gint
path_last_y (Path   *path)
{
  if (path)
    return path->point.y;
  return 0;
}

/* types:
 *   'u' : unitilitlalized state
 *   's' : initialized state (last pen coordinates)
 *   'm' : move_to
 *   'l' : line_to
 *   'c' : curve_to
 *   '.' : curve_to_
 *   'C' : curve_to_
 */

static Path *
path_add (Path *head,
          gchar type,
          gint  x,
          gint  y)
{
  Path *iter = head;
  Path *prev = NULL;

  Path *curve[4]={NULL, NULL, NULL, NULL};

  if (iter)
    {
      while (iter->next)
        {
          switch (iter->type)
            {
              case 'c':
                if (prev)
                  curve[0]=prev;
                curve[1]=iter;
                break;
              case '.':
                curve[2]=iter;
                break;
              case 'C':
                curve[3]=iter;
                break;
            }
          prev=iter;
          iter=iter->next;
        }
      /* XXX: code duplication from above*/
      switch (iter->type)
        {
          case 'c':
            if (prev)
              curve[0]=prev;
            curve[1]=iter;
            break;
          case '.':
            curve[2]=iter;
            break;
          case 'C':
            curve[3]=iter;
            break;
        }
    }
  if (iter)
    {
      iter->next = g_malloc0 (sizeof (Path));
      iter=iter->next;
    }
  else /* creating new path */
    { 
      head = g_malloc0 (sizeof (Head));
      head->type = 'u';
      if (type=='u')
        {
          iter=head;
        }
      else
        {
          head->next = g_malloc0 (sizeof (Path));
          iter=head->next;
        }
    }

  if (head->type=='u' &&
      type!='u' &&
      type!='m')
        {
          if (type=='l')
            {
              type='m'; /* turn an initial line_to into a move_to */
            }
          else
            {
              g_error ("move_to or line_to is the only legal initial element for a path");
              return NULL;
            }
        }

  iter->type=type;

  switch (type)
    {
      case 'm':
        if (head->type=='u')
          head->type='s';

        iter->point.x = x;
        iter->point.y = y;
        break;
      case 'c':
        iter->point.x = x;
        iter->point.y = y;
        break;
      case '.':
        iter->point.x = x;
        iter->point.y = y;
        break;
      case 'C':
        curve[3]=iter;
        iter->point.x = x;
        iter->point.y = y;

        /* chop off unneeded elements */
        curve[0]->next = NULL;
        
        { /* create piecevize linear approximation of bezier curve */
           gint   i;
           Point  foo[4];
           Point *pts[4];

           for (i=0;i<4;i++)
             {
               pts[i]=&foo[i];
               pts[i]->x=curve[i]->point.x;
               pts[i]->y=curve[i]->point.y;
             }

           for (i=0;i<65536;i+=65536 / BEZIER_SEGMENTS)
             {
                Point iter;

                bezier (pts, &iter, i);
                
                head = path_add (head, 'l', iter.x, iter.y);
             }
        }

        /* free amputated stubs when they are no longer useful */
        g_free (curve[1]);
        g_free (curve[2]);
        g_free (curve[3]);
        break;
      case 'l':
        iter->point.x = x;
        iter->point.y = y;
        break;
      case 'u':
        break;
      default:
        g_error ("unknown path instruction '%c'", type);
        break;
    }

  head->point.x=x;
  head->point.y=y;
  return head;
}

Path *
path_new (void)
{
  return path_add (NULL, 'u', 0, 0);
}

Path *
path_destroy      (Path *path)
{
  /*Head *head = (Head*)path;*/

  while (path)
    {
      Path *iter=path->next;
      g_free (path);
      path=iter;
    }
  return NULL;
}

Path *
path_move_to (Path *path,
              gint  x,
              gint  y)
{
  path = path_add (path, 'm', x, y);
  return path;
}

Path *
path_line_to (Path *path,
              gint  x,
              gint  y)
{
  path = path_add (path, 'l', x, y);
  return path;
}

Path *
path_curve_to (Path *path,
               gint  x1,
               gint  y1,
               gint  x2,
               gint  y2,
               gint  x3,
               gint  y3)
{
  path = path_add (path, 'c', x1, y1);
  path = path_add (path, '.', x2, y2);
  path = path_add (path, 'C', x3, y3);
  return path;
}

Path *
path_rel_move_to (Path *path,
                  gint  x,
                  gint  y)
{
  return path_move_to (path, path->point.x + x, path->point.y + y);
}

Path *
path_rel_line_to (Path *path,
                  gint  x,
                  gint  y)
{
  return path_line_to (path, path->point.x + x, path->point.y + y);
}

Path *
path_rel_curve_to (Path *path,
                   gint  x1,
                   gint  y1,
                   gint  x2,
                   gint  y2,
                   gint  x3,
                   gint  y3)
{
  return path_curve_to (path,
                        path->point.x + x1, path->point.y + y1,
                        path->point.x + x2, path->point.y + y2,
                        path->point.x + x3, path->point.y + y3);
}

#if 0

/* drawing code follows */

#include "canvas/canvas.h"
#include "brush.h"
#include "tools/tool_library.h"

/* code to stroke with a specified tool */

void
path_stroke_tool (Path   *path,
                  Canvas *canvas,
                  Tool   *tool)
{
  Path *iter = path;
  gint traveled_length = 0;
  gint need_to_travel = 0;
  gint x = 0,y = 0;

  if (!tool)
    tool = lookup_tool ("Paint");

  gboolean had_move_to = FALSE;

  while (iter)
    {
      //fprintf (stderr, "%c, %i %i\n", iter->type, iter->point.x, iter->point.y);
      switch (iter->type)
        {
          case 'm':
            x = iter->point.x;
            y = iter->point.y;
            need_to_travel = 0;
            traveled_length = 0;
            had_move_to = TRUE;
            break;
          case 'l':
            {
              Point a,b;
              ToolSetting *ts = tool_with_brush_get_toolsetting (tool);

              gint spacing;
              gint local_pos;
              gint distance;
              gint offset;
              gint leftover;
              

              a.x = x;
              a.y = y;

              b.x = iter->point.x; 
              b.y = iter->point.y;

              spacing = ts->spacing * (ts->radius * MAX_RADIUS / 65536) / 65536;

              if (spacing < MIN_SPACING * SPP / 65536)
                spacing = MIN_SPACING * SPP / 65536;

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
                    gint ratio = 65536 * local_pos / distance;
                    gint radius;

                    lerp (&spot, &a, &b, ratio);
                    
                    radius = ts->radius  * MAX_RADIUS  / 65536,

                    canvas_stamp (canvas,
                      spot.x, spot.y, radius,
                      ts->opacity * MAX_OPACITY / 65536,
                      ts->hardness,
                      ts->red, ts->green, ts->blue);

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
            g_error ("can't stroke for instruction: %i\n", iter->type);
            break;
        }
      iter=iter->next;
    }
}

extern Tool *tool_paint_new ();

static Tool *path_tool (Path *path)
{
  Head *head = (Head*)path;
  if (!head)
    return NULL;
  if (head->tool == NULL)
    head->tool = tool_paint_new ();
  return head->tool;
}

void
path_set_line_width (Path *path,
                     gint  line_width)
{
  ToolSetting *ts = tool_with_brush_get_toolsetting (path_tool (path));
  ts->radius = line_width / 2;
}

void
path_set_rgba (Path *path,
               gint  red,
               gint  green,
               gint  blue,
               gint  alpha)
{
  ToolSetting *ts = tool_with_brush_get_toolsetting (path_tool (path));
  ts->red   = red;
  ts->green = green;
  ts->blue  = blue;
  ts->opacity = alpha;
}

void
path_set_hardness (Path *path,
                   gint  hardness)
{
  ToolSetting *ts = tool_with_brush_get_toolsetting (path_tool (path));
  ts->hardness = hardness;
}

void
path_stroke (Path   *path,
             Canvas *canvas)
{
  path_stroke_tool (path, canvas, path_tool (path));
}
#endif

/* ###################################################################### */



enum
{
  PROP_0,
};

typedef struct _GeglVectorPrivate GeglVectorPrivate;
typedef struct _VectorNameEntity  VectorNameEntity;

struct _GeglVectorPrivate
{
  Path *path;
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
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE /*return type */,
                  0 /* n_params */);
}

static void
gegl_vector_emit_changed (GeglVector *vector)
{
  g_signal_emit (vector, gegl_vector_signals[GEGL_VECTOR_CHANGED], 0);
}

static void
finalize (GObject *gobject)
{
  GeglVector        *self = GEGL_VECTOR (gobject);
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);

  self = NULL;
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
  priv = NULL;

  return self;
}

void
gegl_vector_line_to (GeglVector *self,
                     gdouble     x,
                     gdouble     y)
{
  GeglVectorPrivate *priv;
  priv = GEGL_VECTOR_GET_PRIVATE (self);
  priv->path = path_line_to (priv->path, x, y);
  gegl_vector_emit_changed (self);
}

gdouble
gegl_vector_get_length (GeglVector *self)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);
  Path *iter = priv->path;
  gint traveled_length = 0;
  gint x = 0, y = 0;

  while (iter)
    {
      switch (iter->type)
        {
          case 'm':
            x = iter->point.x;
            y = iter->point.y;
            break;
          case 'l':
            {
              Point a,b;
              gint distance;

              a.x = x;
              a.y = y;

              b.x = iter->point.x; 
              b.y = iter->point.y;

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
            g_error ("can't compute length for instruction: %i\n", iter->type);
            break;
        }
      iter=iter->next;
    }
  return traveled_length;
}

void         gegl_vector_get_bounds   (GeglVector   *self,
                                       gdouble      *min_x,
                                       gdouble      *max_x,
                                       gdouble      *min_y,
                                       gdouble      *max_y)
{
  GeglVectorPrivate *priv = GEGL_VECTOR_GET_PRIVATE (self);
  Path *iter = priv->path;
  *min_x = 256.0;
  *min_y = 256.0;
  *max_x = -256.0;
  *max_y = -256.0;

  if (*max_x < *min_x)
    *max_x = *min_x;
  if (*max_y < *min_y)
    *max_y = *min_y;

  while (iter)
    {
      if (iter->type == 'l')
        {
          if (iter->point.x < *min_x)
            *min_x = iter->point.x;
          if (iter->point.x > *max_x)
            *max_x = iter->point.x;
          if (iter->point.y < *min_y)
            *min_y = iter->point.y;
          if (iter->point.y > *max_y)
            *max_y = iter->point.y;
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
  Path *iter = priv->path;
  gint traveled_length = 0;
  gint need_to_travel = 0;
  gint x = 0, y = 0;

  gboolean had_move_to = FALSE;

  while (iter)
    {
      //fprintf (stderr, "%c, %i %i\n", iter->type, iter->point.x, iter->point.y);
      switch (iter->type)
        {
          case 'm':
            x = iter->point.x;
            y = iter->point.y;
            need_to_travel = 0;
            traveled_length = 0;
            had_move_to = TRUE;
            break;
          case 'l':
            {
              Point a,b;

              gint spacing;
              gint local_pos;
              gint distance;
              gint offset;
              gint leftover;
              

              a.x = x;
              a.y = y;

              b.x = iter->point.x; 
              b.y = iter->point.y;

              spacing = 10;

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
                    gint ratio = 65536 * local_pos / distance;

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
          case 'u':
            g_error ("computing uninitialized path\n");
            break;
          case 's':
            break;
          default:
            g_error ("can't compute for instruction: %i\n", iter->type);
            break;
        }
      iter=iter->next;
    }
}


void
gegl_vector_calc_values (GeglVector *self,
                         guint      num_samples,
                         gdouble   *xs,
                         gdouble   *ys)
{
  gdouble length = gegl_vector_get_length (self);
  gint i;
  for (i=0; i<num_samples; i++)
    {
      /* FIXME: speed this up, combine with a "stroking" of the path */
      gdouble x, y;
      gegl_vector_calc (self, (i*1.0)/num_samples * length, &x, &y);
      
      xs[i] = x;
      ys[i] = y;
    }
}


/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglVector as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_TYPE_PARAM_VECTOR              \
    (gegl_param_vector_get_type ())
#define GEGL_PARAM_VECTOR(obj)              \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj),    \
    GEGL_TYPE_PARAM_VECTOR, GeglParamVector))

#define GEGL_IS_PARAM_VECTOR(obj)           \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj),    \
    GEGL_TYPE_PARAM_VECTOR))

#define GEGL_IS_PARAM_VECTOR_CLASS(klass)   \
    (G_TYPE_CHECK_CLASS_TYPE ((klass),     \
    GEGL_TYPE_PARAM_VECTOR))

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

  g_object_ref (gegl_vector->default_vector); /* XXX:
                                               not sure why this is needed,
                                               but a reference is leaked
                                               unless it his here */
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

GParamSpec *
gegl_param_spec_vector (const gchar *name,
                       const gchar *nick,
                       const gchar *blurb,
                       GeglVector   *default_vector,
                       GParamFlags  flags)
{
  GeglParamVector *param_vector;

  param_vector = g_param_spec_internal (GEGL_TYPE_PARAM_VECTOR,
                                       name, nick, blurb, flags);

  param_vector->default_vector = default_vector;

  return G_PARAM_SPEC (param_vector);
}

