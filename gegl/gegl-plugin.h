/* This file is the public operation GEGL API, this API will change to much
 * larger degrees than the api provided by gegl.h
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
 * 2000-2008 Øyvind Kolås.
 */

#ifndef __GEGL_PLUGIN_H__
#define __GEGL_PLUGIN_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib-object.h>
#include <gegl.h>

/* Extra types needed when coding operations */
typedef struct _GeglOperation        GeglOperation;
typedef struct _GeglNodeContext      GeglNodeContext;
typedef struct _GeglPad              GeglPad;
typedef struct _GeglConnection       GeglConnection;

#include <gegl-utils.h>
#include <gegl-buffer.h>
#include <gegl-paramspecs.h>
#include <gmodule.h>

typedef struct _GeglModule     GeglModule;
typedef struct _GeglModuleInfo GeglModuleInfo;
typedef struct _GeglModuleDB   GeglModuleDB;

/***
 * Writing GEGL operations
 *
 */

/*#include <geglmodule.h>*/

/*  increment the ABI version each time one of the following changes:
 *
 *  - the libgeglmodule implementation (if the change affects modules).
 *  - GeglOperation or one of it's base classes changes. (XXX:-
 *    should be extended so a range of abi versions are accepted.
 */

#define GEGL_MODULE_ABI_VERSION 0x0008

struct _GeglModuleInfo
{
  guint32  abi_version;
};

GType
gegl_module_register_type (GTypeModule     *module,
                           GType            parent_type,
                           const gchar     *type_name,
                           const GTypeInfo *type_info,
                           GTypeFlags       flags);



GeglBuffer     *gegl_node_context_get_source (GeglNodeContext *self,
                                              const gchar     *padname);
GeglBuffer     *gegl_node_context_get_target (GeglNodeContext *self,
                                              const gchar     *padname);
void            gegl_node_context_set_object (GeglNodeContext *context,
                                              const gchar     *padname,
                                              GObject         *data);

void          gegl_extension_handler_register (const gchar *extension,
                                                const gchar *handler);
const gchar * gegl_extension_handler_get      (const gchar *extension);


#if 1

#include <glib-object.h>
#include <babl/babl.h>
#include <operation/gegl-operation.h>
#include <operation/gegl-operation-filter.h>
#include <operation/gegl-operation-area-filter.h>
#include <operation/gegl-operation-point-filter.h>
#include <operation/gegl-operation-composer.h>
#include <operation/gegl-operation-point-composer.h>
#include <operation/gegl-operation-source.h>
#include <operation/gegl-operation-sink.h>
#include <operation/gegl-operation-meta.h>

#ifdef USE_SSE

typedef float v4sf __attribute__ ((vector_size (4*sizeof(float))));
typedef union
{
  v4sf  v;
  float a[4];
} GeglV4;

#endif

#else

/***** ***/


#include <glib-object.h>
#include <babl/babl.h>


