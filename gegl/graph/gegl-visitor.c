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
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl.h"
#include "graph/gegl-node.h"
#include "graph/gegl-pad.h"
#include "gegl-visitor.h"
#include "gegl-visitable.h"


typedef struct _GeglVisitInfo GeglVisitInfo;

struct _GeglVisitInfo
{
  gboolean visited;
  gboolean discovered;
  gint     shared_count;
};

enum
{
  PROP_0,
  PROP_ID
};


static void           gegl_visitor_class_init  (GeglVisitorClass *klass);
static void           gegl_visitor_init        (GeglVisitor      *self);
static void           finalize                 (GObject          *gobject);
static void           init_dfs_traversal       (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           dfs_traverse             (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           init_bfs_traversal       (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           visit_info_destroy       (GeglVisitInfo    *visit_info);
static void           insert                   (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static GeglVisitInfo* lookup                   (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static gboolean       get_visited              (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           set_visited              (GeglVisitor      *self,
                                                GeglVisitable    *visitable,
                                                gboolean          visited);
static gboolean       get_discovered           (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           set_discovered           (GeglVisitor      *self,
                                                GeglVisitable    *visitable,
                                                gboolean          discovered);
static gint           get_shared_count         (GeglVisitor      *self,
                                                GeglVisitable    *visitable);
static void           set_shared_count         (GeglVisitor      *self,
                                                GeglVisitable    *visitable,
                                                gint              shared_count);
static void           visit_pad                (GeglVisitor      *self,
                                                GeglPad          *pad);
static void           visit_node               (GeglVisitor      *self,
                                                GeglNode         *node);
static void           set_property             (GObject          *gobject,
                                                guint             prop_id,
                                                const GValue     *value,
                                                GParamSpec       *pspec);
static void           get_property             (GObject          *gobject,
                                                guint             prop_id,
                                                GValue           *value,
                                                GParamSpec       *pspec);



G_DEFINE_TYPE (GeglVisitor, gegl_visitor, G_TYPE_OBJECT)


static void
gegl_visitor_class_init (GeglVisitorClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = finalize;

  klass->visit_pad            = visit_pad;
  klass->visit_node           = visit_node;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;

  g_object_class_install_property (gobject_class, PROP_ID,
                                   g_param_spec_pointer ("id",
                                                         "evaluation-id",
                                                         "The identifier for the evaluation context",
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_READWRITE));
}


static void
set_property (GObject      *gobject,
              guint         property_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  GeglVisitor *self = GEGL_VISITOR (gobject);

  switch (property_id)
    {
      case PROP_ID:
        self->context_id = g_value_get_pointer (value);
        break;

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
  GeglVisitor *self = GEGL_VISITOR (gobject);

  switch (property_id)
    {
      case PROP_ID:
        g_value_set_pointer (value, self->context_id);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, property_id, pspec);
        break;
    }
}



static void
gegl_visitor_init (GeglVisitor *self)
{
  self->visits_list = NULL;
  self->hash = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                      NULL,
                                      (GDestroyNotify) visit_info_destroy);
}

static void
finalize (GObject *gobject)
{
  GeglVisitor *self = GEGL_VISITOR (gobject);

  g_slist_free (self->visits_list);
  g_hash_table_destroy (self->hash);

  G_OBJECT_CLASS (gegl_visitor_parent_class)->finalize (gobject);
}

static GeglVisitInfo *
lookup (GeglVisitor   *self,
        GeglVisitable *visitable)
{
  return g_hash_table_lookup (self->hash, visitable);
}

/* resets the object's data (list of visits and visitable statuses) */
void gegl_visitor_reset (GeglVisitor   *self)
{
  if (self->visits_list)
    {
      g_slist_free (self->visits_list);
      self->visits_list = NULL;
    }
  g_hash_table_remove_all (self->hash);
}

/* Inserts the visitable into the object's hash table of visitables with the
 * object as a key and a new GeglVisitInfo (zero initialised) as object
 */
static void
insert (GeglVisitor   *self,
        GeglVisitable *visitable)
{
  if (lookup (self, visitable))
    {
      g_warning ("visitable already in visitor's hash table");
    }
  else
    {
      g_hash_table_insert (self->hash, visitable, g_slice_new0 (GeglVisitInfo));
    }
}

/* Returns TRUE if a GeglVisitable has already been visited */
static gboolean
get_visited (GeglVisitor   *self,
             GeglVisitable *visitable)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  return visit_info->visited;
}

static void
set_visited (GeglVisitor   *self,
             GeglVisitable *visitable,
             gboolean       visited)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  visit_info->visited = visited;
}

static gboolean
get_discovered (GeglVisitor   *self,
                GeglVisitable *visitable)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  return visit_info->discovered;
}

static void
set_discovered (GeglVisitor   *self,
                GeglVisitable *visitable,
                gboolean       discovered)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  visit_info->discovered = discovered;
}

static gint
get_shared_count (GeglVisitor   *self,
                  GeglVisitable *visitable)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  return visit_info->shared_count;
}

static void
set_shared_count (GeglVisitor   *self,
                  GeglVisitable *visitable,
                  gint           shared_count)
{
  GeglVisitInfo *visit_info = lookup (self, visitable);

  g_assert (visit_info);

  visit_info->shared_count = shared_count;
}

/**
 * gegl_visitor_get_visits_list
 * @self: a #GeglVisitor.
 *
 * Gets a list of the visitables the visitor has visited so far.
 *
 * Returns: A list of the visitables visited by this visitor.
 **/
GSList *
gegl_visitor_get_visits_list (GeglVisitor *self)
{
  g_return_val_if_fail (GEGL_IS_VISITOR (self), NULL);

  return self->visits_list;
}

static void
visit_info_destroy (GeglVisitInfo *visit_info)
{
  g_slice_free (GeglVisitInfo, visit_info);
}

/**
 * gegl_visitor_dfs_traverse:
 * @self: #GeglVisitor
 * @visitable: the start #GeglVisitable
 *
 * Traverse depth first starting at @visitable.
 **/
void
gegl_visitor_dfs_traverse (GeglVisitor   *self,
                           GeglVisitable *visitable)
{
  /* sets up the structures that keeps track of the */
  init_dfs_traversal (self, visitable);
  dfs_traverse (self, visitable);
}

/* Recursively (depth first) sets up the structure (hash) that keeps track of
 * if a visitable's status
 */
static void
init_dfs_traversal (GeglVisitor   *self,
                    GeglVisitable *visitable)
{
  GSList *depends_on_list;
  GSList *llink;

  /* add the visitable to the list */
  insert (self, visitable);
  depends_on_list = gegl_visitable_depends_on (visitable);
  llink           = depends_on_list;

  while (llink)
    {
      GeglVisitable *visitable  = llink->data;
      GeglVisitInfo *visit_info = lookup (self, visitable);

      /* if the visitable doesn't have a visit_info,
       * then it needs to be initialised
       */
      if (!visit_info)
        init_dfs_traversal (self, visitable);

      llink = g_slist_next (llink);
    }

  g_slist_free (depends_on_list);
}

/* Recursively (depth first) traverses the visitables and call's their
 * accept methods
 */
static void
dfs_traverse (GeglVisitor   *self,
              GeglVisitable *visitable)
{
  GSList *depends_on_list;
  GSList *llink;

  depends_on_list = gegl_visitable_depends_on (visitable);
  llink           = depends_on_list;

  while (llink)
    {
      GeglVisitable *visitable = llink->data;

      /* if the visitable has not yet been visitied then visit it */
      if (!get_visited (self, visitable))
        dfs_traverse (self, visitable);

      llink = g_slist_next (llink);
    }

  g_slist_free (depends_on_list);

  /* trigger the actual visit (call the visitable's accept method that will
   * call the visitable's visit method (c.f. the visitor pattern)
   */
  gegl_visitable_accept (visitable, self);
  /* mark the visitable as already visited*/
  set_visited (self, visitable, TRUE);
}

static void
init_bfs_traversal (GeglVisitor   *self,
                    GeglVisitable *visitable)
{
  GSList *depends_on_list;
  GSList *llink;

  insert (self, visitable);

  depends_on_list = gegl_visitable_depends_on (visitable);
  llink           = depends_on_list;

  while (llink)
    {
      gint           shared_count;
      GeglVisitable *depends_on_visitable = llink->data;
      GeglVisitInfo *visit_info           = lookup (self, depends_on_visitable);

      if (!visit_info)
        init_bfs_traversal (self, depends_on_visitable);

      shared_count = get_shared_count (self, depends_on_visitable);
      shared_count++;
      set_shared_count (self, depends_on_visitable, shared_count);
      llink = g_slist_next (llink);
    }

  g_slist_free (depends_on_list);
}

/**
 * gegl_visitor_bfs_traverse:
 * @self: a #GeglVisitor
 * @visitable: the root #GeglVisitable.
 *
 * Traverse breadth-first starting at @visitable.
 **/
void
gegl_visitor_bfs_traverse (GeglVisitor   *self,
                           GeglVisitable *visitable)
{
  GQueue  queue = G_QUEUE_INIT;

  /* Init all visitables */
  init_bfs_traversal (self, visitable);

  /* Initialize the queue with this visitable */
  g_queue_push_head (&queue, visitable);

  /* Mark visitable as "discovered" */
  set_discovered (self, visitable, TRUE);

  /* Pop the top of the queue*/
  while ((visitable = g_queue_pop_head (&queue)))
    {
      gint shared_count = get_shared_count (self, visitable);

      /* Put this one at the end of the queue if its active
       * immediate parents haven't all been visited yet.
       */
      if (shared_count > 0)
        {
          g_queue_push_tail (&queue, visitable);
          continue;
        }

      /* Loop through visitable's sources and examine them */
      {
        GSList *depends_on_list = gegl_visitable_depends_on (visitable);
        GSList *llink;

        for (llink = depends_on_list; llink; llink = g_slist_next (llink))
          {
            GeglVisitable *depends_on_visitable = llink->data;

            shared_count = get_shared_count (self, depends_on_visitable);
            shared_count--;
            set_shared_count (self, depends_on_visitable, shared_count);

            /* Add any undiscovered visitable to the queue at end */
            if (!get_discovered (self, depends_on_visitable))
              {
                g_queue_push_tail (&queue, depends_on_visitable);

                /* Mark it as discovered */
                set_discovered (self, depends_on_visitable, TRUE);
              }
          }

        g_slist_free (depends_on_list);
      }

      /* Visit the visitable */
      gegl_visitable_accept (visitable, self);
      set_visited (self, visitable, TRUE);
    }
}

/* should be called by extending classes when their visit_pad function
 * is called
 */
void
gegl_visitor_visit_pad (GeglVisitor *self,
                        GeglPad     *pad)
{
  GeglVisitorClass *klass;

  klass = GEGL_VISITOR_GET_CLASS (self);

  if (klass->visit_pad)
    klass->visit_pad (self, pad);
}

static void
visit_pad (GeglVisitor *self,
           GeglPad     *pad)
{
  self->visits_list = g_slist_prepend (self->visits_list, pad);
}

/* should be called by extending classes when their visit_node function
 * is called
 */
void
gegl_visitor_visit_node (GeglVisitor *self,
                         GeglNode    *node)
{
  GeglVisitorClass *klass;

  klass = GEGL_VISITOR_GET_CLASS (self);

  if (klass->visit_node)
    klass->visit_node (self, node);
}

/* adds the visiting node to the list of visits */
static void
visit_node (GeglVisitor *self,
            GeglNode    *node)
{
#if 0
#if ENABLE_MT
  g_mutex_lock (node->mutex);
#endif
#endif
  self->visits_list = g_slist_prepend (self->visits_list, node);
#if 0
#if ENABLE_MT
  g_mutex_unlock (node->mutex);
#endif
#endif
}
