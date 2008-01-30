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

/* Extra types needed when coding operations */
typedef struct _GeglOperation        GeglOperation;
typedef struct _GeglNodeContext      GeglNodeContext;
typedef struct _GeglPad              GeglPad;
typedef struct _GeglConnection       GeglConnection;

#include <operation/gegl-operation.h>
#include <gegl-utils.h>
#include <gegl-buffer.h>
#include <gegl-paramspecs.h>
#include <gmodule.h>

typedef struct _GeglModule     GeglModule;
typedef struct _GeglModuleInfo GeglModuleInfo;
typedef struct _GeglModuleDB   GeglModuleDB;

/***
 * Writing GEGL operations
 *
 */

/*#include <geglmodule.h>*/

/*  increment the ABI version each time one of the following changes:
 *
 *  - the libgeglmodule implementation (if the change affects modules).
 *  - GeglOperation or one of it's base classes changes. (XXX:-
 *    should be extended so a range of abi versions are accepted.
 */

#define GEGL_MODULE_ABI_VERSION 0x0005

struct _GeglModuleInfo
{
  guint32  abi_version;
  gchar   *purpose;
  gchar   *author;
  gchar   *version;
  gchar   *copyright;
  gchar   *date;
};

GType
gegl_module_register_type (GTypeModule     *module,
                           GType            parent_type,
                           const gchar     *type_name,
                           const GTypeInfo *type_info,
                           GTypeFlags       flags);



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


void          gegl_extension_handler_register (const gchar *extension,
                                                const gchar *handler);
const gchar * gegl_extension_handler_get      (const gchar *extension);


#endif  /* __GEGL_PLUGIN_H__ */
