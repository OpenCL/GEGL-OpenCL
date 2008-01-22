/* This file is part of GEGL
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
 */

#ifndef __GEGL_MODULE_DB_H__
#define __GEGL_MODULE_DB_H__

G_BEGIN_DECLS

#include "gegl-plugin.h"


#define GEGL_TYPE_MODULE_DB            (gegl_module_db_get_type ())
#define GEGL_MODULE_DB(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_MODULE_DB, GeglModuleDB))
#define GEGL_MODULE_DB_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GEGL_TYPE_MODULE_DB, GeglModuleDBClass))
#define GEGL_IS_MODULE_DB(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_MODULE_DB))
#define GEGL_IS_MODULE_DB_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEGL_TYPE_MODULE_DB))
#define GEGL_MODULE_DB_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GEGL_TYPE_MODULE_DB, GeglModuleDBClass))

typedef struct _GeglModuleDBClass GeglModuleDBClass;

struct _GeglModuleDB
{
  GObject   parent_instance;

  /*< private >*/
  GList    *modules;

  gchar    *load_inhibit;
  gboolean  verbose;
};

struct _GeglModuleDBClass
{
  GObjectClass  parent_class;

  void (* add)             (GeglModuleDB *db,
                            GeglModule   *module);
  void (* remove)          (GeglModuleDB *db,
                            GeglModule   *module);
  void (* module_modified) (GeglModuleDB *db,
                            GeglModule   *module);

  /* Padding for future expansion */
  void (* _gegl_reserved1) (void);
  void (* _gegl_reserved2) (void);
  void (* _gegl_reserved3) (void);
  void (* _gegl_reserved4) (void);
};


GType          gegl_module_db_get_type         (void) G_GNUC_CONST;
GeglModuleDB * gegl_module_db_new              (gboolean      verbose);

void           gegl_module_db_set_load_inhibit (GeglModuleDB *db,
                                                const gchar  *load_inhibit);
const gchar  * gegl_module_db_get_load_inhibit (GeglModuleDB *db);

void           gegl_module_db_load             (GeglModuleDB *db,
                                                const gchar  *module_path);
void           gegl_module_db_refresh          (GeglModuleDB *db,
                                                const gchar  *module_path);


G_END_DECLS

#endif  /* __GEGL_MODULE_DB_H__ */
