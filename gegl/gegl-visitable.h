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
#ifndef __GEGL_VISITABLE_H__
#define __GEGL_VISITABLE_H__

#include <glib-object.h>
#include <glib.h>
#include "gegl-visitor.h"

#define GEGL_TYPE_VISITABLE   (gegl_visitable_get_type ())
#define GEGL_VISITABLE(obj)   (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITABLE, GeglVisitable))
#define GEGL_IS_VISITABLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITABLE))
#define GEGL_VISITABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEGL_TYPE_VISITABLE, GeglVisitableClass))

typedef struct _GeglVisitable      GeglVisitable;
typedef struct _GeglVisitableClass GeglVisitableClass;
struct _GeglVisitableClass
{
  GTypeInterface base_interface;

  void  (*accept) (GeglVisitable *interface, GeglVisitor *visitor);
  GList *(*depends_on) (GeglVisitable *interface);
  gboolean (*needs_visiting) (GeglVisitable *interface);
};

GType gegl_visitable_get_type (void) G_GNUC_CONST;

void gegl_visitable_accept (GeglVisitable   *interface, GeglVisitor *visitor);
GList *gegl_visitable_depends_on(GeglVisitable *interface);
gboolean gegl_visitable_needs_visiting(GeglVisitable *interface);

#endif
