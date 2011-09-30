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
 * Copyright 2011 Michael Muré <batolettre@gmail.com>
 * Copyright 2008 Øyvind Kolås <pippin@gimp.org>
 * Copyright 2006 Mark Probst <mark.probst@gmail.com>
 */

#include "config.h"

#include <string.h>
#include <math.h>

#include "gegl.h"
#include "gegl-path.h"
#include "gegl-utils.h"
#include <glib/gprintf.h>

#include <stdarg.h>

#define BEZIER_SEGMENTS 64

struct _GeglPathClass
{
  GObjectClass parent_class;
  GeglPathList *(*flattener[8]) (GeglPathList *original);
};

struct _GeglPathPrivate
{
  GeglPathList *path;
  GeglPathList *tail; /*< for fast appending */
  GeglPathList *flat_path; /*< cache of flat path */
  gboolean      flat_path_clean;

  gdouble       length;
  gboolean      length_clean;

  GeglPathList *calc_stop;
  gdouble       calc_leftover;
  gboolean      calc_clean;

  GeglRectangle dirtied;
  GeglRectangle cached_extent;
  GeglMatrix3   matrix;
  gint frozen;
};

typedef struct _GeglPathPrivate  GeglPathPrivate;

G_DEFINE_TYPE (GeglPath, gegl_path, G_TYPE_OBJECT)
#define GEGL_PATH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o),\
                                   GEGL_TYPE_PATH, GeglPathPrivate))

enum
{
  GEGL_PATH_CHANGED,
  GEGL_PATH_LAST_SIGNAL
};

guint gegl_path_signals[GEGL_PATH_LAST_SIGNAL] = { 0 };

typedef struct InstructionInfo
{
  gchar  type;
  gint   n_items;
  gchar *name;

  /**
   * flatten:
   * @matrix: a #GeglMatrix3 transformation matrix
   * @head: head of the new path
   * @prev: current tail of the new path
   * @self: the original node to convert
   *
   * a flatten function pointer is kept for all stored InstructionInfo's but are only
   * used for the internal ones
   * This function is called for each node of the path to be flattened.
   * This function should build a new flattened path on the fly.
   * The first call to this function is made with @head and @prev = NULL
   *
   * Return a pointer to the head of the flattened path.
   */
  GeglPathList *(*flatten) (GeglMatrix3   *matrix,
                            GeglPathList *head,
                            GeglPathList *prev,
                            GeglPathList *self);
} InstructionInfo;

static void finalize     (GObject      *self);
static void set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec);
static void get_property (GObject      *gobject,
                          guint         prop_id,
                          GValue       *value,
                          GParamSpec   *pspec);

static const gchar *    parse_float_pair              (const gchar      *p,
                                                       gdouble          *x,
                                                       gdouble          *y);
static void             gegl_path_emit_changed        (GeglPath         *self,
                                                       const GeglRectangle *bounds);
static void             ensure_flattened              (GeglPath         *vector);
static GeglPathList *   ensure_tail                   (GeglPathPrivate  *priv);

static GeglPathList *   flatten_copy                  (GeglMatrix3      *matrix,
                                                       GeglPathList     *head,
                                                       GeglPathList     *prev,
                                                       GeglPathList     *self);
static GeglPathList *   flatten_rel_copy              (GeglMatrix3      *matrix,
                                                       GeglPathList     *head,
                                                       GeglPathList     *prev,
                                                       GeglPathList     *self);
static GeglPathList *   flatten_nop                   (GeglMatrix3      *matrix,
                                                       GeglPathList     *head,
                                                       GeglPathList     *prev,
                                                       GeglPathList     *self);
static GeglPathList *   flatten_curve                 (GeglMatrix3      *matrix,
                                                       GeglPathList     *head,
                                                       GeglPathList     *prev,
                                                       GeglPathList     *self);

static InstructionInfo *lookup_instruction_info       (gchar             type);
static void             transform_data                (GeglMatrix3      *matrix,
                                                       GeglPathItem     *dst);
static void             copy_data                     (const GeglPathItem *src,
                                                       GeglPathItem     *dst);
static void             bezier2                       (GeglPathItem     *prev,
                                                       GeglPathItem     *curve,
                                                       GeglPathPoint    *dest,
                                                       gfloat            t);
static void             gegl_path_item_free           (GeglPathList     *p);
static GeglPathList *   gegl_path_list_append_item    (GeglPathList     *head,
                                                       gchar             type,
                                                       GeglPathList    **res,
                                                       GeglPathList     *tail);
static GeglPathList *   gegl_path_list_flatten        (GeglMatrix3      *matrix,
                                                       GeglPathList     *original);
static gboolean         gegl_path_list_calc           (GeglPathList     *path,
                                                       gdouble           pos,
                                                       gdouble          *xd,
                                                       gdouble          *yd,
                                                       GeglPathList    **stop,
                                                       gdouble          *leftover);
static void             gegl_path_list_calc_values    (GeglPathList     *path,
                                                       guint             num_samples,
                                                       gdouble          *xs,
                                                       gdouble          *ys);
