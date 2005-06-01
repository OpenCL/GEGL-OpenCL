/*
 *   This file is part of GEGL.
 *
 *    GEGL is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    GEGL is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with GEGL; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright 2003 Calvin Williamson
 *
 */
#include "gegl-visitable.h"
#include <string.h>
#include <glib-object.h>

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

      type = g_type_register_static (G_TYPE_INTERFACE,
                                    "GeglVisitable",
                                     &type_info,
                                     0);

      g_type_interface_add_prerequisite (type, GEGL_TYPE_OBJECT);
    }

  return type;
}

void
gegl_visitable_accept (GeglVisitable   *interface,
                       GeglVisitor * visitor)
{
  GeglVisitableClass *interface_class;
  g_return_if_fail (GEGL_IS_VISITABLE (interface));
  interface_class = GEGL_VISITABLE_GET_CLASS (interface);
  g_object_ref (interface);
  interface_class->accept (interface, visitor);
  g_object_unref (interface);
}

GList *
gegl_visitable_depends_on (GeglVisitable   *interface)
{
  GeglVisitableClass *interface_class;
  GList *depends_on = NULL;

  g_return_val_if_fail (GEGL_IS_VISITABLE (interface), NULL);
  interface_class = GEGL_VISITABLE_GET_CLASS (interface);
  g_object_ref (interface);
  depends_on = interface_class->depends_on (interface);
  g_object_unref (interface);
  return depends_on;
}

gboolean
gegl_visitable_needs_visiting (GeglVisitable   *interface)
{
  GeglVisitableClass *interface_class;
  gboolean needs_visiting;

  g_return_val_if_fail (GEGL_IS_VISITABLE (interface), FALSE);
  interface_class = GEGL_VISITABLE_GET_CLASS (interface);
  g_object_ref (interface);
  needs_visiting = interface_class->needs_visiting(interface);
  g_object_unref (interface);
  return needs_visiting;
}
