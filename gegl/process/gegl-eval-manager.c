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
 *           2006 Øyvind Kolås
 *           2013 Daniel Sabo
 */

#include "config.h"

#include <glib-object.h>

#include "gegl.h"
#include "gegl-types-internal.h"
#include "gegl-eval-manager.h"
#include "gegl-instrument.h"

#include "graph/gegl-node-private.h"

#include "process/gegl-graph-traversal.h"

static void gegl_eval_manager_class_init (GeglEvalManagerClass *klass);
static void gegl_eval_manager_init (GeglEvalManager *self);
static void gegl_eval_manager_finalize (GObject *self_object);

G_DEFINE_TYPE (GeglEvalManager, gegl_eval_manager, G_TYPE_OBJECT)

static void
gegl_eval_manager_class_init (GeglEvalManagerClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gegl_eval_manager_finalize;
}

static void
gegl_eval_manager_init (GeglEvalManager *self)
{
  self->state     = INVALID;
  self->pad_name  = NULL;
  self->traversal = NULL;
}

static void
gegl_eval_manager_finalize (GObject *self_object)
{
  GeglEvalManager *self = GEGL_EVAL_MANAGER (self_object);

  if (self->pad_name)
    {
      g_free (self->pad_name);
      self->pad_name = NULL;
    }

  if (self->traversal)
    {
      gegl_graph_free (self->traversal);
      self->traversal = NULL;
    }

  g_signal_handlers_disconnect_by_data (self->node, self);

  G_OBJECT_CLASS (gegl_eval_manager_parent_class)->finalize (self_object);
}

static gboolean
gegl_eval_manager_change_notification (GObject             *gobject,
                                       const GeglRectangle *rect,
                                       gpointer             user_data)
{
  GeglEvalManager *manager = GEGL_EVAL_MANAGER (user_data);
  manager->state = INVALID;

  return FALSE;
}

void
gegl_eval_manager_prepare (GeglEvalManager *self)
{
  g_return_if_fail (GEGL_IS_EVAL_MANAGER (self));
  g_return_if_fail (GEGL_IS_NODE (self->node));

  if (self->state != READY)
    {
      if (!self->traversal)
        self->traversal = gegl_graph_build (self->node);
      else
        gegl_graph_rebuild (self->traversal, self->node);

      gegl_graph_prepare (self->traversal);

      self->state = READY;
    }
}

GeglRectangle
gegl_eval_manager_get_bounding_box (GeglEvalManager     *self)
{
  gegl_eval_manager_prepare (self);
  return gegl_graph_get_bounding_box (self->traversal);
}

GeglBuffer *
gegl_eval_manager_apply (GeglEvalManager     *self,
                         const GeglRectangle *roi,
                         gint                 level)
{
  GeglBuffer  *object;

  g_return_val_if_fail (GEGL_IS_EVAL_MANAGER (self), NULL);
  g_return_val_if_fail (GEGL_IS_NODE (self->node), NULL);

  if (level >= GEGL_CACHE_VALID_MIPMAPS)
    level = GEGL_CACHE_VALID_MIPMAPS-1;

  GEGL_INSTRUMENT_START();
  gegl_eval_manager_prepare (self);
  GEGL_INSTRUMENT_END ("gegl", "prepare-graph");

  GEGL_INSTRUMENT_START();
  gegl_graph_prepare_request (self->traversal, roi, level);
  GEGL_INSTRUMENT_END ("gegl", "prepare-request");

  GEGL_INSTRUMENT_START();
  object = gegl_graph_process (self->traversal, level);
  GEGL_INSTRUMENT_END ("gegl", "process");

  return object;
}

GeglEvalManager * gegl_eval_manager_new     (GeglNode    *node,
                                             const gchar *pad_name)
{
  GeglEvalManager *self = g_object_new (GEGL_TYPE_EVAL_MANAGER, NULL);

  g_assert (GEGL_IS_NODE (node));

  /* FIXME: This should be a weakref */
  self->node = node;

  if (pad_name)
    self->pad_name = g_strdup (pad_name);
  else
    self->pad_name = g_strdup ("output");

  g_signal_connect (G_OBJECT (self->node), "invalidated",
                    G_CALLBACK (gegl_eval_manager_change_notification),
                    self);
  return self;
}