static gdouble          gegl_path_list_get_length     (GeglPathList     *path);


/* FIXME: handling of relative commands should be moved to the flattening stage */

/* This table can be extended at runtime and extends the type of knots understood by the
 * "SVG path" parser/serializer.
 */
static InstructionInfo knot_types[64]= /* reserve space for a total of up to 64 types of instructions. */
{
  {'M',  2, "move to",              flatten_copy},
  {'L',  2, "line to",              flatten_copy},
  {'C',  6, "curve to",             flatten_curve},

  /* each point is relative to the previous */
  {'m',  2, "rel move to",          flatten_rel_copy},
  {'l',  2, "rel line to",          flatten_rel_copy},
  {'c',  6, "rel curve to",         flatten_rel_copy},

  {'s',  0, "sentinel",             flatten_nop},
  {'z',  0, "sentinel",             flatten_nop},
  {'\0', 0, "end of instructions",  flatten_copy},
};

static void
gegl_path_init (GeglPath *self)
{
  GeglPathPrivate *priv;
  priv = GEGL_PATH_GET_PRIVATE (self);
  gegl_matrix3_identity (&priv->matrix);
}

static void
gegl_path_class_init (GeglPathClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize     = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_type_class_add_private (klass, sizeof (GeglPathPrivate));

  gegl_path_signals[GEGL_PATH_CHANGED] =
    g_signal_new ("changed", G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE | G_SIGNAL_NO_HOOKS,
                  0    /* class offset */,
                  NULL /* accumulator */,
                  NULL /* accu_data */,
                  g_cclosure_marshal_VOID__POINTER,
                  G_TYPE_NONE, /*return type */
                  1, G_TYPE_POINTER);
}

static void
finalize (GObject *gobject)
{
  GeglPath        *self = GEGL_PATH (gobject);
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);

  self = NULL;
  if (priv->path)
    gegl_path_list_destroy (priv->path);
  if (priv->flat_path)
    gegl_path_list_destroy (priv->flat_path);
  priv = NULL;

  G_OBJECT_CLASS (gegl_path_parent_class)->finalize (gobject);
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
/***** GeglPath *****/
GeglPath *
gegl_path_new (void)
{
  GeglPath        *self = GEGL_PATH (g_object_new (GEGL_TYPE_PATH, NULL));
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);

  gegl_path_init (self);
  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
  priv->calc_clean = FALSE;

  return self;
}

GeglPath *
gegl_path_new_from_string (const gchar *path_string)
{
  GeglPath *self = gegl_path_new ();
  gegl_path_parse_string (self, path_string);
  return self;
}

gboolean
gegl_path_is_empty (GeglPath *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);
  return priv->path != NULL;
}

gint
gegl_path_get_n_nodes  (GeglPath *vector)
{
  GeglPathPrivate *priv;
  GeglPathList *iter;
  gint count=0;
  if (!vector)
    return 0;
  priv = GEGL_PATH_GET_PRIVATE (vector);

  for (iter = priv->path; iter; iter=iter->next)
    {
      count ++;
    }
  return count;
}

gdouble
gegl_path_get_length (GeglPath *self)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);
  if (!self)
    return 0.0;

  if (!priv->length_clean)
    {
      ensure_flattened (self);
      priv->length = gegl_path_list_get_length (priv->flat_path);
      priv->length_clean = TRUE;
    }
  return priv->length;
}

gboolean
gegl_path_get_node (GeglPath     *vector,
                    gint          index,
                    GeglPathItem *node)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  GeglPathList *iter;
  GeglPathItem *last = NULL;
  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      last=&iter->d;
      if (count == index)
        {
          copy_data (last, node);
          return TRUE;
        }
      count ++;
    }
  if (index==-1)
    {
      copy_data (last, node);
      return TRUE;
    }
  return FALSE;
}

/* this code is generic and should also work for extensions providing
 * new knot types to the infrastructure
 */
gchar *
gegl_path_to_string (GeglPath  *vector)
{
  GeglPathPrivate *priv;
  GString *str;
  gchar *ret;
  GeglPathList *iter;
  if (!vector)
    return g_strdup ("");
  str = g_string_new ("");
  priv = GEGL_PATH_GET_PRIVATE (vector);
  for (iter = priv->path; iter; iter=iter->next)
    {
      gint i;
      InstructionInfo *info = lookup_instruction_info(iter->d.type);

      g_string_append_c (str, iter->d.type);
      for (i=0;i<(info->n_items+1)/2;i++)
        {
          gchar buf[16];
          gchar *eptr;
          g_sprintf (buf, "%f", iter->d.point[i].x);

          for (eptr = &buf[strlen(buf)-1];eptr != buf && (*eptr=='0');eptr--)
              *eptr='\0';
          if (*eptr=='.')
            *eptr='\0';

          /* FIXME: make this work better also for other odd count
           * of n_items
           */
          if (info->n_items>1)
            {
              g_string_append_printf (str, "%s,", buf);
              sprintf (buf, "%f", iter->d.point[i].y);

              for (eptr = &buf[strlen(buf)-1];eptr != buf && (*eptr=='0');eptr--)
                  *eptr='\0';
              if (*eptr=='.')
                *eptr='\0';
            }

          g_string_append_printf (str, "%s ", buf);
        }
    }

  ret = str->str;
  g_string_free (str, FALSE);
  return ret;
}

