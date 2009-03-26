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

#include "config.h"
#include <string.h>

#include <glib-object.h>
#include "gegl-plugin.h"
#include "geglmodule.h"
#include "geglmoduledb.h"
#include "gegldatafiles.h"

enum
{
  ADD,
  REMOVE,
  MODULE_MODIFIED,
  LAST_SIGNAL
};


/*  #define DUMP_DB 1  */


static void         gegl_module_db_finalize            (GObject      *object);

static void         gegl_module_db_module_initialize   (const GeglDatafileData *file_data,
                                                        gpointer                user_data);

static GeglModule * gegl_module_db_module_find_by_path (GeglModuleDB *db,
                                                        const char   *fullpath);

#ifdef DUMP_DB
static void         gegl_module_db_dump_module         (gpointer      data,
                                                        gpointer      user_data);
#endif

static void         gegl_module_db_module_on_disk_func (gpointer      data,
                                                        gpointer      user_data);
static void         gegl_module_db_module_remove_func  (gpointer      data,
                                                        gpointer      user_data);
static void         gegl_module_db_module_modified     (GeglModule   *module,
                                                        GeglModuleDB *db);


G_DEFINE_TYPE (GeglModuleDB, gegl_module_db, G_TYPE_OBJECT)

#define parent_class gegl_module_db_parent_class

static guint db_signals[LAST_SIGNAL] = { 0 };


