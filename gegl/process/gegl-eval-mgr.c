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
 *           2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-eval-mgr.h"
#include "gegl-eval-visitor.h"
#include "gegl-debug-rect-visitor.h"
#include "gegl-need-visitor.h"
#include "gegl-have-visitor.h"
#include "gegl-instrument.h"
#include "graph/gegl-node.h"
#include "gegl-prepare-visitor.h"
#include "gegl-finish-visitor.h"
#include "graph/gegl-pad.h"
#include "graph/gegl-visitable.h"
#include "operation/gegl-operation.h"
#include <stdlib.h>


static void gegl_eval_mgr_class_init (GeglEvalMgrClass *klass);
static void gegl_eval_mgr_init (GeglEvalMgr *self);
static void gegl_eval_mgr_finalize (GObject *self_object);

G_DEFINE_TYPE (GeglEvalMgr, gegl_eval_mgr, G_TYPE_OBJECT)

static void
gegl_eval_mgr_class_init (GeglEvalMgrClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gegl_eval_mgr_finalize;
}

static void
gegl_eval_mgr_init (GeglEvalMgr *self)
{
  GeglRectangle roi = { 0, 0, -1, -1 };
  gpointer     context_id = self;

  self->roi = roi;
  self->prepare_visitor = g_object_new (GEGL_TYPE_PREPARE_VISITOR, "id", context_id, NULL);
  self->have_visitor = g_object_new (GEGL_TYPE_HAVE_VISITOR, "id", context_id, NULL);
  self->eval_visitor = g_object_new (GEGL_TYPE_EVAL_VISITOR, "id", context_id, NULL);
  self->need_visitor = g_object_new (GEGL_TYPE_NEED_VISITOR, "id", context_id, NULL);
  self->finish_visitor = g_object_new (GEGL_TYPE_FINISH_VISITOR, "id", context_id, NULL);
  self->state = UNINITIALIZED;
}

static void
gegl_eval_mgr_finalize (GObject *self_object)
{
  GeglEvalMgr *self = GEGL_EVAL_MGR (self_object);
#if 0
  GeglNode    *root;
  GeglPad     *pad;
  root=self->node;
  pad = gegl_node_get_pad (root, self->pad_name);
  /* Use the redirect output NOP of a graph instead of a graph if a traversal
   * is attempted directly on a graph */
  if (pad && pad->node != self->node)
    root = pad->node;
  else
    root = self->node;

  gegl_visitor_reset (self->finish_visitor);
  gegl_visitor_dfs_traverse (self->finish_visitor, GEGL_VISITABLE (root));
#endif

  g_object_unref (self->prepare_visitor);
  g_object_unref (self->have_visitor);
  g_object_unref (self->eval_visitor);
  g_object_unref (self->need_visitor);
  g_object_unref (self->finish_visitor);
  g_free (self->pad_name);

  G_OBJECT_CLASS (gegl_eval_mgr_parent_class)->finalize (self_object);
}

static gboolean
gegl_eval_mgr_change_notification (GObject             *gobject,
                                   const GeglRectangle *rect,
                                   gpointer             user_data)
{
  GeglEvalMgr *mgr = GEGL_EVAL_MGR (user_data);

  if (mgr->node != NULL)
    {
      gpointer              context_id = mgr;
      GeglOperationContext *context    = gegl_node_get_context (mgr->node,
                                                                context_id);
      if (context != NULL)
        {
          /* If the node is changed we can't use the cache any longer */
          context->cached = FALSE;
        }
    }

  if (mgr->state != UNINITIALIZED)
    {
      mgr->state = NEED_REDO_PREPARE_AND_HAVE_RECT_TRAVERSAL;
    }

  return FALSE;
}