void
gegl_path_set_matrix (GeglPath    *path,
                      GeglMatrix3 *matrix)
{
  GeglPathPrivate *priv;
  if (!path)
    {
      g_warning ("EEek! no path\n");
      return;
    }
  priv = GEGL_PATH_GET_PRIVATE (path);
  gegl_matrix3_copy_into (&priv->matrix, matrix);
  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
}

void gegl_path_get_matrix (GeglPath    *path,
                           GeglMatrix3 *matrix)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);
  gegl_matrix3_copy_into (matrix, &priv->matrix);
}

gdouble
gegl_path_closest_point (GeglPath *path,
                         gdouble   x,
                         gdouble   y,
                         gdouble  *dx,
                         gdouble  *dy,
                         gint     *node_pos_before)
{
  gdouble length = gegl_path_get_length (path);
  gint     i, n;
  gdouble closest_dist = 100000;
  gint   closest_val = 0;
  gdouble  *samples_x;
  gdouble  *samples_y;

  if (length == 0)
    {
      if (node_pos_before)
        {
          *node_pos_before = 0;
        }
      return 0.0;
    }

  n = ceil(length);
  samples_x = g_malloc (sizeof (gdouble)* n);
  samples_y = g_malloc (sizeof (gdouble)* n);

  gegl_path_calc_values (path, n, samples_x, samples_y);

  for (i=0;i<n;i++)
    {
      gdouble dist = (samples_x[i]-x) * (samples_x[i]-x)  +
                     (samples_y[i]-y) * (samples_y[i]-y);
      if (dist < closest_dist)
        {
          closest_dist = dist;
          closest_val = i;
        }
    }

  if (fabs (samples_x[n-1] - samples_x[0]) < 2.1)
    {
      if (closest_val == n-1)
        {
          closest_val = 0;
        }
    }

  if (dx)
    {
      *dx = samples_x[closest_val];
    }
  if (dy)
    {
      *dy = samples_y[closest_val];
    }

  if (node_pos_before)
    {
      GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);
      GeglPathList *iter;
      /* what node was the one before us ? */

      for (iter=priv->path,i=0; iter;i++,iter=iter->next)
        {
          gdouble dist;
          if (iter->d.type == 'z')
            continue;
          dist = gegl_path_closest_point (path,
                                   iter->d.point[0].x,
                                   iter->d.point[0].y,
                                   NULL, NULL, NULL);
          *node_pos_before = i;
          if(dist >= closest_val - 2)
            {
              *node_pos_before = i-1;
              break;
            }
          /*if(dist > closest_val)
            {
              break;
            }*/
        }
    }

  g_free (samples_x);
  g_free (samples_y);

  return closest_val * 1.0;
}

gboolean
gegl_path_calc (GeglPath   *self,
                gdouble     pos,
                gdouble    *xd,
                gdouble    *yd)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);
  GeglPathList *entry = priv->flat_path;
  GeglPathList *stop  = NULL;
  gdouble rel_pos  = 0.0;
  gdouble leftover = 0.0;
  gboolean result = FALSE;

  if (!self)
    return FALSE;
  ensure_flattened (self);

  if (priv->calc_clean && (pos > priv->calc_leftover))
    {
      entry    = priv->calc_stop;
      leftover = priv->calc_leftover;
      rel_pos  = pos - leftover;
    }
  else
    {
      rel_pos = pos;
    }

  if (gegl_path_list_calc (entry,rel_pos,xd,yd,&stop,&leftover))
    {
      priv->calc_stop = stop;
      priv->calc_leftover = leftover;
      priv->calc_clean = TRUE;

      result = TRUE;
    }
  else
    {
      priv->calc_clean = FALSE;
      result = FALSE;
    }
  return result;
}

void
gegl_path_calc_values (GeglPath *self,
                       guint      num_samples,
                       gdouble   *xs,
                       gdouble   *ys)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);
  if (!self)
    return;
  ensure_flattened (self);
  gegl_path_list_calc_values (priv->flat_path, num_samples, xs, ys);
}

