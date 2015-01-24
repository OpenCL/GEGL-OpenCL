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
 * Copyright 2014 Jon Nordby <jononor@gmail.com>
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl.h"
#include "gegl-operation-meta-json.h"

static void       finalize     (GObject       *self_object);

G_DEFINE_TYPE (GeglOperationMetaJson, gegl_operation_meta_json, GEGL_TYPE_OPERATION_META)


static void
gegl_operation_meta_json_class_init (GeglOperationMetaJsonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  // TODO: override attach()
  object_class->finalize               = finalize;
}

static void
gegl_operation_meta_json_init (GeglOperationMetaJson *self)
{

}

static void
finalize (GObject *gobject)
{
  G_OBJECT_CLASS (gegl_operation_meta_json_parent_class)->finalize (gobject);
}