/***
 * GeglOperation:
 *
 * All the image processing code in GEGL is implemented as
 * GeglOperations, GEGL oeprations are implemented as GObject with a
 * convenience API called chanting that abstracts away the boiler
 * plater needed to generate introspectable named properties of
 * different types.
 *
 * Most types of operations like: filters, composers, sources, sinks,
 * point operations, compositing operations, and spatial operations
 * with fixed neighbourhoods. These base classes builds on top of the
 * GeglOperationsClass:
 *
 * See <a href='gegl-operation.h.html'>gegl-operation.h</a> for details.
 */

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
  GObjectClass    parent_class;

  const gchar    *name;        /* name(string) used to create/indetify
                                    this type of operation in GEGL*/
  const gchar    *description; /* textual description of the operation */
  const gchar    *categories;  /* a colon seperated list of categories */

  gboolean        no_cache;    /* do not create a cache for this operation */

  /* attach this operation with a GeglNode, override this if you are creating a
   * GeglGraph, it is already defined for Filters/Sources/Composers.
   */
  void          (*attach)                    (GeglOperation *operation);

  /* prepare() is called on each operation providing data to a node that
   * is requested to provide a rendered result. When prepare is called all
   * properties are known. This is the time to set desired pixel formats
   * for input and output pads.
   */
  void          (*prepare)                   (GeglOperation *operation);

  /* Returns the bounding rectangle for the data that is defined by this op.
   * (is already implemented in GeglOperationPointFilter and
   * GeglOperationPointComposer, GeglOperationAreaFilter base classes.
   */
  GeglRectangle (*get_bounding_box)          (GeglOperation *operation);

  /* Computes the region in output (same affected rect assumed for all outputs)
   * when a given region has changed on an input. Used to aggregate dirt in the
   * graph. A default implementation of this, if not provided should probably
   * be to report that the entire defined region is dirtied.
   */
  GeglRectangle (*get_invalidated_by_change) (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *input_region);

  /* Computes the rectangle needed to be correctly computed in a buffer
   * on the named input_pad, for a given region of interest.
   */
  GeglRectangle (*get_required_for_output)   (GeglOperation       *operation,
                                              const gchar         *input_pad,
                                              const GeglRectangle *roi);

  /* Adjust result rect, adapts the rectangle used for computing results.
   * (useful for global operations like contrast stretching, as well as
   * file loaders to force caching of the full raster).
   */
  GeglRectangle (*get_cached_region)         (GeglOperation       *operation,
                                              const GeglRectangle *roi);

  /* Perform processing for the @output_pad, pad The result_rect provides the
   * region to process. For sink operations @output_pad can be ignored but the
   * result_rect is then then indicating the data available for consumption.
   */
  gboolean      (*process)                   (GeglOperation       *operation,
                                              GeglNodeContext     *context,
                                              const gchar         *output_pad,
                                              const GeglRectangle *result_rect);

  /* XXX: What is GeglNode doing in this part of the API?
   * Returns the node providing data for a specific location within the
   * operations output. Does this recurse?, perhaps it should only point out
   * which pad the data is coming from?
   */
  GeglNode*     (*detect)                    (GeglOperation       *operation,
                                              gint                 x,
                                              gint                 y);
};




GType           gegl_operation_get_type        (void) G_GNUC_CONST;

GeglRectangle   gegl_operation_get_invalidated_by_change
                                                 (GeglOperation *operation,
                                                  const gchar   *input_pad,
                                                  const GeglRectangle *input_region);
GeglRectangle   gegl_operation_get_bounding_box  (GeglOperation *operation);

/* retrieves the bounding box of an input pad */
GeglRectangle * gegl_operation_source_get_bounding_box
                                                 (GeglOperation *operation,
                                                  const gchar   *pad_name);


GeglRectangle   gegl_operation_get_cached_region (GeglOperation *operation,
                                                  const GeglRectangle *roi);

GeglRectangle   gegl_operation_get_required_for_output
                                                 (GeglOperation *operation,
                                                  const gchar   *input_pad,
                                                  const GeglRectangle *roi);

GeglNode       *gegl_operation_detect            (GeglOperation *operation,
                                                  gint           x,
                                                  gint           y);


/* virtual method invokers that change behavior based on the roi being computed,
 * needs a context_id being based that is used for storing context data.
 */

void            gegl_operation_attach            (GeglOperation       *operation,
                                                  GeglNode            *node);
void            gegl_operation_prepare           (GeglOperation       *operation);
gboolean        gegl_operation_process           (GeglOperation       *operation,
                                                 GeglNodeContext     *context,
                                                  const gchar         *output_pad,
                                                  const GeglRectangle *result_rect);

/* create a pad for a specified property for this operation, this method is
 * to be called from the attach method of operations, most operations do not
 * have to care about this since a super class like filter, sink, source or
 * composer already does so.
 */
void       gegl_operation_create_pad             (GeglOperation *operation,
                                                  GParamSpec    *param_spec);

/* specify the bablformat for a pad on this operation (XXX: document when
 * this is legal, at the moment, only used internally in some ops,. but might
 * turn into a global mechanism) */
void       gegl_operation_set_format             (GeglOperation *operation,
                                                  const gchar   *pad_name,
                                                  const Babl    *format);


const Babl * gegl_operation_get_format           (GeglOperation *operation,
                                                  const gchar   *pad_name);



/* retrieves the node providing data to a named input pad */
GeglNode      * gegl_operation_get_source_node   (GeglOperation *operation,
                                                  const gchar   *pad_name);