void
gegl_path_get_bounds (GeglPath *self,
                      gdouble  *min_x,
                      gdouble  *max_x,
                      gdouble  *min_y,
                      gdouble  *max_y)
{
  GeglPathPrivate *priv;
  GeglPathList *iter;

  *min_x = 256.0;
  *min_y = 256.0;
  *max_x = -256.0;
  *max_y = -256.0;

  if (!self)
    return;

  priv = GEGL_PATH_GET_PRIVATE (self);

  ensure_flattened (self);
  iter = priv->flat_path;

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
      else if (iter->d.type == 'z')
        max = 0;

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
gegl_path_foreach (GeglPath     *vector,
                   void (*func) (const GeglPathItem *knot,
                                 gpointer            data),
                   gpointer      data)
{
  GeglPathPrivate *priv;
  GeglPathList *iter;
  if (!vector)
    return;
  priv = GEGL_PATH_GET_PRIVATE (vector);
  for (iter = priv->path; iter; iter=iter->next)
    {
      func (&(iter->d), data);
    }
}

void
gegl_path_foreach_flat (GeglPath     *vector,
                        void (*func) (const GeglPathItem *knot,
                                      gpointer            data),
                        gpointer      data)
{
  GeglPathPrivate *priv;
  GeglPathList *iter;
  if (!vector)
    return;
  priv = GEGL_PATH_GET_PRIVATE (vector);
  ensure_flattened (vector);
  for (iter = priv->flat_path; iter; iter=iter->next)
    {
      func (&(iter->d), data);
    }
}

void
gegl_path_clear (GeglPath *vector)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  if (priv->path)
    gegl_path_list_destroy (priv->path);
  priv->path = NULL;
  priv->tail = NULL;
}

void
gegl_path_insert_node (GeglPath           *vector,
                       gint                pos,
                       const GeglPathItem *knot)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  GeglPathList *iter;
  GeglPathList *prev = NULL;
  InstructionInfo *info = lookup_instruction_info (knot->type);

  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          GeglPathList *new = g_slice_alloc0 (sizeof (gpointer) + sizeof (gchar) + sizeof (gfloat)*2 *(info->n_items+3)/2);
          new->d.type=knot->type;
          copy_data (knot, &new->d);
          new->next = iter->next;
          /*if (prev)
            prev->next = new;*/
          iter->next = new;
          priv->flat_path_clean = FALSE;
          priv->length_clean = FALSE;
          gegl_path_emit_changed (vector, NULL);
          return;
        }
      prev = iter;
      count ++;
    }
  if (pos==-1)
    {
      GeglPathList *new = g_slice_alloc0 (sizeof (gpointer) + sizeof (gchar) + sizeof (gfloat)*2 *(info->n_items+3)/2);
      new->d.type = knot->type;
      copy_data (knot, &new->d);
      new->next = NULL;
      if (prev)
        prev->next = new;
      else
        priv->path = new;
    }
  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
  gegl_path_emit_changed (vector, NULL);
}

void
gegl_path_replace_node (GeglPath           *vector,
                        gint                pos,
                        const GeglPathItem *knot)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  GeglPathList *iter;
  GeglPathList *prev = NULL;

  gint count=0;
  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          /* check that it is large enough to contain us */
          copy_data (knot, &iter->d);
          priv->flat_path_clean = FALSE;
          priv->length_clean = FALSE;
          priv->tail = NULL;
          gegl_path_emit_changed (vector, NULL);
          return;
        }
      prev = iter;
      count ++;
    }
  if (pos==-1)
    {
      if (prev)
        copy_data (knot, &prev->d);
    }
  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
  gegl_path_emit_changed (vector, NULL);
}

void
gegl_path_remove_node (GeglPath *vector,
                       gint      pos)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  GeglPathList *iter;
  GeglPathList *prev = NULL;

  gint count=0;

  if (pos == -1)
    pos = gegl_path_get_n_nodes (vector)-1;

  for (iter = priv->path; iter; iter=iter->next)
    {
      if (count == pos)
        {
          if (prev)
            prev->next = iter->next;
          else
            priv->path = iter->next;
          gegl_path_item_free (iter);
          break;
        }
      prev = iter;
      count ++;
    }

  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
  priv->tail = NULL;
  gegl_path_emit_changed (vector, NULL);
}

void
gegl_path_parse_string (GeglPath    *vector,
                        const gchar *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  const gchar *p = path;
  InstructionInfo *previnfo = NULL;
  gdouble x0, y0, x1, y1, x2, y2;

  while (*p)
    {
      gchar            type = *p;
      InstructionInfo *info = lookup_instruction_info(type);

      if (!info && ((type>= '0' && type <= '9') || type == '-'))
        {
          if (previnfo->type == 'M')
            {
              info = lookup_instruction_info(type = 'L');
            }
          else if (previnfo->type == 'm')
            {
              info = lookup_instruction_info(type = 'l');
            }
          else if (previnfo->type == ' ')
            g_warning ("EEEK");
        }

      if (info)
        {
          switch (info->n_items)
            {
              case 0:
                priv->path = gegl_path_list_append (priv->path, type, 0., 0.);
                /* coordinates are ignored, all of these could have used add3)*/
                break;
              case 2:
                p = parse_float_pair (p, &x0, &y0);
                priv->path = gegl_path_list_append (priv->path, type, x0, y0);
                continue;
              case 4:
                p = parse_float_pair (p, &x0, &y0);
                p = parse_float_pair (p, &x1, &y1);
                priv->path = gegl_path_list_append (priv->path, type, x0, y0, x1, y1);
                continue;
              case 6:
                p = parse_float_pair (p, &x0, &y0);
                p = parse_float_pair (p, &x1, &y1);
                p = parse_float_pair (p, &x2, &y2);
                priv->path = gegl_path_list_append (priv->path, type, x0, y0, x1, y1, x2, y2);
                continue;
              default:
                g_warning ("parsing of data %i items not implemented\n", info->n_items);
                continue;
            }
          previnfo = info;
        }
      if (*p)
        p++;
    }

  priv->flat_path_clean = FALSE;
  priv->length_clean = FALSE;
  {
   gegl_path_emit_changed (vector, NULL);
  }
}

