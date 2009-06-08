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

#ifndef __GEGL_EVAL_MGR_H__
#define __GEGL_EVAL_MGR_H__

#include "gegl-types-internal.h"
#include "buffer/gegl-buffer-types.h"

G_BEGIN_DECLS


typedef enum
{
  UNINITIALIZED,

  /* means we need to redo an extra prepare and have_rect
   * traversal
   */
  NEED_REDO_PREPARE_AND_HAVE_RECT_TRAVERSAL,

  /* means we need a prepare traversal to set up the contexts on the
   * nodes
   */
  NEED_CONTEXT_SETUP_TRAVERSAL
} GeglEvalMgrStates;


#define GEGL_TYPE_EVAL_MGR            (gegl_eval_mgr_get_type ())
#define GEGL_EVAL_MGR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_MGR, GeglEvalMgr))
#define GEGL_EVAL_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))
#define GEGL_IS_EVAL_MGR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_MGR))
#define GEGL_IS_EVAL_MGR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_MGR))
#define GEGL_EVAL_MGR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))


typedef struct _GeglEvalMgrClass GeglEvalMgrClass;

struct _GeglEvalMgr
{
  GObject    parent_instance;
  GeglNode  *node;
  gchar     *pad_name;
  GeglRectangle roi;

  /* whether we can fire off rendering requests straight
   * away or we have to re-prepare etc of the graph
   */
  GeglEvalMgrStates state;

  /* we keep these objects around, they are too expensive to throw away */
  GeglVisitor *prepare_visitor;
  GeglVisitor *need_visitor;
  GeglVisitor *eval_visitor;
  GeglVisitor *have_visitor;
  GeglVisitor *finish_visitor;

};

struct _GeglEvalMgrClass
{
  GObjectClass  parent_class;
};


GType        gegl_eval_mgr_get_type (void) G_GNUC_CONST;

GeglBuffer * gegl_eval_mgr_apply    (GeglEvalMgr *self);
GeglEvalMgr * gegl_eval_mgr_new     (GeglNode *node,
                                     const gchar *pad_name);

G_END_DECLS

#endif /* __GEGL_EVAL_MGR_H__ */
