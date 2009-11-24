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
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>

#include "gegl-types-internal.h"

#include "gegl-visitable.h"


GType
gegl_visitable_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      static const GTypeInfo type_info =
      {
        sizeof (GeglVisitableClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) NULL,
        (GClassFinalizeFunc) NULL,
        NULL,
        0,
        0,
        (GInstanceInitFunc) NULL,
        NULL,
      };

      type = g_type_register_static (G_TYPE_INTERFACE, "GeglVisitable",
                                     &type_info, 0);

      g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
    }

  return type;
}

void
gegl_visitable_accept (GeglVisitable *interface,
                       GeglVisitor   *visitor)
{
  GeglVisitableClass *interface_class;

  g_return_if_fail (GEGL_IS_VISITABLE (interface));

  interface_class = GEGL_VISITABLE_GET_CLASS (interface);

  interface_class->accept (interface, visitor);
}

GSList *
gegl_visitable_depends_on (GeglVisitable *interface)
{
  GeglVisitableClass *interface_class;
  GSList             *depends_on = NULL;

  interface_class = GEGL_VISITABLE_GET_CLASS (interface);

  depends_on = interface_class->depends_on (interface);

  return depends_on;
}
