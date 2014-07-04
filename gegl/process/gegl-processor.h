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
 * Copyright 2007 Øyvind Kolås
 */

#ifndef __GEGL_PROCESSOR_H__
#define __GEGL_PROCESSOR_H__

G_BEGIN_DECLS

/***
 * GeglProcessor:
 *
 * A #GeglProcessor, is a worker that can be used for background rendering
 * of regions in a node's cache. Or for processing a sink node. For most
 * non GUI tasks using #gegl_node_blit and #gegl_node_process directly
 * should be sufficient. See #gegl_processor_work for a code sample.
 *
 */

/**
 * gegl_node_new_processor:
 * @node: a #GeglNode
 * @rectangle: the #GeglRectangle to work on or NULL to work on all available
 * data.
 *
 * Return value: (transfer full): a new #GeglProcessor.
 */
GeglProcessor *gegl_node_new_processor      (GeglNode            *node,
                                             const GeglRectangle *rectangle);

void gegl_processor_set_level (GeglProcessor *processor,
                               gint           level);
void gegl_processor_set_scale (GeglProcessor *processor,
                               gdouble        scale);

/**
 * gegl_processor_set_rectangle:
 * @processor: a #GeglProcessor
 * @rectangle: the new #GeglRectangle the processor shold work on or NULL
 * to make it work on all data in the buffer.
 *
 * Change the rectangle a #GeglProcessor is working on.
 */
void           gegl_processor_set_rectangle (GeglProcessor       *processor,
                                             const GeglRectangle *rectangle);


/**
 * gegl_processor_work:
 * @processor: a #GeglProcessor
 * @progress: (out caller-allocates): a location to store the (estimated) percentage complete.
 *
 * Do an iteration of work for the processor.
 *
 * Returns TRUE if there is more work to be done.
 *
 * ---
 * GeglProcessor *processor = gegl_node_new_processor (node, &roi);
 * double         progress;
 *
 * while (gegl_processor_work (processor, &progress))
 *   g_warning ("%f%% complete", progress);
 * g_object_unref (processor);
 */
gboolean       gegl_processor_work          (GeglProcessor *processor,
                                             gdouble       *progress);

G_END_DECLS

#endif /* __GEGL_PROCESSOR_H__ */