GeglBuffer *
gegl_eval_mgr_apply (GeglEvalMgr *self)
{
  GeglNode    *root;
  GeglBuffer  *object;
  GeglPad     *pad;
  glong        time       = gegl_ticks ();
  gpointer     context_id = self;

  g_assert (GEGL_IS_EVAL_MGR (self));

  gegl_instrument ("gegl", "process", 0);

  root=self->node;
  pad = gegl_node_get_pad (root, self->pad_name);
  /* Use the redirect output NOP of a graph instead of a graph if a traversal
   * is attempted directly on a graph */
  if (pad && pad->node != self->node)
    root = pad->node;
  else
    root = self->node;
  g_assert (root);

  g_object_ref (root);

  /* do the necessary set-up work (all using depth first traversal) */
  switch (self->state)
    {
      case UNINITIALIZED:
        /* Set up the node's context and "needed rectangle"*/
        gegl_visitor_reset (self->prepare_visitor);
        gegl_visitor_dfs_traverse (self->prepare_visitor, GEGL_VISITABLE (root));
        /* No idea why there is a second call */
        gegl_visitor_reset (self->prepare_visitor);
        gegl_visitor_dfs_traverse (self->prepare_visitor, GEGL_VISITABLE (root));
      case NEED_REDO_PREPARE_AND_HAVE_RECT_TRAVERSAL:
        /* sets up the node's rect (bounding box) */
        gegl_visitor_reset (self->have_visitor);
        gegl_visitor_dfs_traverse (self->have_visitor, GEGL_VISITABLE (root));
      case NEED_CONTEXT_SETUP_TRAVERSAL:

        gegl_visitor_reset (self->prepare_visitor);
        gegl_visitor_dfs_traverse (self->prepare_visitor, GEGL_VISITABLE (root));
        self->state = NEED_CONTEXT_SETUP_TRAVERSAL;
     }

  /* set up the root node */
  if (self->roi.width == -1 &&
      self->roi.height == -1)
    {
      self->roi = root->have_rect;
    }

  gegl_node_set_need_rect (root, context_id, &self->roi);

  /* set up the context's rectangle (breadth first traversal) */
  gegl_visitor_reset (self->need_visitor);

  /* should the need rect be moved into the context, making this
   * part of gegl re-entrable without locking?.. or does that
   * hamper other useful API that depends on the need_rect to be
   * in the nodes?
   */
  gegl_visitor_bfs_traverse (self->need_visitor, GEGL_VISITABLE (root));

#if 0
  if (g_getenv ("GEGL_DEBUG_RECTS") != NULL)
    {
      GeglVisitor *debug_rect_visitor;

      g_warning ("---------------------");
      debug_rect_visitor = g_object_new (GEGL_TYPE_DEBUG_RECT_VISITOR, "id", context_id, NULL);
      gegl_visitor_dfs_traverse (debug_rect_visitor, GEGL_VISITABLE (root));
      g_object_unref (debug_rect_visitor);
    }
#endif

  /* now let's do the real work */
  gegl_visitor_reset (self->eval_visitor);
  if (pad)
    {
      gegl_visitor_dfs_traverse (self->eval_visitor, GEGL_VISITABLE (pad));
    }
  else
    { /* pull on the input of our sink if no pad of the given pad-name
         was available, we take this as an indication that we're in fact
         doing processing on a sink (and the ROI inidcates the data to
         be written, note that GEGL might subdivide this roi
         in its processing.
       */
      GeglPad *pad = gegl_node_get_pad (root, "input");
      gegl_visitor_dfs_traverse (self->eval_visitor, GEGL_VISITABLE (pad));
    }

  if (pad)
    {
      /* extract returned object before running finish visitor */
      GValue value = { 0, };
      g_value_init (&value, G_TYPE_OBJECT);
      gegl_operation_context_get_property (gegl_node_get_context (root, context_id),
                                      "output", &value);
      object = g_value_get_object (&value);
      g_object_ref (object);/* salvage buffer from finalization */
      g_value_unset (&value);
    }

  /* do the clean up */
  gegl_visitor_reset (self->finish_visitor);
  gegl_visitor_dfs_traverse (self->finish_visitor, GEGL_VISITABLE (root));

  g_object_unref (root);
  time = gegl_ticks () - time;
  gegl_instrument ("gegl", "process", time);

  if (!pad || !G_IS_OBJECT (object))
    {
      return NULL;
    }
  return object;
}

GeglEvalMgr * gegl_eval_mgr_new     (GeglNode *node,
                                     const gchar *pad_name)
{
  GeglEvalMgr *self = g_object_new (GEGL_TYPE_EVAL_MGR, NULL);
  g_assert (GEGL_IS_NODE (node));
  self->node = node;
  if (pad_name)
    self->pad_name = g_strdup (pad_name);
  else
    self->pad_name = g_strdup ("output");
  /*g_signal_connect (G_OBJECT (self->node->operation), "notify", G_CALLBACK (gegl_eval_mgr_change_notification), self);*/
  g_signal_connect (G_OBJECT (self->node), "invalidated", G_CALLBACK (gegl_eval_mgr_change_notification), self);
  g_signal_connect (G_OBJECT (self->node), "notify", G_CALLBACK (gegl_eval_mgr_change_notification), self);
  return self;
}
