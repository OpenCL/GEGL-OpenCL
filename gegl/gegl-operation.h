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

  const gchar *name;        /* name used to refer to this type of operation
                               in GEGL */
  gchar       *description; /* textual description of the operation */
  char        *categories;  /* a colon seperated list of categories */

  /* associate this operation with a GeglNode, override this if you are
   * creating a GeglGraph, it is already defined for Filters/Sources/Composers.
   */
  void       (*associate)           (GeglOperation *self);

  /* prepare the node for processing (all properties will be set)
   * override this if you are creating a meta operation (using the node
   * as a GeglGraph).
   */
  void       (*prepare)             (GeglOperation *self);

  /* Returns a bounding rectangle for the data that is defined by
   * this op. (is already implemented for GeglOperationPointFilter and
   * GeglOperationPointComposer.
   */
  GeglRect   (*get_defined_region)  (GeglOperation *self);

  /* Compute the region of interests on out own sources (and use
   * gegl_operation_set_source_region() on each of them).
   */
  gboolean   (*calc_source_regions) (GeglOperation *self);

  /* do the actual processing needed to put GeglBuffers on the output pad */
  gboolean   (*process)             (GeglOperation *self,
                                     const gchar   *output_pad);
};

/* returns the ROI passed to _this_ operation */
GeglRect * gegl_operation_get_requested_region      (GeglOperation *operation);

/* retrieves the bounding box of a connected input */
GeglRect * gegl_operation_source_get_defined_region (GeglOperation *operation,
                                                     const gchar   *pad_name);

/* sets the ROI needed to be computed on one of the sources */
void       gegl_operation_set_source_region         (GeglOperation *operation,
                                                     const gchar   *pad_name,
                                                     GeglRect      *region);

/* returns the bounding box of the buffer that needs to be computed */
GeglRect * gegl_operation_result_rect               (GeglOperation *operation);


/* virtual method invokers */
GeglRect   gegl_operation_get_defined_region        (GeglOperation *self);
gboolean   gegl_operation_calc_source_regions       (GeglOperation *self);
void       gegl_operation_associate                 (GeglOperation *self,
                                                     GeglNode      *node);
void       gegl_operation_prepare                   (GeglOperation *self);

gboolean   gegl_operation_process                   (GeglOperation *self,
                                                     const gchar   *output_pad);

GType      gegl_operation_get_type                  (void) G_GNUC_CONST;
void       gegl_operation_class_set_name            (GeglOperationClass *self,
                                                     const gchar        *name);
void       gegl_operation_class_set_description     (GeglOperationClass *self,
                                                     const gchar        *description);
void       gegl_operation_create_pad                (GeglOperation *self,
                                                     GParamSpec    *param_spec);

G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
