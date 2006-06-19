/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 *           2006 Øyvind Kolås
 */

#include "config.h"

#include <glib-object.h>

#include "gegl-types.h"

#include "gegl-eval-mgr.h"
#include "gegl-have-visitor.h"
#include "gegl-need-visitor.h"
#include "gegl-cr-visitor.h"
#include "gegl-debug-rect-visitor.h"
#include "gegl-eval-visitor.h"
#include "gegl-node.h"
#include "gegl-operation.h"
#include "gegl-visitable.h"
#include "gegl-pad.h"
#include <stdlib.h>


static void gegl_eval_mgr_class_init (GeglEvalMgrClass *klass);
static void gegl_eval_mgr_init       (GeglEvalMgr      *self);


G_DEFINE_TYPE(GeglEvalMgr, gegl_eval_mgr, GEGL_TYPE_OBJECT)


static void
gegl_eval_mgr_class_init (GeglEvalMgrClass * klass)
{
}

static void
gegl_eval_mgr_init (GeglEvalMgr *self)
{
  GeglRect roi={0,0,-1,-1};
  self->roi = roi;
}

/**
 * gegl_eval_mgr_apply:
 * @self: a #GeglEvalMgr.
 * @root:
 * @property_name:
 *
 * Update this property.
 **/
void
gegl_eval_mgr_apply (GeglEvalMgr *self,
                     GeglNode    *root,
                     const gchar *pad_name)
{
  GeglVisitor  *have_visitor;
  GeglVisitor  *need_visitor;
  GeglVisitor  *cr_visitor;
  GeglVisitor  *eval_visitor;
  GeglPad      *pad;

  g_return_if_fail (GEGL_IS_EVAL_MGR (self));
  g_return_if_fail (GEGL_IS_NODE (root));

  if (pad_name == NULL)
    pad_name = "output";
  pad = gegl_node_get_pad (root, pad_name);

  /* Use the redirect output NOP of a graph instead of a graph if a traversal
   * is attempted directly on a graph */
  if (pad->node != root)
    root = pad->node;
  g_object_ref (root);

  have_visitor = g_object_new (GEGL_TYPE_HAVE_VISITOR, NULL);
  gegl_visitor_dfs_traverse (have_visitor, GEGL_VISITABLE(root));
  g_object_unref (have_visitor);

  if (self->roi.w==-1 &&
      self->roi.h==-1)
    {
      GeglRect *root_have_rect = gegl_node_get_have_rect (root);
      g_assert (root_have_rect);
      self->roi = *root_have_rect;
    }

  gegl_node_set_need_rect (root, self->roi.x, self->roi.y,
                                 self->roi.w, self->roi.h);
  root->is_root = TRUE;

  need_visitor = g_object_new (GEGL_TYPE_NEED_VISITOR, NULL);
  gegl_visitor_bfs_traverse (need_visitor, GEGL_VISITABLE(root));
  g_object_unref (need_visitor);

  cr_visitor = g_object_new (GEGL_TYPE_CR_VISITOR, NULL);
  gegl_visitor_bfs_traverse (cr_visitor, GEGL_VISITABLE(root));
  g_object_unref (cr_visitor);

  root->result_rect = self->roi;


  if(getenv("GEGL_DEBUG_RECTS")!=NULL)
    {
      GeglVisitor  *debug_rect_visitor;

      debug_rect_visitor = g_object_new (GEGL_TYPE_DEBUG_RECT_VISITOR, NULL);
        gegl_visitor_dfs_traverse (debug_rect_visitor, GEGL_VISITABLE(root));
      g_object_unref (debug_rect_visitor);
    }

  eval_visitor = g_object_new (GEGL_TYPE_EVAL_VISITOR, NULL);
  gegl_visitor_dfs_traverse (eval_visitor, GEGL_VISITABLE(pad));
  g_object_unref (eval_visitor);

  root->is_root = FALSE;

  g_object_unref (root);
}