#define GEGL_TYPE_OPERATION_SOURCE            (gegl_operation_source_get_type ())
#define GEGL_OPERATION_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_SOURCE, GeglOperationSource))
#define GEGL_OPERATION_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_SOURCE, GeglOperationSourceClass))
#define GEGL_IS_OPERATION_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_SOURCE, GeglOperationSource))
#define GEGL_IS_OPERATION_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_SOURCE, GeglOperationSourceClass))
#define GEGL_OPERATION_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_SOURCE, GeglOperationSourceClass))

typedef struct _GeglOperationSource  GeglOperationSource;
struct _GeglOperationSource
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationSourceClass GeglOperationSourceClass;
struct _GeglOperationSourceClass
{
  GeglOperationClass parent_class;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *output,
                        const GeglRectangle *result);
};

GType gegl_operation_source_get_type (void) G_GNUC_CONST;



#define GEGL_TYPE_OPERATION_SINK            (gegl_operation_sink_get_type ())
#define GEGL_OPERATION_SINK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_SINK, GeglOperationSink))
#define GEGL_OPERATION_SINK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_SINK, GeglOperationSinkClass))
#define GEGL_IS_OPERATION_SINK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_SINK))
#define GEGL_IS_OPERATION_SINK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_SINK))
#define GEGL_OPERATION_SINK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_SINK, GeglOperationSinkClass))

typedef struct _GeglOperationSink  GeglOperationSink;
struct _GeglOperationSink
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationSinkClass GeglOperationSinkClass;
struct _GeglOperationSinkClass
{
  GeglOperationClass parent_class;

  gboolean           needs_full;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *input,
                        const GeglRectangle *result);
};

GType    gegl_operation_sink_get_type   (void) G_GNUC_CONST;

gboolean gegl_operation_sink_needs_full (GeglOperation *operation);



#define GEGL_TYPE_OPERATION_FILTER            (gegl_operation_filter_get_type ())
#define GEGL_OPERATION_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_FILTER, GeglOperationFilter))
#define GEGL_OPERATION_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_FILTER, GeglOperationFilterClass))
#define GEGL_IS_OPERATION_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_FILTER))
#define GEGL_IS_OPERATION_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_FILTER))
#define GEGL_OPERATION_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_FILTER, GeglOperationFilterClass))

typedef struct _GeglOperationFilter  GeglOperationFilter;
struct _GeglOperationFilter
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationFilterClass GeglOperationFilterClass;
struct _GeglOperationFilterClass
{
  GeglOperationClass parent_class;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *input,
                        GeglBuffer          *output,
                        const GeglRectangle *result);
};

GType gegl_operation_filter_get_type (void) G_GNUC_CONST;


#define GEGL_TYPE_OPERATION_COMPOSER            (gegl_operation_composer_get_type ())
#define GEGL_OPERATION_COMPOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposer))
#define GEGL_OPERATION_COMPOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposerClass))
#define GEGL_IS_OPERATION_COMPOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_COMPOSER))
#define GEGL_IS_OPERATION_COMPOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_COMPOSER))
#define GEGL_OPERATION_COMPOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_COMPOSER, GeglOperationComposerClass))

typedef struct _GeglOperationComposer  GeglOperationComposer;
struct _GeglOperationComposer
{
  GeglOperation parent_instance;
};

typedef struct _GeglOperationComposerClass GeglOperationComposerClass;
struct _GeglOperationComposerClass
{
  GeglOperationClass parent_class;

  gboolean (* process) (GeglOperation       *self,
                        GeglBuffer          *input,
                        GeglBuffer          *aux,
                        GeglBuffer          *output,
                        const GeglRectangle *result);
};

GType gegl_operation_composer_get_type (void) G_GNUC_CONST;




#define GEGL_TYPE_OPERATION_POINT_COMPOSER            (gegl_operation_point_composer_get_type ())
#define GEGL_OPERATION_POINT_COMPOSER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposer))
#define GEGL_OPERATION_POINT_COMPOSER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))
#define GEGL_IS_OPERATION_POINT_COMPOSER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_POINT_COMPOSER))
#define GEGL_IS_OPERATION_POINT_COMPOSER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_POINT_COMPOSER))
#define GEGL_OPERATION_POINT_COMPOSER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_COMPOSER, GeglOperationPointComposerClass))

