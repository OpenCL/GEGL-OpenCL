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
 *
 * gimpmodule.c: * (C) 1999 Austin Donnelly <austin@gegl.org>
 */

#include "config.h"

#include <string.h>

#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "geglmodule.h"


#define gegl_filename_to_utf8(filename) (filename)

enum
{
  MODIFIED,
  LAST_SIGNAL
};


static void       gegl_module_finalize       (GObject     *object);

static gboolean   gegl_module_load           (GTypeModule *module);
static void       gegl_module_unload         (GTypeModule *module);

static gboolean   gegl_module_open           (GeglModule  *module);
static gboolean   gegl_module_close          (GeglModule  *module);
static void       gegl_module_set_last_error (GeglModule  *module,
                                              const gchar *error_str);


G_DEFINE_TYPE (GeglModule, gegl_module, G_TYPE_TYPE_MODULE)

#define parent_class gegl_module_parent_class

static guint module_signals[LAST_SIGNAL];


static void
gegl_module_class_init (GeglModuleClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

  module_signals[MODIFIED] =
    g_signal_new ("modified",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GeglModuleClass, modified),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  object_class->finalize = gegl_module_finalize;

  module_class->load     = gegl_module_load;
  module_class->unload   = gegl_module_unload;

  klass->modified        = NULL;
}

static void
gegl_module_init (GeglModule *module)
{
  module->filename          = NULL;
  module->verbose           = FALSE;
  module->state             = GEGL_MODULE_STATE_ERROR;
  module->on_disk           = FALSE;
  module->load_inhibit      = FALSE;

  module->module            = NULL;
  module->info              = NULL;
  module->last_module_error = NULL;

  module->query_module      = NULL;
  module->register_module   = NULL;
}