void
gegl_path_append (GeglPath *self,
                 ...)
{
  GeglPathPrivate *priv;
  InstructionInfo *info;
  GeglPathList *iter;
  gchar type;
  gint pair_no;
  va_list var_args;

  priv = GEGL_PATH_GET_PRIVATE (self);
  va_start (var_args, self);
  type = va_arg (var_args, int); /* we pass in a char, but it is promoted to int by varargs*/

  info = lookup_instruction_info(type);
  if (!info)
    g_error ("didn't find [%c]", type);

  priv->path = gegl_path_list_append_item (priv->path, type, &iter, ensure_tail(priv));

  iter->d.type       = type;
  for (pair_no=0;pair_no < (info->n_items+1)/2;pair_no++)
    {
      iter->d.point[pair_no].x = va_arg (var_args, gdouble);
      iter->d.point[pair_no].y = va_arg (var_args, gdouble);
    }

  va_end (var_args);
  priv->flat_path_clean = FALSE;

  if (type == 'L')
    {
      /* special case lineto so that the full path doesn't need
         to be re-rendered */

      GeglPathList *iter2;
      GeglRectangle rect;
      gdouble x0, y0, x1, y1;
      gdouble len;

      x0 = iter->d.point[0].x;
      y0 = iter->d.point[0].y;
      for (iter2=priv->path;iter2 && iter2->next != iter;iter2=iter2->next);
      x1 = iter2->d.point[0].x;
      y1 = iter2->d.point[0].y;

      len = sqrt ((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));

      if (x0<x1)
        {
          rect.x=x0;
          rect.width = x1-x0;
        }
      else
        {
          rect.x=x1;
          rect.width = x0-x1;
        }
      if (y0<y1)
        {
          rect.y=y0;
          rect.height = y1-y0;
        }
      else
        {
          rect.y=y1;
          rect.height = y0-y1;
        }

      if (priv->length_clean)
        priv->length += len;
      gegl_path_emit_changed (self, &rect);
    }
  else
    {
      gegl_path_emit_changed (self, NULL);
      priv->length_clean = FALSE;
    }
}

void
gegl_path_freeze (GeglPath *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);
  priv->frozen ++;
}

void
gegl_path_thaw (GeglPath *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);
  priv->frozen --;
  gegl_path_emit_changed (path, NULL); /* expose a full changed */
}

void
gegl_path_add_type (gchar        type,
                    gint         n_items,
                    const gchar *name)
{
  gint i;
  for (i=0; knot_types[i].type != '\0'; i++)
    if (knot_types[i].type == type)
      {
        g_warning ("control point type %c already exists\n", type);
        return;
      }
  knot_types[i].type = type;
  knot_types[i].n_items = n_items;
  knot_types[i].name = g_strdup (name);
  knot_types[i].flatten = flatten_copy;
  knot_types[i+1].type = '\0';
  return;
}

void
gegl_path_add_flattener (GeglPathList *(*flattener) (GeglPathList *original))
{
  GeglPath *vector = g_object_new (GEGL_TYPE_PATH, NULL);
  GeglPathClass *klass= GEGL_PATH_GET_CLASS (vector);
  gint i;
  g_object_unref (vector);
  /* currently only one additional flattener is supported, this should be fixed,
   * and flatteners should be able to return the original pointer to indicate
   * that no op was done, making memory handling more efficient
   */
  for (i=0;i<8;i++)
    {
      if (klass->flattener[i]==NULL)
        {
          klass->flattener[i] = flattener;
          klass->flattener[i+1] = NULL;
          return;
        }
    }
}

GeglPathList *
gegl_path_get_path (GeglPath *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);

  return priv->path;
}

GeglPathList *
gegl_path_get_flat_path (GeglPath *path)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (path);

  ensure_flattened (path);
  return priv->flat_path;
}

static const gchar *
parse_float_pair (const gchar *p,
                  gdouble     *x,
                  gdouble     *y)
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

static void
gegl_path_emit_changed (GeglPath            *self,
                        const GeglRectangle *bounds)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (self);
  GeglRectangle rect;
  GeglRectangle temp;
  gdouble min_x;
  gdouble max_x;
  gdouble min_y;
  gdouble max_y;

  if (priv->frozen)
    return;

  gegl_path_get_bounds (self, &min_x, &max_x, &min_y, &max_y);

  rect.x = floor (min_x);
  rect.y = floor (min_y);
  rect.width = ceil (max_x) - floor (min_x);
  rect.height = ceil (max_y) - floor (min_y);

  temp = priv->cached_extent;
  priv->cached_extent = rect;

  if (!bounds)
    {
       gegl_rectangle_bounding_box (&temp, &temp, &rect);
       bounds = &temp;
    }
  g_signal_emit (self, gegl_path_signals[GEGL_PATH_CHANGED], 0,
                 bounds, NULL);
}


