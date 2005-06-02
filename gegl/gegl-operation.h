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

#ifndef __GEGL_FILTER_H__
#define __GEGL_FILTER_H__

#include "gegl-node.h"

G_BEGIN_DECLS


#define GEGL_TYPE_FILTER            (gegl_filter_get_type ())
#define GEGL_FILTER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_FILTER, GeglFilter))
#define GEGL_FILTER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_FILTER, GeglFilterClass))
#define GEGL_IS_FILTER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_FILTER))
#define GEGL_IS_FILTER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_FILTER))
#define GEGL_FILTER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_FILTER, GeglFilterClass))


typedef struct _GeglFilterClass GeglFilterClass;

struct _GeglFilter
{
  GeglNode  parent_instance;
};

struct _GeglFilterClass
{
  GeglNodeClass parent_class;

  gboolean (* evaluate) (GeglFilter  *self,
                         const gchar *output_prop);
};


GType      gegl_filter_get_type        (void) G_GNUC_CONST;

void       gegl_filter_create_property (GeglFilter  *self,
                                        GParamSpec  *param_spec);
gboolean   gegl_filter_evaluate        (GeglFilter  *self,
                                        const gchar *output_prop);


G_END_DECLS

#endif /* __GEGL_FILTER_H__ */
