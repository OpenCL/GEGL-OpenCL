/* This file is the public operation GEGL API, this API will change to much
 * larger degrees than the api provided by gegl.h
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
 * 2000-2008 Øyvind Kolås.
 */

#ifndef __GEGL_PLUGIN_H__
#define __GEGL_PLUGIN_H__

#include <string.h>
#include <glib-object.h>
#include <gegl.h>
#include <operation/gegl-operation.h>
#include <operation/gegl-extension-handler.h>
#include <gegl-utils.h>
#include <gegl-buffer.h>
#include <gegl-paramspecs.h>
#include <geglmoduletypes.h>
#include <geglmodule.h>

GeglBuffer     *gegl_node_context_get_source (GeglNodeContext *self,
                                              const gchar     *padname);
GeglBuffer     *gegl_node_context_get_target (GeglNodeContext *self,
                                              const gchar     *padname);
void            gegl_node_context_set_object (GeglNodeContext *context,
                                              const gchar     *padname,
                                              GObject         *data);



GParamSpec *
gegl_param_spec_color_from_string (const gchar *name,
                                   const gchar *nick,
                                   const gchar *blurb,
                                   const gchar *default_color_string,
                                   GParamFlags  flags);

/* Probably needs a more API exposed */
GParamSpec * gegl_param_spec_curve     (const gchar *name,
                                        const gchar *nick,
                                        const gchar *blurb,
                                        GeglCurve   *default_curve,
                                        GParamFlags  flags);



#endif  /* __GEGL_PLUGIN_H__ */
