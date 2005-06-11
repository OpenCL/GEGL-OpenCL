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

#ifndef __GEGL_OPERATION_H__
#define __GEGL_OPERATION_H__

#include "gegl-node.h"

G_BEGIN_DECLS


#define GEGL_TYPE_OPERATION            (gegl_operation_get_type ())
#define GEGL_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION, GeglOperation))
#define GEGL_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION, GeglOperationClass))
#define GEGL_IS_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION))
#define GEGL_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION))
#define GEGL_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION, GeglOperationClass))


typedef struct _GeglOperationClass GeglOperationClass;

struct _GeglOperation
{
  GeglNode  parent_instance;
};

struct _GeglOperationClass
{
  GeglNodeClass parent_class;

  gboolean (* evaluate) (GeglOperation  *self,
                         const gchar    *output_prop);
};


GType      gegl_operation_get_type        (void) G_GNUC_CONST;

void       gegl_operation_create_property (GeglOperation  *self,
                                           GParamSpec     *param_spec);
gboolean   gegl_operation_evaluate        (GeglOperation  *self,
                                           const gchar    *output_prop);


G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
