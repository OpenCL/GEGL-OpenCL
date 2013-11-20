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
 *           2006-2008 Øyvind Kolås
 *           2013 Daniel Sabo
 */

#ifndef __GEGL_OPERATION_CONTEXT_PRIVATE_H__
#define __GEGL_OPERATION_CONTEXT_PRIVATE_H__

G_BEGIN_DECLS


/**
 * When a node in a GEGL graph does processing, it needs context such
 * as inputs. This structure holds this stuff and is passed to the
 * node during processing. The data in a context is bound to one
 * specific rendering/processing job/thread.
 */
struct _GeglOperationContext
{
  GeglOperation *operation;

  GSList        *property;    /* used internally for data being exchanged */
  GeglRectangle  need_rect;   /* the rectangle needed from the operation */
  GeglRectangle  result_rect; /* the result computation rectangle for the operation ,
                                 (will differ if the needed rect extends beyond
                                 the defined rectangle, some operations might
                                 force/suggest expansion of the result
                                 rect, like contrast stretching.
                               */


  gboolean       cached;       /* true if the cache can be used directly, and
                                  recomputation of inputs is unneccesary) */

  gint           refs;         /* set to number of nodes that depends on it
                                  before evaluation begins, each time data is
                                  fetched from the op the reference count is
                                  dropped, when it drops to zero, the op is
                                  asked to clean it's pads, FIXME: should be
                                  incorporated into the refcount of
                                  GeglOperationContext?
                                */
  gint           level;         /* subdivision level to render at, 0 = 1:1,
                                                                   1 = 1:2,
                                                                   2 = 1:4,
                                                                   4 = 1:8,
                                                                   6 = 1:16 .. */
};

GeglOperationContext *gegl_operation_context_new       (GeglOperation        *operation);
void                  gegl_operation_context_destroy   (GeglOperationContext *self);

void            gegl_operation_context_set_property    (GeglOperationContext *self,
                                                        const gchar          *name,
                                                        const GValue         *value) G_GNUC_DEPRECATED;
void            gegl_operation_context_get_property    (GeglOperationContext *self,
                                                        const gchar          *name,
                                                        GValue               *value) G_GNUC_DEPRECATED;
void            gegl_operation_context_remove_property (GeglOperationContext *self,
                                                        const gchar          *name) G_GNUC_DEPRECATED;

GValue         *gegl_operation_context_get_value       (GeglOperationContext *self,
                                                        const gchar          *name);

GeglRectangle  *gegl_operation_context_get_need_rect   (GeglOperationContext *self);
void            gegl_operation_context_set_need_rect   (GeglOperationContext *self,
                                                        const GeglRectangle  *rect);
GeglRectangle  *gegl_operation_context_get_result_rect (GeglOperationContext *node);
void            gegl_operation_context_set_result_rect (GeglOperationContext *node,
                                                        const GeglRectangle  *rect);

G_END_DECLS

#endif /* __GEGL_OPERATION_CONTEXT_PRIVATE_H__ */