/**
 * ensure_flattened:
 * @vector: a #GeglPath
 *
 * Check if the polyline version of the curse is computed and up to date,
 * and compute it if needed.
 */
static void
ensure_flattened (GeglPath *vector)
{
  GeglPathPrivate *priv = GEGL_PATH_GET_PRIVATE (vector);
  gint i;
  GeglPathList *path = priv->path;
  GeglPathList *new_path;
  GeglPathClass *klass= GEGL_PATH_GET_CLASS (vector);

  if (priv->flat_path_clean)
    return;
  if (priv->flat_path)
    gegl_path_list_destroy (priv->flat_path);

  for (i=0;klass->flattener[i];i++)
    {
      new_path = klass->flattener[i] (path);
      if (new_path != path)
        {
          if (path != priv->path)
            gegl_path_list_destroy (path);
          path = new_path;
        }
    }

  priv->flat_path = gegl_path_list_flatten (&priv->matrix, path);
  if (path != priv->path)
    gegl_path_list_destroy (path);
  priv->flat_path_clean = TRUE;
  priv->length_clean = FALSE;
  priv->calc_clean = FALSE;
}

/**
 * ensure_tail:
 * @priv: the private struct of a GeglPath
 *
 * Ensure that priv->tail point to the last element of the path list
 */
static GeglPathList *
ensure_tail (GeglPathPrivate *priv)
{
  GeglPathList *tail;

  tail = priv->tail ? priv->tail : priv->path;

  while (tail && tail->next)
    tail=tail->next;

  priv->tail = tail;
  return tail;
}

/***** Flattener *****/
static GeglPathList *
flatten_nop (GeglMatrix3  *matrix,
             GeglPathList *head,
             GeglPathList *prev,
             GeglPathList *self)
{
  return head;
}

static GeglPathList *
flatten_copy (GeglMatrix3  *matrix,
              GeglPathList *head,
              GeglPathList *prev,
              GeglPathList *self)
{
  GeglPathList *newp;
  head = gegl_path_list_append_item (head, self->d.type, &newp, NULL);
  copy_data (&self->d, &newp->d);
  transform_data (matrix, &newp->d);
  return head;
}

static GeglPathList *
flatten_rel_copy (GeglMatrix3  *matrix,
                  GeglPathList *head,
                  GeglPathList *prev,
                  GeglPathList *self)
{
  GeglPathList *newp;
  InstructionInfo *info;
  gint i;

  head = gegl_path_list_append_item (head, self->d.type, &newp, NULL);
  copy_data (&self->d, &newp->d);
  info = lookup_instruction_info (self->d.type);
  for (i=0;i<(info->n_items+1)/2;i++)
    {
      newp->d.point[i].x += prev->d.point[0].x;
      newp->d.point[i].y += prev->d.point[0].y;
    }
  switch (newp->d.type)
    {
      case 'l': newp->d.type = 'L'; break;
      case 'm': newp->d.type = 'M'; break;
      case 'c': newp->d.type = 'C'; break;
    }
  transform_data (matrix, &newp->d);
  return head;
}

/* create piecevise linear approximation of bezier curve */
static GeglPathList *
flatten_curve (GeglMatrix3  *matrix,
               GeglPathList *head,
               GeglPathList *prev,
               GeglPathList *self)
{
  gfloat f;
  GeglPathPoint res;
  gchar buf[64]="C";
  GeglPathItem *item=(void*)buf;

  copy_data (&self->d, (void*)buf);
  transform_data (matrix, (void*)buf);

  for (f=0; f<1.0; f += 1.0 / BEZIER_SEGMENTS)
    {
      bezier2 (&prev->d, (void*)buf, &res, f);
      head = gegl_path_list_append (head, 'L', res.x, res.y);
    }

  res = item->point[2];
  head = gegl_path_list_append (head, 'L', res.x, res.y);

  return head;
}

/***** InstructInfo *****/
static InstructionInfo *
lookup_instruction_info (gchar type)
{
  gint i;
  for (i=0; knot_types[i].type != '\0'; i++)
    if (knot_types[i].type == type)
      return &knot_types[i];
  return NULL;
}

/***** GeglPathItem *****/
static void
transform_data (GeglMatrix3  *matrix,
                GeglPathItem *dst)
{
  InstructionInfo *dst_info = lookup_instruction_info(dst->type);
  gint i;

  for (i=0;i<(dst_info->n_items+1)/2;i++)
    {
      gdouble x = dst->point[i].x;
      gdouble y = dst->point[i].y;
      gegl_matrix3_transform_point (matrix, &x, &y);
      dst->point[i].x=x;
      dst->point[i].y=y;
    }
}

/* XXX: copy_data should exit for internal to external conversions */
static void
copy_data (const GeglPathItem *src,
           GeglPathItem       *dst)
{
  InstructionInfo *src_info;
  gint i;

  if (!src)
    return;

  src_info = lookup_instruction_info(src->type);

  dst->type = src->type;
  for (i=0;i<(src_info->n_items+1)/2;i++)
    {
      dst->point[i].x = src->point[i].x;
      dst->point[i].y = src->point[i].y;
    }
}

