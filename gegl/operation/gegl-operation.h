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
 *           2005-2008 Øyvind Kolås
 */

#ifndef __GEGL_OPERATION_H__
#define __GEGL_OPERATION_H__

#include <glib-object.h>
#include <babl/babl.h>


#include "gegl-buffer.h"

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
  GObject parent_instance;

  /*< private >*/
  GeglNode *node;  /* the node that this operation object is communicated
                      with through */
};

struct _GeglOperationClass
{
  GObjectClass  parent_class;

  const gchar  *name;            /* name used to refer to this type of
                                    operation in GEGL */
  const gchar  *description;     /* textual description of the operation */
  const gchar  *categories;      /* a colon seperated list of categories */


  gboolean          no_cache;    /* do not create a cache for this operation */

  /* attach this operation with a GeglNode, override this if you are creating a
   * GeglGraph, it is already defined for Filters/Sources/Composers.
   */
  void            (*attach)               (GeglOperation *operation);

  /* prepare() is called on each operation providing data to a node that
   * is requested to provide a rendered result. When prepare is called all
   * properties are known. This is the time to set desired pixel formats
   * for input and output pads.
   */
  void            (*prepare)              (GeglOperation *operation);

  /* Returns the bounding rectangle for the data that is defined by this op.
   * (is already implemented in GeglOperationPointFilter and
   * GeglOperationPointComposer, GeglOperationAreaFilter base classes.
   */
  GeglRectangle   (*get_bounding_box)   (GeglOperation *operation);

  /* Computes the region in output (same affected rect assumed for all outputs)
   * when a given region has changed on an input. Used to aggregate dirt in the
   * graph. A default implementation of this, if not provided should probably
   * be to report that the entire defined region is dirtied.
   */
  GeglRectangle   (*get_required_for_output)  (GeglOperation       *operation,
                                               const gchar         *input_pad,
                                               const GeglRectangle *input_region);

  /* computes the rectangle needed to be correctly computed in a buffer
   * on the named input_pad, for a given result rectangle
   */
  GeglRectangle   (*get_invalidated_by_change) (GeglOperation       *operation,
                                                const gchar         *input_pad,
                                                const GeglRectangle *roi);

  /* Adjust result rect, adapts the rectangle used for computing results.
   * (useful for global operations like contrast stretching, as well as
   * file loaders to force caching of the full raster).
   */
  GeglRectangle   (*adjust_result_region)  (GeglOperation       *operation,
                                            const GeglRectangle *roi);

  /* XXX: What is GeglNode doing in this part of the API?
   * Returns the node providing data for a specific location within the
   * operations output. Does this recurse?, perhaps it should only point
   * out which pad the data is coming from?
   */
  GeglNode*       (*detect)                (GeglOperation       *operation,
                                            gint                 x,
                                            gint                 y);

  /* Perform processing for the @output_pad, pad The result_rect provides
   * the region to process. For sink operations @output_pad can be ignored
   * but the result_rect is then then indicating the data available for
   * consumption.
   */
  gboolean        (*process)               (GeglOperation       *operation,
                                            GeglNodeContext     *context,
                                            const gchar         *output_pad,
                                            const GeglRectangle *result_rect);
};

GType           gegl_operation_get_type             (void) G_GNUC_CONST;

/* retrieves the bounding box of a connected input */
GeglRectangle * gegl_operation_source_get_bounding_box (GeglOperation *operation,
                                                          const gchar   *pad_name);


/* sets the ROI needed to be computed on one of the sources */
void            gegl_operation_set_source_region    (GeglOperation       *operation,
                                                     gpointer             context_id,
                                                     const gchar         *pad_name,
                                                     const GeglRectangle *region);


/* retrieves the node providing data to a named input pad */
GeglNode      * gegl_operation_get_source_node         (GeglOperation *operation,
                                                        const gchar   *pad_name);
GeglRectangle   gegl_operation_get_required_for_output (GeglOperation *operation,
                                                        const gchar   *input_pad,
                                                        const GeglRectangle *input_region);
GeglRectangle   gegl_operation_get_bounding_box        (GeglOperation *operation);
GeglRectangle   gegl_operation_adjust_result_region    (GeglOperation *operation,
                                                        const GeglRectangle *roi);

GeglRectangle   gegl_operation_get_invalidated_by_change(GeglOperation *operation,
                                                         const gchar   *input_pad,
                                                         const GeglRectangle *roi);

GeglNode       *gegl_operation_detect                   (GeglOperation *operation,
                                                         gint           x,
                                                         gint           y);


/* virtual method invokers that change behavior based on the roi being computed,
 * needs a context_id being based that is used for storing context data.
 */

void            gegl_operation_attach               (GeglOperation       *operation,
                                                     GeglNode            *node);
void            gegl_operation_prepare              (GeglOperation       *operation);
gboolean        gegl_operation_process              (GeglOperation       *operation,
                                                     GeglNodeContext     *context,
                                                     const gchar         *output_pad,
                                                     const GeglRectangle *result_rect);

/* create a pad for a specified property for this operation, this method is
 * to be called from the attach method of operations, most operations do not
 * have to care about this since a super class like filter, sink, source or
 * composer already does so.
 */
void       gegl_operation_create_pad                (GeglOperation *operation,
                                                     GParamSpec    *param_spec);

/* specify the bablformat for a pad on this operation (XXX: document when
 * this is legal, at the moment, only used internally in some ops,. but might
 * turn into a global mechanism) */
void       gegl_operation_set_format                (GeglOperation *operation,
                                                     const gchar   *pad_name,
                                                     const Babl    *format);


const Babl * gegl_operation_get_format              (GeglOperation *operation,
                                                     const gchar   *pad_name);


G_END_DECLS

#endif /* __GEGL_OPERATION_H__ */
