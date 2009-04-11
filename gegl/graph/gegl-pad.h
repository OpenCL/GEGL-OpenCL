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

#ifndef __GEGL_PAD_H__
#define __GEGL_PAD_H__

#include <glib-object.h>

G_BEGIN_DECLS



#define GEGL_TYPE_PAD            (gegl_pad_get_type ())
#define GEGL_PAD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PAD, GeglPad))
#define GEGL_PAD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PAD, GeglPadClass))
#define GEGL_IS_PAD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PAD))
#define GEGL_IS_PAD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PAD))
#define GEGL_PAD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PAD, GeglPadClass))


typedef struct _GeglPadClass GeglPadClass;

struct _GeglPad
{
  GObject        parent_instance;

  /*< private >*/
  GParamSpec    *param_spec;
  GeglNode      *node;
  GSList        *connections;
  const Babl    *format;      /* pixel format this pad produces/consumes,
                                 (initially only what it produces, and used
                                  for gegl_operation_get_target.)
                               */
  gchar         *name;
};

struct _GeglPadClass
{
  GObjectClass   parent_class;
};


GType            gegl_pad_get_type                  (void) G_GNUC_CONST;

const gchar    * gegl_pad_get_name                  (GeglPad        *self);
void             gegl_pad_set_name                  (GeglPad        *self,
                                                     const gchar    *name);

GSList         * gegl_pad_get_depends_on            (GeglPad        *self);
GeglNode       * gegl_pad_get_node                  (GeglPad        *self);
void             gegl_pad_set_node                  (GeglPad        *self,
                                                     GeglNode       *node);
gboolean         gegl_pad_is_output                 (GeglPad        *self);
gboolean         gegl_pad_is_input                  (GeglPad        *self);
GeglPad        * gegl_pad_get_connected_to          (GeglPad        *self);
GeglConnection * gegl_pad_connect                   (GeglPad        *sink,
                                                     GeglPad        *source);
void             gegl_pad_disconnect                (GeglPad        *sink,
                                                     GeglPad        *source,
                                                     GeglConnection *connection);
GSList         * gegl_pad_get_connections           (GeglPad        *self);
gint             gegl_pad_get_num_connections       (GeglPad        *self);
GParamSpec     * gegl_pad_get_param_spec            (GeglPad        *self);
void             gegl_pad_set_param_spec            (GeglPad        *self,
                                                     GParamSpec     *param_spec);

void             gegl_pad_set_format                (GeglPad        *self,
                                                     const Babl     *format);
const Babl     * gegl_pad_get_format                (GeglPad        *self);

G_END_DECLS

#endif /* __GEGL_PAD_H__ */