static void
bezier2 (GeglPathItem  *prev,
         GeglPathItem  *curve,
         GeglPathPoint *dest,
         gfloat t)
{
  GeglPathPoint ab,bc,cd,abbc,bccd;

  if (prev->type == 'c')
    gegl_path_point_lerp (&ab, &prev->point[2], &curve->point[0], t);
  else
    gegl_path_point_lerp (&ab, &prev->point[0], &curve->point[0], t);
  gegl_path_point_lerp (&bc, &curve->point[0], &curve->point[1], t);
  gegl_path_point_lerp (&cd, &curve->point[1], &curve->point[2], t);
  gegl_path_point_lerp (&abbc, &ab, &bc,t);
  gegl_path_point_lerp (&bccd, &bc, &cd,t);
  gegl_path_point_lerp (dest, &abbc, &bccd, t);
}

static void
gegl_path_item_free (GeglPathList *p)
{
  InstructionInfo *info = lookup_instruction_info(p->d.type);
  g_slice_free1 (sizeof (gpointer) + sizeof (gchar) + sizeof (gfloat)*2 * (info->n_items+3)/2, p);
}

/***** GeglPathList *****/
GeglPathList *
gegl_path_list_destroy (GeglPathList *path)
{
  GeglPathList *iter = path;
  while (iter)
    {
      GeglPathList *next = iter->next;
      gegl_path_item_free (iter);
      iter = next;
    }
  return NULL;
}

GeglPathList *
gegl_path_list_append (GeglPathList *head,
                       ...)
{
  InstructionInfo *info;
  GeglPathList *iter;
  gchar type;
  gint pair_no;

  va_list var_args;
  va_start (var_args, head);
  type = va_arg (var_args, int); /* we pass in a char, but it is promoted to int by varargs*/

  info = lookup_instruction_info(type);
  if (!info)
    g_error ("didn't find [%c]", type);

  head = gegl_path_list_append_item (head, type, &iter, NULL);

  iter->d.type       = type;
  for (pair_no=0;pair_no<(info->n_items+2)/2;pair_no++)
    {
      iter->d.point[pair_no].x = va_arg (var_args, gdouble);
      iter->d.point[pair_no].y = va_arg (var_args, gdouble);
    }
  va_end (var_args);
  return head;
}

/**
 * gegl_path_list_append_item:
 * @head: the list to append item on
 * @type: the type of the new item
 * @res:  return a pointer to the new item
 * @tail: the tail of the list
 *
 * Append a new item in the list.
 * @tail can be NULL. Providing it will accelerate the operation.
 */
static GeglPathList *
gegl_path_list_append_item  (GeglPathList  *head,
                             gchar          type,
                             GeglPathList **res,
                             GeglPathList  *tail)
{
  GeglPathList *iter = tail?tail:head;
  InstructionInfo *info = lookup_instruction_info (type);
  g_assert (info);

  while (iter && iter->next)
    iter=iter->next;

  if (iter)
    {
      /* the +3 is padding, +1 was expected to be sufficient */
      iter->next =
        g_slice_alloc0 (sizeof (gpointer) + sizeof (gchar) + sizeof (gfloat)*2 *(info->n_items+3)/2);
      iter->next->d.type = type;
      iter = iter->next;
    }
  else /* creating new path */
    {
      /* the +3 is padding, +1 was expected to be sufficient */
      head =
        g_slice_alloc0 (sizeof (gpointer) + sizeof (gchar) + sizeof (gfloat)*2 *(info->n_items+3)/2);
      head->d.type = type;
      iter=head;
    }
  g_assert (res);
  *res = iter;

  return head;
}

/**
 * gegl_path_list_flatten:
 * @matrix: a #GeglMatrix3 transformation matrix
 * @original: the #GeglPathList to flatten
 *
 * Flatten the provided GeglPathList
 */
static GeglPathList *
gegl_path_list_flatten (GeglMatrix3  *matrix,
                        GeglPathList *original)
{
  GeglPathList *iter;
  GeglPathList *self = NULL;

  GeglPathList *endp = NULL;

  if (!original)
    return NULL;

  for (iter=original; iter; iter=iter->next)
    {
      InstructionInfo *info = lookup_instruction_info (iter->d.type);
      if(info)
        self = info->flatten (matrix, self, endp, iter);
      if (!endp)
        endp = self;
      while (endp && endp->next)
        endp=endp->next;
    }
  return self;
}

