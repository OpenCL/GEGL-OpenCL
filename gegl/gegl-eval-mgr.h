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

#ifndef __GEGL_EVAL_MGR_H__
#define __GEGL_EVAL_MGR_H__

#include "gegl-object.h"

G_BEGIN_DECLS


#define GEGL_TYPE_EVAL_MGR            (gegl_eval_mgr_get_type ())
#define GEGL_EVAL_MGR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_EVAL_MGR, GeglEvalMgr))
#define GEGL_EVAL_MGR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))
#define GEGL_IS_EVAL_MGR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_EVAL_MGR))
#define GEGL_IS_EVAL_MGR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_EVAL_MGR))
#define GEGL_EVAL_MGR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_EVAL_MGR, GeglEvalMgrClass))


typedef struct _GeglEvalMgrClass GeglEvalMgrClass;

struct _GeglEvalMgr
{
  GeglObject  parent_instance;
};

struct _GeglEvalMgrClass
{
  GeglObjectClass  parent_class;
};


GType   gegl_eval_mgr_get_type (void) G_GNUC_CONST;

void    gegl_eval_mgr_apply    (GeglEvalMgr *self,
                                GeglNode    *root,
                                const gchar *property_name);


G_END_DECLS

#endif /* __GEGL_EVAL_MGR_H__ */
