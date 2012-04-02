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


/***
 * GeglOperation:
 *
 * All the image processing code in GEGL is implemented as GeglOperations,
 * GEGL operations are implemented as GObject with a convenience API called
 * chanting that abstracts away the boiler plater needed to generate introspectable
 * named properties of different types.
 *
 * Most types of operations like: filters, composers, sources, sinks, point
 * operations, compositing operations, and spatial operations with fixed
 * neighbourhoods. These base classes build on top of the GeglOperationsClass:
 *
 * See <a href='gegl-plugin.h.html'>gegl-plugin.h</a> for details.
 */

/* the level at which is being operated is stored in the context,
*/

struct _GeglOperationClass
{
  GObjectClass    parent_class;

  const gchar    *name;        /* name(string) used to create/identify
                                  this type of operation in GEGL, should be
                                  set through gegl_operation_class_set_key(s) */
  const gchar    *compat_name; /* allows specifying an alias that the op is
                                  also known as */
  GHashTable     *keys;        /* hashtable used for storing meta-data about an op */

  guint           no_cache      :1;  /* do not create a cache for this operation */
  guint           opencl_support:1;
  guint64         bit_pad:62;

  /* attach this operation with a GeglNode, override this if you are creating a
   * GeglGraph, it is already defined for Filters/Sources/Composers.
   */
  void          (*attach)                    (GeglOperation *operation);

  /* Initialize the operation, prepare is called when all properties are
   * known but before processing will begin. Prepare will be invoked one
   * or multiple times prior to processing.
   */
  void          (*prepare)                   (GeglOperation *operation);

  /* The bounding rectangle for the data that is defined by this op.
   */
  GeglRectangle (*get_bounding_box)          (GeglOperation *operation);

  /* The output region that is made invalid by a change in the input_roi
   * rectangle of the buffer passed in on the pad input_pad. Defaults to
   * returning the input_roi.
   */
  GeglRectangle (*get_invalidated_by_change) (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *input_roi);

  /* The rectangle needed to be correctly computed in a buffer on the named
   * input_pad, for a given region of interest. Defaults to return the
   * output_roi.
   */
  GeglRectangle (*get_required_for_output)   (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *output_roi);

  /* The rectangular area that should be processed in one go, by default if not
   * defined the output roi would be returned. This is useful for file loaders
   * and operations like contrast stretching which is a point operation but we
   * need the parameters as the minimum/maximum values in the entire input buffer.
   */
  GeglRectangle (*get_cached_region)         (GeglOperation       *operation,
                                              const GeglRectangle *output_roi);

  /* Compute the rectangular region output roi for the specified output_pad.
   * For operations that are sinks (have no output pads), roi is the rectangle
   * to consume and the output_pad argument is to be ignored.
   */
  gboolean      (*process)                   (GeglOperation        *operation,
                                              GeglOperationContext *context,
                                              const gchar          *output_pad,
                                              const GeglRectangle  *roi,
                                              gint                  level);

  /* The node providing data for a specific location within the operations
   * output. The node is responsible for delegating blame to one of it's
   * inputs taking into account opacity and similar issues.
   *
   * XXX: What is GeglNode doing in this part of the API?,
   * perhaps we should only point out which pad the data is coming from?
   */
  GeglNode*     (*detect)                    (GeglOperation       *operation,
                                              gint                 x,
                                              gint                 y);
  gpointer      pad[10];
};



GType           gegl_operation_get_type        (void) G_GNUC_CONST;

GeglRectangle   gegl_operation_get_invalidated_by_change
                                             (GeglOperation *operation,
                                              const gchar   *input_pad,
                                              const GeglRectangle *roi);
GeglRectangle   gegl_operation_get_bounding_box  (GeglOperation *operation);

/* retrieves the bounding box of an input pad */
GeglRectangle * gegl_operation_source_get_bounding_box
                                             (GeglOperation *operation,
                                              const gchar   *pad_name);


GeglRectangle   gegl_operation_get_cached_region
                                             (GeglOperation *operation,
                                              const GeglRectangle *roi);

GeglRectangle   gegl_operation_get_required_for_output
                                             (GeglOperation *operation,
                                              const gchar   *input_pad,
                                              const GeglRectangle *roi);

GeglNode       *gegl_operation_detect        (GeglOperation *operation,
                                              gint           x,
                                              gint           y);


/* virtual method invokers that change behavior based on the roi being computed,
 * needs a context_id being based that is used for storing context data.
 */

void            gegl_operation_attach        (GeglOperation *operation,
                                              GeglNode      *node);
void            gegl_operation_prepare       (GeglOperation *operation);
gboolean        gegl_operation_process       (GeglOperation *operation,
                                              GeglOperationContext *context,
                                              const gchar          *output_pad,
                                              const GeglRectangle  *roi,
                                              gint                  level);

/* create a pad for a specified property for this operation, this method is
 * to be called from the attach method of operations, most operations do not
 * have to care about this since a super class like filter, sink, source or
 * composer already does so.
 */
void            gegl_operation_create_pad    (GeglOperation *operation,
                                              GParamSpec    *param_spec);

/* specify the bablformat for a pad on this operation (XXX: document when
 * this is legal, at the moment, only used internally in some ops,. but might
 * turn into a global mechanism) */
void            gegl_operation_set_format    (GeglOperation *operation,
                                              const gchar   *pad_name,
                                              const Babl    *format);


const Babl *    gegl_operation_get_format    (GeglOperation *operation,
                                              const gchar   *pad_name);

const gchar *   gegl_operation_get_name      (GeglOperation *operation);


/* retrieves the node providing data to a named input pad */
GeglNode      * gegl_operation_get_source_node (GeglOperation *operation,
                                                const gchar   *pad_name);

GParamSpec ** gegl_operation_list_properties   (const gchar *operation_type,
                                                guint       *n_properties_p);

/* API to change  */
void          gegl_operation_class_set_key     (GeglOperationClass *klass,
                                                const gchar *key_name,
                                                const gchar *key_value);

const gchar * gegl_operation_class_get_key     (GeglOperationClass *operation_class,
                                                const gchar        *key_name);

void          gegl_operation_class_set_keys    (GeglOperationClass *klass,
                                                const gchar        *key_name,
                                                ...);

gchar      ** gegl_operation_list_keys         (const gchar *operation_type,
                                                guint       *n_keys);

void          gegl_operation_set_key           (const gchar *operation_type,
                                                const gchar *key_name,
                                                const gchar *key_value);

const gchar * gegl_operation_get_key            (const gchar *operation_type,
                                                 const gchar *key_name);

/* invalidate a specific rectangle, indicating the any computation depending
 * on this roi is now invalid.
 *
 * @roi : the region to blank or NULL for the nodes current have_rect
 * @clear_cache: whether any present caches should be zeroed out
 */
void     gegl_operation_invalidate       (GeglOperation       *operation,
                                          const GeglRectangle *roi,
                                          gboolean             clear_cache);

/* internal utility functions used by gegl, these should not be used
 * externally */
gboolean gegl_operation_calc_need_rects      (GeglOperation       *operation,
                                              gpointer             context_id);
void     gegl_operation_path_prop_changed    (GeglPath            *path,
                                              GeglOperation       *operation);

G_END_DECLS

/***
 */

#endif /* __GEGL_OPERATION_H__ */