typedef struct _GeglOperationPointComposer  GeglOperationPointComposer;
struct _GeglOperationPointComposer
{
  GeglOperationComposer parent_instance;

  /*< private >*/
};

typedef struct _GeglOperationPointComposerClass GeglOperationPointComposerClass;
struct _GeglOperationPointComposerClass
{
  GeglOperationComposerClass parent_class;

  gboolean (* process) (GeglOperation *self,      /* for parameters      */
                        void          *in,
                        void          *aux,
                        void          *out,
                        glong          samples);  /* number of samples   */

};

GType gegl_operation_point_composer_get_type (void) G_GNUC_CONST;




#define GEGL_TYPE_OPERATION_POINT_FILTER            (gegl_operation_point_filter_get_type ())
#define GEGL_OPERATION_POINT_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_POINT_FILTER, GeglOperationPointFilter))
#define GEGL_OPERATION_POINT_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_POINT_FILTER, GeglOperationPointFilterClass))
#define GEGL_IS_OPERATION_POINT_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_POINT_FILTER))
#define GEGL_IS_OPERATION_POINT_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_POINT_FILTER))
#define GEGL_OPERATION_POINT_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_POINT_FILTER, GeglOperationPointFilterClass))

typedef struct _GeglOperationPointFilter  GeglOperationPointFilter;
struct _GeglOperationPointFilter
{
  GeglOperationFilter parent_instance;
};

typedef struct _GeglOperationPointFilterClass GeglOperationPointFilterClass;
struct _GeglOperationPointFilterClass
{
  GeglOperationFilterClass parent_class;

  gboolean (* process) (GeglOperation *self,      /* for parameters      */
                        void          *in_buf,    /* input buffer */
                        void          *out_buf,   /* output buffer */
                        glong          samples);  /* number of samples   */
};

GType gegl_operation_point_filter_get_type (void) G_GNUC_CONST;



#define GEGL_TYPE_OPERATION_AREA_FILTER            (gegl_operation_area_filter_get_type ())
#define GEGL_OPERATION_AREA_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilter))
#define GEGL_OPERATION_AREA_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilterClass))
#define GEGL_IS_OPERATION_AREA_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_AREA_FILTER))
#define GEGL_IS_OPERATION_AREA_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_AREA_FILTER))
#define GEGL_OPERATION_AREA_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_AREA_FILTER, GeglOperationAreaFilterClass))

typedef struct _GeglOperationAreaFilter  GeglOperationAreaFilter;
struct _GeglOperationAreaFilter
{
  GeglOperationFilter parent_instance;

  gint                left;
  gint                right;
  gint                top;
  gint                bottom;
};

typedef struct _GeglOperationAreaFilterClass GeglOperationAreaFilterClass;
struct _GeglOperationAreaFilterClass
{
  GeglOperationFilterClass parent_class;
};

GType gegl_operation_area_filter_get_type (void) G_GNUC_CONST;


#define GEGL_TYPE_OPERATION_META            (gegl_operation_meta_get_type ())
#define GEGL_OPERATION_META(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_OPERATION_META, GeglOperationMeta))
#define GEGL_OPERATION_META_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_OPERATION_META, GeglOperationMetaClass))
#define GEGL_IS_OPERATION_META(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_OPERATION_META))
#define GEGL_IS_OPERATION_META_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_OPERATION_META))
#define GEGL_OPERATION_META_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_OPERATION_META, GeglOperationMetaClass))

typedef struct _GeglOperationMeta  GeglOperationMeta;
struct _GeglOperationMeta
{
  GeglOperation parent_instance;
  GSList       *redirects;
};

typedef struct _GeglOperationMetaClass GeglOperationMetaClass;
struct _GeglOperationMetaClass
{
  GeglOperationClass parent_class;
};


GType gegl_operation_meta_get_type         (void) G_GNUC_CONST;

void  gegl_operation_meta_redirect         (GeglOperation     *operation,
                                            const gchar       *name,
                                            GeglNode          *internal,
                                            const gchar       *internal_name);

void  gegl_operation_meta_property_changed (GeglOperationMeta *self,
                                            GParamSpec        *arg1,
                                            gpointer           user_data);



#endif

#endif  /* __GEGL_PLUGIN_H__ */