static gboolean
gegl_path_list_calc (GeglPathList *path,
                     gdouble       pos,
                     gdouble      *xd,
                     gdouble      *yd,
                     GeglPathList **stop,
                     gdouble      *leftover)
{
  GeglPathList *iter = path, *prev = NULL;
  gfloat traveled = 0.0, next_pos = 0.0;

  while (iter && !prev)
  /* fetch the start point of the path */
    {
      /* fprintf (stderr, "%c, %f %f\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);*/
      switch (iter->d.type)
        {
          case 'M':
          case 'L':
            prev = iter;
            break;
          default :
            break;
        }
      iter = iter->next;
    }

  while (iter)
  /* travel along the path */
    {
      /* fprintf (stderr, "%c, %f %f\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);*/
      switch (iter->d.type)
        {
          case 'M':
            prev = iter;
            break;

          case 'L':
            {
              GeglPathPoint a,b;
              gfloat distance;

              a.x = prev->d.point[0].x;
              a.y = prev->d.point[0].y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              distance = gegl_path_point_dist (&a, &b);
              next_pos += distance;

              if (pos <= next_pos)
                {
                  GeglPathPoint spot;
                  gfloat ratio = (pos - traveled) / (next_pos - traveled);

                  gegl_path_point_lerp (&spot, &a, &b, ratio);

                  *xd = spot.x;
                  *yd = spot.y;

                  *stop = prev;
                  *leftover += traveled;
                  return TRUE;
                }

              traveled = next_pos;

              prev = iter;
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
  /*fprintf (stderr, "outside iterator bounds");*/
  return FALSE;
}

static void
gegl_path_list_calc_values (GeglPathList *path,
                            guint         num_samples,
                            gdouble      *xs,
                            gdouble      *ys)
{
  GeglPathList *iter = path;
  gdouble length = gegl_path_list_get_length (path);
  gfloat spacing = length / (num_samples-1);

  gfloat traveled = 0, next_pos = 0, next_sample = 0;
  gfloat x = 0, y = 0;

  gint i=0;

  while (iter)
    {
      /*fprintf (stderr, "%c, %i %i\n", iter->d.type, iter->d.point[0].x, iter->d.point[0].y);*/
      switch (iter->d.type)
        {
          case 'M':
            x = iter->d.point[0].x;
            y = iter->d.point[0].y;
            break;
          case 'L':
            {
              GeglPathPoint a,b;
              gfloat distance;

              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              distance = gegl_path_point_dist (&a, &b);
              next_pos += distance;

              while (next_sample <= next_pos)
                {
                  GeglPathPoint spot;
                  gfloat ratio = (next_sample - traveled) / (next_pos - traveled);

                  gegl_path_point_lerp (&spot, &a, &b, ratio);

                  xs[i]=spot.x;
                  ys[i]=spot.y;

                  next_sample += spacing;
                  i++;
                }
              if (!iter->next)
                {
                  xs[num_samples-1]=b.x;
                  ys[num_samples-1]=b.y;
                }

              x = b.x;
              y = b.y;

              traveled = next_pos;
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
}

static gdouble
gegl_path_list_get_length (GeglPathList *path)
{
  GeglPathList *iter = path;
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
              GeglPathPoint a,b;
              gfloat distance;

              a.x = x;
              a.y = y;

              b.x = iter->d.point[0].x;
              b.y = iter->d.point[0].y;

              distance = gegl_path_point_dist (&a, &b);
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

/***** GeglPathPoint *****/
void
gegl_path_point_lerp (GeglPathPoint  *dest,
                      GeglPathPoint  *a,
                      GeglPathPoint  *b,
                      gfloat  t)
{
  dest->x = a->x + (b->x-a->x) * t;
  dest->y = a->y + (b->y-a->y) * t;
}

gdouble
gegl_path_point_dist (GeglPathPoint *a,
                      GeglPathPoint *b)
{
  return sqrt ((a->x-b->x)*(a->x-b->x) +
               (a->y-b->y)*(a->y-b->y));
}

/* --------------------------------------------------------------------------
 * A GParamSpec class to describe behavior of GeglPath as an object property
 * follows.
 * --------------------------------------------------------------------------
 */

#define GEGL_PARAM_PATH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PARAM_PATH, GeglParamPath))
#define GEGL_IS_PARAM_PATH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_PARAM_PATH))

typedef struct _GeglParamPath GeglParamPath;

struct _GeglParamPath
{
  GParamSpec parent_instance;
};

static void
gegl_param_vector_init (GParamSpec *self)
{
}

static void
gegl_param_vector_finalize (GParamSpec *self)
{
  GParamSpecClass *parent_class = g_type_class_peek (g_type_parent (
                                                       GEGL_TYPE_PARAM_PATH));

  parent_class->finalize (self);
}

GType
gegl_param_path_get_type (void)
{
  static GType param_vector_type = 0;

  if (G_UNLIKELY (param_vector_type == 0))
    {
      static GParamSpecTypeInfo param_vector_type_info = {
        sizeof (GeglParamPath),
        0,
        gegl_param_vector_init,
        0,
        gegl_param_vector_finalize,
        NULL,
        NULL,
        NULL};
      param_vector_type_info.value_type = GEGL_TYPE_PATH;

      param_vector_type = g_param_type_register_static ("GeglParamPath",
                                                       &param_vector_type_info);
    }

  return param_vector_type;
}

GParamSpec *
gegl_param_spec_path   (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        GeglPath  *default_vector,
                        GParamFlags  flags)
{
  GeglParamPath *param_vector;

  param_vector = g_param_spec_internal (GEGL_TYPE_PARAM_PATH,
                                       name, nick, blurb, flags);

  return G_PARAM_SPEC (param_vector);
}