static void
gegl_module_finalize (GObject *object)
{
  GeglModule *module = GEGL_MODULE (object);

  if (module->info)
    {
      gegl_module_info_free (module->info);
      module->info = NULL;
    }

  if (module->last_module_error)
    {
      g_free (module->last_module_error);
      module->last_module_error = NULL;
    }

  if (module->filename)
    {
      g_free (module->filename);
      module->filename = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gegl_module_load (GTypeModule *module)
{
  GeglModule *gegl_module = GEGL_MODULE (module);
  gpointer    func;

  g_return_val_if_fail (gegl_module->filename != NULL, FALSE);
  g_return_val_if_fail (gegl_module->module == NULL, FALSE);

  if (gegl_module->verbose)
    g_print ("Loading module '%s'\n",
             gegl_filename_to_utf8 (gegl_module->filename));

  if (! gegl_module_open (gegl_module))
    return FALSE;

  if (! gegl_module_query_module (gegl_module))
    return FALSE;

  /* find the gegl_module_register symbol */
  if (! g_module_symbol (gegl_module->module, "gegl_module_register", &func))
    {
      gegl_module_set_last_error (gegl_module,
                                  "Missing gegl_module_register() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gegl_filename_to_utf8 (gegl_module->filename),
                 gegl_module->last_module_error);

      gegl_module_close (gegl_module);

      gegl_module->state = GEGL_MODULE_STATE_ERROR;

      return FALSE;
    }

  gegl_module->register_module = func;

  if (! gegl_module->register_module (module))
    {
      gegl_module_set_last_error (gegl_module,
                                  "gegl_module_register() returned FALSE");

      g_message (_("Module '%s' load error: %s"),
                 gegl_filename_to_utf8 (gegl_module->filename),
                 gegl_module->last_module_error);

      gegl_module_close (gegl_module);

      gegl_module->state = GEGL_MODULE_STATE_LOAD_FAILED;

      return FALSE;
    }

  gegl_module->state = GEGL_MODULE_STATE_LOADED;

  return TRUE;
}

static void
gegl_module_unload (GTypeModule *module)
{
  GeglModule *gegl_module = GEGL_MODULE (module);

  g_return_if_fail (gegl_module->module != NULL);

  if (gegl_module->verbose)
    g_print ("Unloading module '%s'\n",
             gegl_filename_to_utf8 (gegl_module->filename));

  gegl_module_close (gegl_module);
}


/*  public functions  */

/**
 * gegl_module_new:
 * @filename:     The filename of a loadable module.
 * @load_inhibit: Pass %TRUE to exclude this module from auto-loading.
 * @verbose:      Pass %TRUE to enable debugging output.
 *
 * Creates a new #GeglModule instance.
 *
 * Return value: The new #GeglModule object.
 **/
GeglModule *
gegl_module_new (const gchar *filename,
                 gboolean     load_inhibit,
                 gboolean     verbose)
{
  GeglModule *module;

  g_return_val_if_fail (filename != NULL, NULL);

  module = g_object_new (GEGL_TYPE_MODULE, NULL);

  module->filename     = g_strdup (filename);
  module->load_inhibit = load_inhibit ? TRUE : FALSE;
  module->verbose      = verbose ? TRUE : FALSE;
  module->on_disk      = TRUE;

  if (! module->load_inhibit)
    {
      if (gegl_module_load (G_TYPE_MODULE (module)))
        gegl_module_unload (G_TYPE_MODULE (module));
    }
  else
    {
      if (verbose)
        g_print ("Skipping module '%s'\n",
                 gegl_filename_to_utf8 (filename));

      module->state = GEGL_MODULE_STATE_NOT_LOADED;
    }

  return module;
}

/**
 * gegl_module_query_module:
 * @module: A #GeglModule.
 *
 * Queries the module without actually registering any of the types it
 * may implement. After successful query, the @info field of the
 * #GeglModule struct will be available for further inspection.
 *
 * Return value: %TRUE on success.
 **/
gboolean
gegl_module_query_module (GeglModule *module)
{
  const GeglModuleInfo *info;
  gboolean              close_module = FALSE;
  gpointer              func;

  g_return_val_if_fail (GEGL_IS_MODULE (module), FALSE);

  if (! module->module)
    {
      if (! gegl_module_open (module))
        return FALSE;

      close_module = TRUE;
    }

  /* find the gegl_module_query symbol */
  if (! g_module_symbol (module->module, "gegl_module_query", &func))
    {
      gegl_module_set_last_error (module,
                                  "Missing gegl_module_query() symbol");

      g_message (_("Module '%s' load error: %s"),
                 gegl_filename_to_utf8 (module->filename),
                 module->last_module_error);

      gegl_module_close (module);

      module->state = GEGL_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->query_module = func;

  if (module->info)
    {
      gegl_module_info_free (module->info);
      module->info = NULL;
    }

  info = module->query_module (G_TYPE_MODULE (module));

  if (! info || info->abi_version != GEGL_MODULE_ABI_VERSION)
    {
      gegl_module_set_last_error (module,
                                  info ?
                                  "module ABI version does not match op not loaded, to get rid of this warning remove (clean/uninstall) .so files in GEGLs search path." :
                                  "gegl_module_query() returned NULL");

      g_message (_("Module '%s' load error: %s"),
                 gegl_filename_to_utf8 (module->filename),
                 module->last_module_error);

      gegl_module_close (module);

      module->state = GEGL_MODULE_STATE_ERROR;
      return FALSE;
    }

  module->info = gegl_module_info_copy (info);

  if (close_module)
    return gegl_module_close (module);

  return TRUE;
}

/**
 * gegl_module_modified:
 * @module: A #GeglModule.
 *
 * Emits the "modified" signal. Call it whenever you have modified the module
 * manually (which you shouldn't do).
 **/
void
gegl_module_modified (GeglModule *module)
{
  g_return_if_fail (GEGL_IS_MODULE (module));

  g_signal_emit (module, module_signals[MODIFIED], 0);
}

/**
 * gegl_module_set_load_inhibit:
 * @module:       A #GeglModule.
 * @load_inhibit: Pass %TRUE to exclude this module from auto-loading.
 *
 * Sets the @load_inhibit property if the module. Emits "modified".
 **/
void
gegl_module_set_load_inhibit (GeglModule *module,
                              gboolean    load_inhibit)
{
  g_return_if_fail (GEGL_IS_MODULE (module));

  if (load_inhibit != module->load_inhibit)
    {
      module->load_inhibit = load_inhibit ? TRUE : FALSE;

      gegl_module_modified (module);
    }
}

/**
 * gegl_module_state_name:
 * @state: A #GeglModuleState.
 *
 * Returns the translated textual representation of a #GeglModuleState.
 * The returned string must not be freed.
 *
 * Return value: The @state's name.
 **/
const gchar *
gegl_module_state_name (GeglModuleState state)
{
  static const gchar * const statenames[] =
  {
    N_("Module error"),
    N_("Loaded"),
    N_("Load failed"),
    N_("Not loaded")
  };

  g_return_val_if_fail (state >= GEGL_MODULE_STATE_ERROR &&
                        state <= GEGL_MODULE_STATE_NOT_LOADED, NULL);

  return gettext (statenames[state]);
}

/*  private functions  */

static gboolean
gegl_module_open (GeglModule *module)
{
  module->module = g_module_open (module->filename, 0);

  if (! module->module)
    {
      module->state = GEGL_MODULE_STATE_ERROR;
      gegl_module_set_last_error (module, g_module_error ());

      g_message (_("Module '%s' load error: %s"),
                 gegl_filename_to_utf8 (module->filename),
                 module->last_module_error);

      return FALSE;
    }

  return TRUE;
}

static gboolean
gegl_module_close (GeglModule *module)
{
  g_module_close (module->module); /* FIXME: error handling */
  module->module          = NULL;
  module->query_module    = NULL;
  module->register_module = NULL;

  module->state = GEGL_MODULE_STATE_NOT_LOADED;

  return TRUE;
}

static void
gegl_module_set_last_error (GeglModule  *module,
                            const gchar *error_str)
{
  if (module->last_module_error)
    g_free (module->last_module_error);

  module->last_module_error = g_strdup (error_str);
}


/*  GeglModuleInfo functions  */

/**
 * gegl_module_info_new:
 * @abi_version: The #GEGL_MODULE_ABI_VERSION the module was compiled against.
 * @purpose:     The module's general purpose.
 * @author:      The module's author.
 * @version:     The module's version.
 * @copyright:   The module's copyright.
 * @date:        The module's release date.
 *
 * Creates a newly allocated #GeglModuleInfo struct.
 *
 * Return value: The new #GeglModuleInfo struct.
 **/
GeglModuleInfo *
gegl_module_info_new (guint32 abi_version)
{
  GeglModuleInfo *info = g_slice_new0 (GeglModuleInfo);

  info->abi_version = abi_version;

  return info;
}

/**
 * gegl_module_info_copy:
 * @info: The #GeglModuleInfo struct to copy.
 *
 * Copies a #GeglModuleInfo struct.
 *
 * Return value: The new copy.
 **/
GeglModuleInfo *
gegl_module_info_copy (const GeglModuleInfo *info)
{
  g_return_val_if_fail (info != NULL, NULL);

  return gegl_module_info_new (info->abi_version);
}

/**
 * gegl_module_info_free:
 * @info: The #GeglModuleInfo struct to free
 *
 * Frees the passed #GeglModuleInfo.
 **/
void
gegl_module_info_free (GeglModuleInfo *info)
{
  g_return_if_fail (info != NULL);

  g_slice_free (GeglModuleInfo, info);
}

/**
 * gegl_module_register_type: (skip)
 * @module:
 * @parent_type:
 * @type_name:
 * @type_info:
 * @flags
 *
 * Register a type, checking if another plugin has registered it before.
 *
 * Return value: The created GType
 **/
GType
gegl_module_register_type (GTypeModule     *module,
                           GType            parent_type,
                           const gchar     *type_name,
                           const GTypeInfo *type_info,
                           GTypeFlags       flags)
{
  GType type = g_type_from_name (type_name);
  if (0 && type)
    {
      GTypePlugin *old_plugin = g_type_get_plugin (type);

      if (old_plugin != G_TYPE_PLUGIN (module))
        {
          g_warning ("EeeK");
          return -1;
          /* ignoring loading of plug-in from second source */
          return type;
        }
    }
  return g_type_module_register_type (module,
                                      parent_type,
                                      type_name,
                                      type_info,
                                      flags);
}
