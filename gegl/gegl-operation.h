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
 *           2005, 2006 Øyvind Kolås
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
  GObject parent_instance;

  /*< private >*/
  GeglNode *node;  /* the node that this operation object is communicated
                      with through */
};

struct _GeglOperationClass
{
  GObjectClass  parent_class;

  const gchar *name;
  gchar       *description;
  char        *categories;  /* a colon seperated list of categories */

  gboolean (*process)             (GeglOperation *self,
                                   const gchar   *output_pad);

  void     (*associate)           (GeglOperation *self);
  void     (*prepare)             (GeglOperation *self);
  void     (*clean_pads)          (GeglOperation *self);

  GeglRect (*get_defined_region)  (GeglOperation *self);

  gboolean (*calc_source_regions) (GeglOperation *self);

  gboolean (*calc_result_rect)    (GeglOperation *self);
};



GType         gegl_operation_get_type           (void) G_GNUC_CONST;

void          gegl_operation_class_set_name (GeglOperationClass *self,
                                             const gchar        *name);
void          gegl_operation_class_set_description (GeglOperationClass *self,
                                                 const gchar        *description);

const gchar * gegl_operation_get_name       (GeglOperation *self);
void          gegl_operation_associate      (GeglOperation *self,
                                             GeglNode      *node);
void          gegl_operation_prepare        (GeglOperation *self);
gboolean      gegl_operation_process        (GeglOperation *self,
                                             const gchar   *output_pad);
void          gegl_operation_create_pad     (GeglOperation *self,
                                             GParamSpec    *param_spec);
gboolean      gegl_operation_register       (GeglOperation *self,
                                             GeglNode      *node);
void          gegl_operation_clean_pads     (GeglOperation *self);
GeglRect      gegl_operation_get_defined_region (GeglOperation *self);
gboolean      gegl_operation_calc_source_regions (GeglOperation *self);
gboolean      gegl_operation_calc_result_rect (GeglOperation *self);

/* this method defined for the Operation, even though it acts on the Node.
 * The rationale for this is that the knowledge for setting the rect
 * belongs on the Op side of the Node/Op pair
 */
void gegl_operation_set_have_rect (GeglOperation *operation,
                                   gint           x,
                                   gint           y,
                                   gint           width,
                                   gint           height);
GeglRect *gegl_operation_get_have_rect (GeglOperation *operation,
                                        const gchar   *input_pad_name);

void gegl_operation_set_need_rect (GeglOperation *operation,
                                   const gchar   *input_pad_name,
                                   gint           x,
                                   gint           y,
                                   gint           width,
                                   gint           height);

void gegl_operation_set_result_rect (GeglOperation *operation,
                                     gint           x,
                                     gint           y,
                                     gint           width,
                                     gint           height);

GeglRect *gegl_operation_need_rect     (GeglOperation *operation);
GeglRect *gegl_operation_have_rect     (GeglOperation *operation);
GeglRect *gegl_operation_result_rect   (GeglOperation *operation);

G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