static void
gegl_module_db_class_init (GeglModuleDBClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  db_signals[ADD] =
    g_signal_new ("add",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, add),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  db_signals[REMOVE] =
    g_signal_new ("remove",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, remove),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  db_signals[MODULE_MODIFIED] =
    g_signal_new ("module-modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleDBClass, module_modified),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GEGL_TYPE_MODULE);

  object_class->finalize = gegl_module_db_finalize;

  klass->add             = NULL;
  klass->remove          = NULL;
}

static void
gegl_module_db_init (GeglModuleDB *db)
{
  db->modules      = NULL;
  db->load_inhibit = NULL;
  db->verbose      = FALSE;
}

static void
gegl_module_db_finalize (GObject *object)
{
  GeglModuleDB *db = GEGL_MODULE_DB (object);

  if (db->modules)
    {
      g_list_free (db->modules);
      db->modules = NULL;
    }

  if (db->load_inhibit)
    {
      g_free (db->load_inhibit);
      db->load_inhibit = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gegl_module_db_new:
 * @verbose: Pass %TRUE to enable debugging output.
 *
 * Creates a new #GeglModuleDB instance. The @verbose parameter will be
 * passed to the created #GeglModule instances using gegl_module_new().
 *
 * Return value: The new #GeglModuleDB instance.
 **/
GeglModuleDB *
gegl_module_db_new (gboolean verbose)
{
  GeglModuleDB *db;

  db = g_object_new (GEGL_TYPE_MODULE_DB, NULL);

  db->verbose = verbose ? TRUE : FALSE;

  return db;
}

static gboolean
is_in_inhibit_list (const gchar *filename,
                    const gchar *inhibit_list)
{
  gchar       *p;
  gint         pathlen;
  const gchar *start;
  const gchar *end;

  if (! inhibit_list || ! strlen (inhibit_list))
    return FALSE;

  p = strstr (inhibit_list, filename);
  if (!p)
    return FALSE;

  /* we have a substring, but check for colons either side */
  start = p;
  while (start != inhibit_list && *start != G_SEARCHPATH_SEPARATOR)
    start--;

  if (*start == G_SEARCHPATH_SEPARATOR)
    start++;

  end = strchr (p, G_SEARCHPATH_SEPARATOR);
  if (! end)
    end = inhibit_list + strlen (inhibit_list);

  pathlen = strlen (filename);

  if ((end - start) == pathlen)
    return TRUE;

  return FALSE;
}

/**
 * gegl_module_db_set_load_inhibit:
 * @db:           A #GeglModuleDB.
 * @load_inhibit: A #G_SEARCHPATH_SEPARATOR delimited list of module
 *                filenames to exclude from auto-loading.
 *
 * Sets the @load_inhibit flag for all #GeglModule's which are kept
 * by @db (using gegl_module_set_load_inhibit()).
 **/
void
gegl_module_db_set_load_inhibit (GeglModuleDB *db,
                                 const gchar  *load_inhibit)
{
  GList *list;

  g_return_if_fail (GEGL_IS_MODULE_DB (db));

  if (db->load_inhibit)
    g_free (db->load_inhibit);

  db->load_inhibit = g_strdup (load_inhibit);

  for (list = db->modules; list; list = g_list_next (list))
    {
      GeglModule *module = list->data;

      gegl_module_set_load_inhibit (module,
                                    is_in_inhibit_list (module->filename,
                                                        load_inhibit));
    }
}

/**
 * gegl_module_db_get_load_inhibit:
 * @db: A #GeglModuleDB.
 *
 * Return the #G_SEARCHPATH_SEPARATOR selimited list of module filenames
 * which are excluded from auto-loading.
 *
 * Return value: the @db's @load_inhibit string.
 **/
const gchar *
gegl_module_db_get_load_inhibit (GeglModuleDB *db)
{
  g_return_val_if_fail (GEGL_IS_MODULE_DB (db), NULL);

  return db->load_inhibit;
}

/**
 * gegl_module_db_load:
 * @db:          A #GeglModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Scans the directories contained in @module_path using
 * gegl_datafiles_read_directories() and creates a #GeglModule
 * instance for every loadable module contained in the directories.
 **/
void
gegl_module_db_load (GeglModuleDB *db,
                     const gchar  *module_path)
{
  g_return_if_fail (GEGL_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  if (g_module_supported ())
    gegl_datafiles_read_directories (module_path,
                                     G_FILE_TEST_EXISTS,
                                     gegl_module_db_module_initialize,
                                     db);

#ifdef DUMP_DB
  g_list_foreach (db->modules, gegl_module_db_dump_module, NULL);
#endif
}

/**
 * gegl_module_db_refresh:
 * @db:          A #GeglModuleDB.
 * @module_path: A #G_SEARCHPATH_SEPARATOR delimited list of directories
 *               to load modules from.
 *
 * Does the same as gegl_module_db_load(), plus removes all #GeglModule
 * instances whose modules have been deleted from disk.
 *
 * Note that the #GeglModule's will just be removed from the internal
 * list and not freed as this is not possible with #GTypeModule
 * instances which actually implement types.
 **/
void
gegl_module_db_refresh (GeglModuleDB *db,
                        const gchar  *module_path)
{
  GList *kill_list = NULL;

  g_return_if_fail (GEGL_IS_MODULE_DB (db));
  g_return_if_fail (module_path != NULL);

  /* remove modules we don't have on disk anymore */
  g_list_foreach (db->modules,
                  gegl_module_db_module_on_disk_func,
                  &kill_list);
  g_list_foreach (kill_list,
                  gegl_module_db_module_remove_func,
                  db);
  g_list_free (kill_list);

  /* walk filesystem and add new things we find */
  gegl_datafiles_read_directories (module_path,
                                   G_FILE_TEST_EXISTS,
                                   gegl_module_db_module_initialize,
                                   db);
}

/* name must be of the form lib*.so (Unix) or *.dll (Win32) */
static gboolean
valid_module_name (const gchar *filename)
{
  gchar *basename = g_path_get_basename (filename);

  if (! gegl_datafiles_check_extension (basename, "." G_MODULE_SUFFIX))
    {
      g_free (basename);

      return FALSE;
    }

  g_free (basename);

  return TRUE;
}

static void
gegl_module_db_module_initialize (const GeglDatafileData *file_data,
                                  gpointer                user_data)
{
  GeglModuleDB *db = GEGL_MODULE_DB (user_data);
  GeglModule   *module;
  gboolean      load_inhibit;

  if (! valid_module_name (file_data->filename))
    return;

  /* don't load if we already know about it */
  if (gegl_module_db_module_find_by_path (db, file_data->filename))
    return;

  load_inhibit = is_in_inhibit_list (file_data->filename,
                                     db->load_inhibit);

  module = gegl_module_new (file_data->filename,
                            load_inhibit,
                            db->verbose);

  g_signal_connect (module, "modified",
                    G_CALLBACK (gegl_module_db_module_modified),
                    db);

  db->modules = g_list_append (db->modules, module);
  g_signal_emit (db, db_signals[ADD], 0, module);
}

static GeglModule *
gegl_module_db_module_find_by_path (GeglModuleDB *db,
                                    const char   *fullpath)
{
  GList *list;

  for (list = db->modules; list; list = g_list_next (list))
    {
      GeglModule *module = list->data;

      if (! strcmp (module->filename, fullpath))
        return module;
    }

  return NULL;
}

#ifdef DUMP_DB
static void
gegl_module_db_dump_module (gpointer data,
                            gpointer user_data)
{
  GeglModule *module = data;

  g_print ("\n%s: %s\n",
           module->filename,
           gegl_module_state_name (module->state));

  g_print ("  module: %p  lasterr: %s  query: %p register: %p\n",
           module->module,
           module->last_module_error ? module->last_module_error : "NONE",
           module->query_module,
           module->register_module);
}
#endif

static void
gegl_module_db_module_on_disk_func (gpointer data,
                                    gpointer user_data)
{
  GeglModule  *module    = data;
  GList      **kill_list = user_data;
  gboolean     old_on_disk;

  old_on_disk = module->on_disk;

  module->on_disk = g_file_test (module->filename, G_FILE_TEST_IS_REGULAR);

  /* if it's not on the disk, and it isn't in memory, mark it to be
   * removed later.
   */
  if (! module->on_disk && ! module->module)
    {
      *kill_list = g_list_append (*kill_list, module);
      module = NULL;
    }

  if (module && module->on_disk != old_on_disk)
    gegl_module_modified (module);
}

static void
gegl_module_db_module_remove_func (gpointer data,
                                   gpointer user_data)
{
  GeglModule   *module = data;
  GeglModuleDB *db     = user_data;

  g_signal_handlers_disconnect_by_func (module,
                                        gegl_module_db_module_modified,
                                        db);

  db->modules = g_list_remove (db->modules, module);

  g_signal_emit (db, db_signals[REMOVE], 0, module);
}

static void
gegl_module_db_module_modified (GeglModule   *module,
                                GeglModuleDB *db)
{
  g_signal_emit (db, db_signals[MODULE_MODIFIED], 0, module);
}
