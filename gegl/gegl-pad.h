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

#ifndef __GEGL_PROPERTY_H__
#define __GEGL_PROPERTY_H__

#include "gegl-object.h"

G_BEGIN_DECLS


typedef enum
{
  GEGL_PROPERTY_OUTPUT  = 1 << G_PARAM_USER_SHIFT,
  GEGL_PROPERTY_INPUT   = 1 << (G_PARAM_USER_SHIFT + 1)
} GeglPropertyType;


#define GEGL_TYPE_PROPERTY            (gegl_property_get_type ())
#define GEGL_PROPERTY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PROPERTY, GeglProperty))
#define GEGL_PROPERTY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PROPERTY, GeglPropertyClass))
#define GEGL_IS_PROPERTY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PROPERTY))
#define GEGL_IS_PROPERTY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PROPERTY))
#define GEGL_PROPERTY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PROPERTY, GeglPropertyClass))


typedef struct _GeglPropertyClass GeglPropertyClass;

struct _GeglProperty
{
  GeglObject  parent_instance;

  /*< private >*/
  GParamSpec *param_spec;
  GeglFilter *filter;
  GList      *connections;
  gboolean    dirty;
};

struct _GeglPropertyClass
{
  GeglObjectClass  parent_class;
};


GType            gegl_property_get_type             (void) G_GNUC_CONST;

const gchar    * gegl_property_get_name            (GeglProperty   *self);
GList          * gegl_property_get_depends_on      (GeglProperty   *self);
GeglFilter     * gegl_property_get_filter          (GeglProperty   *self);
void             gegl_property_set_filter          (GeglProperty   *self,
                                                    GeglFilter     *filter);
gboolean         gegl_property_is_output           (GeglProperty   *self);
gboolean         gegl_property_is_input            (GeglProperty   *self);
GeglProperty   * gegl_property_get_connected_to    (GeglProperty   *self);
gboolean         gegl_property_is_dirty            (GeglProperty   *self);
void             gegl_property_set_dirty           (GeglProperty   *self,
                                                    gboolean        flag);
GeglConnection * gegl_property_connect             (GeglProperty   *sink,
                                                    GeglProperty   *source);
void             gegl_property_disconnect          (GeglProperty   *sink,
                                                    GeglProperty   *source,
                                                    GeglConnection *connection);
GList          * gegl_property_get_connections     (GeglProperty   *self);
gint             gegl_property_get_num_connections (GeglProperty   *self);
GParamSpec     * gegl_property_get_param_spec      (GeglProperty   *self);
void             gegl_property_set_param_spec      (GeglProperty   *self,
                                                    GParamSpec     *param_spec);


G_END_DECLS

#endif /* __GEGL_PROPERTY_H__ */
