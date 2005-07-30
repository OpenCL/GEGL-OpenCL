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

#include "gegl-object.h"
#include "gegl-node.h"

G_BEGIN_DECLS


#define GEGL_TYPE_OPERATION            (gegl_operation_get_type ())
#define GEGL_OPERATION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION, GeglOperation))
#define GEGL_OPERATION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION, GeglOperationClass))
#define GEGL_IS_OPERATION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION))
#define GEGL_IS_OPERATION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION))
#define GEGL_OPERATION_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION, GeglOperationClass))

#define MAX_PADS        16
#define MAX_INPUT_PADS  MAX_PADS
#define MAX_OUTPUT_PADS MAX_PADS

typedef struct _GeglOperationClass GeglOperationClass;

struct _GeglOperation
{
  GeglObject parent_instance;

  /*< private >*/
  GeglNode *node;  /* the node that this operation object is communicated
                      with through */
  gchar    *name;
  gboolean  constructed;

  gint input_pads;
  gint output_pads;
  gint required_input_pads;
  gint in_pad_fmt[MAX_INPUT_PADS];
  gint out_pad_fmt[MAX_OUTPUT_PADS];
};

struct _GeglOperationClass
{
  GeglObjectClass  parent_class;

  gint     (*query_in_pad_fmt)  (GeglOperation *operation,
                                 gint pad_no,
                                 gint fmt);
  gint     (*query_out_pad_fmt) (GeglOperation *operation,
                                 gint no,
                                 gint fmt);
  gint     (*set_in_pad_fmt)    (GeglOperation *operation,
                                 gint no,
                                 gint fmt);
  gint     (*set_out_pad_fmt)   (GeglOperation *operation,
                                 gint no,
                                 gint fmt);

  gboolean (*evaluate)          (GeglOperation *operation,
                                 const gchar   *output_pad);

  void     (*associate)         (GeglOperation *operation);
};


void
gegl_operation_create_pad (GeglOperation *self,
                           GParamSpec    *param_spec);

/* let the PADs have the formats accepted,..
 * sharing of pads between ops and nodes, or is that overkill?
 */

GType         gegl_operation_get_type  (void) G_GNUC_CONST;
void          gegl_operation_set_name  (GeglOperation *self,
                                        const gchar   *name);
const gchar * gegl_operation_get_name  (GeglOperation *self);
gboolean      gegl_operation_evaluate  (GeglOperation *self,
                                        const gchar   *output_pad);
void          gegl_operation_associate (GeglOperation *self,
                                        GeglNode      *node);
gboolean      gegl_operation_register  (GeglOperation *self,
                                        GeglNode      *node);
G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
