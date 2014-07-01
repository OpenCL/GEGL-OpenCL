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
 * Copyright 2013 Daniel Sabo
 */

#ifndef __GEGL_EVAL_MANAGER_H__
#define __GEGL_EVAL_MANAGER_H__

#include "gegl-types-internal.h"
#include "buffer/gegl-buffer-types.h"
#include "process/gegl-graph-traversal.h"

G_BEGIN_DECLS


typedef enum
{
  INVALID,
  READY
} GeglEvalManagerStates;


#define GEGL_TYPE_EVAL_MANAGER            (gegl_eval_manager_get_type ())
#define GEGL_EVAL_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_MANAGER, GeglEvalManager))
#define GEGL_EVAL_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_MANAGER, GeglEvalManagerClass))
#define GEGL_IS_EVAL_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_MANAGER))
#define GEGL_IS_EVAL_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_MANAGER))
#define GEGL_EVAL_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_MANAGER, GeglEvalManagerClass))


typedef struct _GeglEvalManagerClass GeglEvalManagerClass;

struct _GeglEvalManager
{
  GObject    parent_instance;
  GeglNode  *node;
  gchar     *pad_name;

  GeglGraphTraversal    *traversal;
  GeglEvalManagerStates  state;

};

struct _GeglEvalManagerClass
{
  GObjectClass  parent_class;
};


GType             gegl_eval_manager_get_type (void) G_GNUC_CONST;

void              gegl_eval_manager_prepare  (GeglEvalManager     *self);
GeglRectangle     gegl_eval_manager_get_bounding_box (GeglEvalManager     *self);

GeglBuffer *      gegl_eval_manager_apply    (GeglEvalManager     *self,
                                              const GeglRectangle *roi,
                                              gint                 level);
GeglEvalManager * gegl_eval_manager_new      (GeglNode        *node,
                                              const gchar     *pad_name);

G_END_DECLS

#endif /* __GEGL_EVAL_MANAGER_H__ */
