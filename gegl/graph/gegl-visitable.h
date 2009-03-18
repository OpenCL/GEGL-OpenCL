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

#ifndef __GEGL_VISITABLE_H__
#define __GEGL_VISITABLE_H__

G_BEGIN_DECLS


#define GEGL_TYPE_VISITABLE           (gegl_visitable_get_type ())
#define GEGL_VISITABLE(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_VISITABLE, GeglVisitable))
#define GEGL_IS_VISITABLE(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_VISITABLE))
#define GEGL_VISITABLE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEGL_TYPE_VISITABLE, GeglVisitableClass))


typedef struct _GeglVisitableClass GeglVisitableClass;

struct _GeglVisitableClass
{
  GTypeInterface  base_interface;

  void       (* accept)         (GeglVisitable *interface,
                                 GeglVisitor   *visitor);
  GSList   * (* depends_on)     (GeglVisitable *interface);
};


GType      gegl_visitable_get_type       (void) G_GNUC_CONST;

void       gegl_visitable_accept         (GeglVisitable *interface,
                                          GeglVisitor   *visitor);
GSList   * gegl_visitable_depends_on     (GeglVisitable *interface);



G_END_DECLS

#endif /* __GEGL_VISITABLE_H__ */
