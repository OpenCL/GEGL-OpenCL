/* Originally from: LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, see <http://www.gnu.org/licenses/>.
 *
 * (C) 1999 Austin Donnelly <austin@gimp.org>
 */

#ifndef __GEGL_MODULE_H__
#define __GEGL_MODULE_H__

#include <gmodule.h>
#include "gegl-plugin.h"

G_BEGIN_DECLS

typedef enum
{
  GEGL_MODULE_STATE_ERROR,       /* missing gegl_module_register function
                                  * or other error
                                  */
  GEGL_MODULE_STATE_LOADED,      /* an instance of a type implemented by
                                  * this module is allocated
                                  */
  GEGL_MODULE_STATE_LOAD_FAILED, /* gegl_module_register returned FALSE
                                  */
  GEGL_MODULE_STATE_NOT_LOADED   /* there are no instances allocated of
                                  * types implemented by this module
                                  */
} GeglModuleState;

typedef const GeglModuleInfo * (* GeglModuleQueryFunc)    (GTypeModule *module);
typedef gboolean               (* GeglModuleRegisterFunc) (GTypeModule *module);


#define GEGL_TYPE_MODULE            (gegl_module_get_type ())
#define GEGL_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MODULE, GeglModule))
#define GEGL_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEGL_TYPE_MODULE, GeglModuleClass))
#define GEGL_IS_MODULE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MODULE))
#define GEGL_IS_MODULE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_MODULE))
#define GEGL_MODULE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEGL_TYPE_MODULE, GeglModuleClass))


typedef struct _GeglModuleClass GeglModuleClass;

struct _GeglModule
{
  GTypeModule      parent_instance;

  /*< public >*/
  gchar           *filename;     /* path to the module                       */
  gboolean         verbose;      /* verbose error reporting                  */
  GeglModuleState  state;        /* what's happened to the module            */
  gboolean         on_disk;      /* TRUE if file still exists                */
  gboolean         load_inhibit; /* user requests not to load at boot time   */

  /* stuff from now on may be NULL depending on the state the module is in   */
  /*< private >*/
  GModule         *module;       /* handle on the module                     */

  /*< public >*/
  GeglModuleInfo  *info;         /* returned values from module_query        */
  gchar           *last_module_error;

  /*< private >*/
  GeglModuleQueryFunc     query_module;
  GeglModuleRegisterFunc  register_module;
};

struct _GeglModuleClass
{
  GTypeModuleClass  parent_class;

  void (* modified) (GeglModule *module);

  /* Padding for future expansion */
  void (* _gegl_reserved1) (void);
  void (* _gegl_reserved2) (void);
  void (* _gegl_reserved3) (void);
  void (* _gegl_reserved4) (void);
};


GType         gegl_module_get_type         (void) G_GNUC_CONST;

GeglModule  * gegl_module_new              (const gchar     *filename,
                                            gboolean         load_inhibit,
                                            gboolean         verbose);

gboolean      gegl_module_query_module     (GeglModule      *module);

void          gegl_module_modified         (GeglModule      *module);
void          gegl_module_set_load_inhibit (GeglModule      *module,
                                            gboolean         load_inhibit);

const gchar * gegl_module_state_name       (GeglModuleState  state);


/*  GeglModuleInfo functions  */

GeglModuleInfo * gegl_module_info_new  (guint32               abi_version);
GeglModuleInfo * gegl_module_info_copy (const GeglModuleInfo *info);
void             gegl_module_info_free (GeglModuleInfo       *info);

GType
gegl_module_register_type (GTypeModule     *module,
                           GType            parent_type,
                           const gchar     *type_name,
                           const GTypeInfo *type_info,
                           GTypeFlags       flags);

G_END_DECLS

#endif  /* __GEGL_MODULE_INFO_H__ */
