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

G_BEGIN_DECLS

/* Extra types needed when coding operations */
typedef struct _GeglOperation        GeglOperation;
typedef struct _GeglOperationContext GeglOperationContext;
typedef struct _GeglPad              GeglPad;
typedef struct _GeglConnection       GeglConnection;

#include <gegl-matrix.h>
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

#define GEGL_MODULE_ABI_VERSION 0x000A

struct _GeglModuleInfo
{
  guint32  abi_version;
};

GType gegl_module_register_type (GTypeModule     *module,
                                 GType            parent_type,
                                 const gchar     *type_name,
                                 const GTypeInfo *type_info,
                                 GTypeFlags       flags);

gint            gegl_operation_context_get_level  (GeglOperationContext *self);
GeglBuffer     *gegl_operation_context_get_source (GeglOperationContext *self,
                                                   const gchar          *padname);
GeglBuffer     *gegl_operation_context_get_target (GeglOperationContext *self,
                                                   const gchar          *padname);
void            gegl_operation_context_set_object (GeglOperationContext *context,
                                                   const gchar          *padname,
                                                   GObject              *data);
void            gegl_operation_context_take_object(GeglOperationContext *context,
                                                   const gchar          *padname,
                                                   GObject              *data);
GObject        *gegl_operation_context_get_object (GeglOperationContext *context,
                                                   const gchar          *padname);

void            gegl_extension_handler_register    (const gchar         *extension,
                                                    const gchar         *handler);
void            gegl_extension_handler_register_saver
                                                   (const gchar         *extension,
                                                    const gchar         *handler);
const gchar   * gegl_extension_handler_get         (const gchar         *extension);
const gchar   * gegl_extension_handler_get_saver   (const gchar         *extension);


/* code template utility, updates the jacobian matrix using
 * a user defined mapping function for displacement, example
 * with an identity transform (note that for the identity
 * transform this is massive computational overhead that can
 * be skipped by passing NULL to the sampler.
 *
 * #define gegl_unmap(x,y,dx,dy) { dx=x; dy=y; }
 *
 * gegl_sampler_compute_scale (scale, x, y);
 * gegl_unmap(x,y,sample_x,sample_y);
 * gegl_buffer_sample (buffer, sample_x, sample_y, scale, dest, format,
 *                     GEGL_SAMPLER_LOHALO);
 *
 * #undef gegl_unmap      // IMPORTANT undefine map macro
 */
#define gegl_sampler_compute_scale(matrix, x, y) \
{                                       \
  float ax, ay, bx, by;                 \
  gegl_unmap(x + 0.5, y, ax, ay);       \
  gegl_unmap(x - 0.5, y, bx, by);       \
  matrix.coeff[0][0] = ax - bx;         \
  matrix.coeff[1][0] = ay - by;         \
  gegl_unmap(x, y + 0.5, ax, ay);       \
  gegl_unmap(x, y - 0.5, bx, by);       \
  matrix.coeff[0][1] = ax - bx;         \
  matrix.coeff[1][1] = ay - by;         \
}

typedef struct
{
  GObject       parent_instance;
  void (* get) (GeglSampler *self,
                gdouble      x,
                gdouble      y,
                GeglMatrix2 *scale,
                void        *output);
} SamplerMock;


#define gegl_sampler_get(sampler,x,y,scale,dest) \
  ((SamplerMock*)(sampler))->get((sampler),(x),(y),(scale),(dest))

#include <glib-object.h>
#include <babl/babl.h>
#include <operation/gegl-operation.h>
#include <operation/gegl-operation-filter.h>
#include <operation/gegl-operation-area-filter.h>
#include <operation/gegl-operation-point-filter.h>
#include <operation/gegl-operation-composer.h>
#include <operation/gegl-operation-composer3.h>
#include <operation/gegl-operation-point-composer.h>
#include <operation/gegl-operation-point-composer3.h>
#include <operation/gegl-operation-point-render.h>
#include <operation/gegl-operation-temporal.h>
#include <operation/gegl-operation-source.h>
#include <operation/gegl-operation-sink.h>
#include <operation/gegl-operation-meta.h>

#include <gegl-lookup.h>

G_END_DECLS
#endif  /* __GEGL_PLUGIN_H__ */
