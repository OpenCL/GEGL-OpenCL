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
 */

#include "config.h"

#include <glib-object.h>
#include <string.h>

#include "gegl-types.h"
#include "gegl-interpolator.h"
#include "gegl-utils.h"

static void      gegl_interpolator_class_init (GeglInterpolatorClass    *klass);
static void      gegl_interpolator_init       (GeglInterpolator         *self);
static void      finalize                  (GObject       *gobject);

G_DEFINE_TYPE (GeglInterpolator, gegl_interpolator, G_TYPE_OBJECT)

static void
gegl_interpolator_class_init (GeglInterpolatorClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);  
  object_class->finalize = finalize;

  klass->prepare = NULL;
  klass->get = NULL;
}

static void
gegl_interpolator_init (GeglInterpolator *self)
{
}

void
gegl_interpolator_get (GeglInterpolator *self,
                       gdouble           x,
                       gdouble           y,
                       void             *output)
{
  GeglInterpolatorClass *klass;

  g_return_if_fail (GEGL_IS_INTERPOLATOR (self));

  klass = GEGL_INTERPOLATOR_GET_CLASS (self);

  klass->get (self, x, y, output);
}

void
gegl_interpolator_prepare (GeglInterpolator *self)
{
  GeglInterpolatorClass *klass;

  g_return_if_fail (GEGL_IS_INTERPOLATOR (self));

  klass = GEGL_INTERPOLATOR_GET_CLASS (self);

  if (klass->prepare)
    klass->prepare (self);
}

static void
finalize (GObject *gobject)
{  
  G_OBJECT_CLASS (gegl_interpolator_parent_class)->finalize (gobject);
}
