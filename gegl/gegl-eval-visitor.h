/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */

#ifndef __GEGL_EVAL_VISITOR_H__
#define __GEGL_EVAL_VISITOR_H__

#include "gegl-visitor.h"

G_BEGIN_DECLS


#define GEGL_TYPE_EVAL_VISITOR            (gegl_eval_visitor_get_type ())
#define GEGL_EVAL_VISITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitor))
#define GEGL_EVAL_VISITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitorClass))
#define GEGL_IS_EVAL_VISITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_VISITOR))
#define GEGL_IS_EVAL_VISITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_VISITOR))
#define GEGL_EVAL_VISITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_VISITOR, GeglEvalVisitorClass))


typedef struct _GeglEvalVisitorClass GeglEvalVisitorClass;

struct _GeglEvalVisitor
{
  GeglVisitor  parent_instance;
};

struct _GeglEvalVisitorClass
{
  GeglVisitorClass  parent_class;
};


GType   gegl_eval_visitor_get_type (void) G_GNUC_CONST;


G_END_DECLS

#endif /* __GEGL_EVAL_VISITOR_H__ */
