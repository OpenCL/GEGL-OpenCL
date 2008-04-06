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
 * Copyright 2003-2007 Calvin Williamson, Øyvind Kolås
 */

#include "config.h"

#include <babl/babl.h>

#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef G_OS_WIN32
#include <process.h>
#endif

#include "gegl-instrument.h"
/*#include "gegl-types.h"*/
#include "gegl-init.h"

#include "module/geglmodule.h"
#include "module/geglmoduledb.h"

#include "buffer/gegl-buffer-allocator.h"

#include "operation/gegl-operation.h"
#include "operation/gegl-operations.h"
#include "operation/gegl-extension-handler.h"


static gboolean  gegl_post_parse_hook (GOptionContext *context,
                                       GOptionGroup   *group,
                                       gpointer        data,
                                       GError        **error);


static gboolean      gegl_initialized = FALSE;

static GeglModuleDB *module_db   = NULL;

static glong         global_time = 0;

static const gchar *makefile (void);

/**
 * gegl_init:
 * @argc: a pointer to the number of command line arguments.
 * @argv: a pointer to the array of command line arguments.
 *
 * Call this function before using any other GEGL functions. It will initialize
 * everything needed to operate GEGL and parses some standard command line
 * options.  @argc and @argv are adjusted accordingly so your own code will
 * never see those standard arguments.
 *
 * Note that there is an alternative ways to initialize GEGL: if you are
 * calling g_option_context_parse() with the option group returned by
 * gegl_get_option_group(), you don't have to call gegl_init().
 **/
void
gegl_init (gint    *argc,
           gchar ***argv)
{
  if (gegl_initialized)
    return;

#if ENABLE_MP
  if (!g_thread_supported())
    g_thread_init (NULL);
#endif

  /*  If any command-line actions are ever added to GEGL, then the commented
   *  out code below should be used.  Until then, we simply call the parse hook
   *  directly.
   */
  gegl_post_parse_hook (NULL, NULL, NULL, NULL);

#if 0
  GOptionContext *context;
  GError         *error = NULL;

  context = g_option_context_new (NULL);
  g_option_context_set_ignore_unknown_options (context, TRUE);
  g_option_context_set_help_enabled (context, FALSE);
  g_option_context_set_main_group (context, gegl_get_option_group ());

  if (!g_option_context_parse (context, argc, argv, &error))
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

  g_option_context_free (context);
#endif
}

/**
 * gegl_get_option_group:
 *
 * Returns a #GOptionGroup for the commandline arguments recognized
 * by GEGL. You should add this group to your #GOptionContext
 * with g_option_context_add_group(), if you are using
 * g_option_context_parse() to parse your commandline arguments.
 *
 * Returns a #GOptionGroup for the commandline arguments recognized by GEGL.
 */
GOptionGroup *
gegl_get_option_group (void)
{
  GOptionGroup *group;

  group = g_option_group_new ("gegl", "GEGL Options", "Show GEGL Options",
                              NULL, NULL);

  g_option_group_set_parse_hooks (group, NULL, gegl_post_parse_hook);

  return group;
}

void gegl_tile_mem_stats (void);

void
gegl_exit (void)
{
  glong timing = gegl_ticks ();

  gegl_operation_gtype_cleanup ();
  gegl_extension_handler_cleanup ();
  gegl_buffer_allocators_free ();

  if (module_db != NULL)
    {
      g_object_unref (module_db);
      module_db = NULL;
    }

  babl_destroy ();

  timing = gegl_ticks () - timing;
  gegl_instrument ("gegl", "gegl_exit", timing);

  /* used when tracking buffer and tile leaks */
  if (g_getenv ("GEGL_DEBUG_BUFS") != NULL)
    {
      gegl_buffer_stats ();
      gegl_tile_mem_stats ();
    }
  global_time = gegl_ticks () - global_time;
  gegl_instrument ("gegl", "gegl", global_time);

  if (g_getenv ("GEGL_DEBUG_TIME") != NULL)
    {
      g_print ("\n%s", gegl_instrument_utf8 ());
    }

  if (gegl_buffer_leaks ())
    g_print ("  buffer-leaks: %i", gegl_buffer_leaks ());

  if (gegl_swap_dir())
    {
      /* remove all files matching <$GEGL_SWAP>/GEGL-<pid>-*.swap */

      guint         pid     = getpid ();
      GDir         *dir     = g_dir_open (gegl_swap_dir (), 0, NULL);

      gchar        *glob    = g_strdup_printf ("GEGL-%i-*.swap", pid);
      GPatternSpec *pattern = g_pattern_spec_new (glob);
      g_free (glob);

      if (dir != NULL)
        {
          const gchar *name;

          while ((name = g_dir_read_name (dir)) != NULL)
            {
              if (g_pattern_match_string (pattern, name))
                {
                  gchar *fname = g_build_filename (gegl_swap_dir (),
                                                   name,
                                                   NULL);
                  g_unlink (fname);
                  g_free (fname);
                }
            }

          g_dir_close (dir);
        }

      g_pattern_spec_free (pattern);
    }

  g_print ("\n");
}

void
gegl_get_version (int *major,
		  int *minor,
		  int *micro)
{
  if (major != NULL)
    *major = GEGL_MAJOR_VERSION;

  if (minor != NULL)
    *minor = GEGL_MINOR_VERSION;

  if (micro != NULL)
    *micro = GEGL_MICRO_VERSION;
}


static void
gegl_init_i18n (void)
{
  bindtextdomain (GETTEXT_PACKAGE, GEGL_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
}

static gboolean
gegl_post_parse_hook (GOptionContext *context,
                      GOptionGroup   *group,
                      gpointer        data,
                      GError        **error)
{
  glong time;

  if (gegl_initialized)
    return TRUE;

  g_assert (global_time == 0);
  global_time = gegl_ticks ();
  g_type_init ();
  gegl_instrument ("gegl", "gegl_init", 0);

  time = gegl_ticks ();

  if (g_getenv ("BABL_ERROR") == NULL)
    g_setenv ("BABL_ERROR", "0.0001", 0);

  babl_init ();
  gegl_instrument ("gegl_init", "babl_init", gegl_ticks () - time);

  gegl_init_i18n ();

  time = gegl_ticks ();
  if (!module_db)
    {
      const gchar *gegl_path = g_getenv ("GEGL_PATH");

      module_db = gegl_module_db_new (FALSE);

      if (gegl_path)
        {
          gegl_module_db_load (module_db, gegl_path);
        }
      else
        {
          gchar *module_path;

#ifdef G_OS_WIN32
          module_path =
            g_win32_get_package_installation_subdirectory (NULL,
                                                           "lib" GEGL_LIBRARY "-0.dll",
                                                           "lib/" GEGL_LIBRARY);
#else
          module_path = g_build_filename (LIBDIR, GEGL_LIBRARY, NULL);
#endif

          gegl_module_db_load (module_db, module_path);
          g_free (module_path);

          /* also load plug-ins from ~/.gegl-0.0/plug-ins */
          module_path = g_build_filename (g_get_home_dir (),
                                          "." GEGL_LIBRARY,
                                          "plug-ins",
                                          NULL);

          if (g_mkdir_with_parents (module_path,
                                    S_IRUSR | S_IWUSR | S_IXUSR) == 0)
            {
              gchar *makefile_path = g_build_filename (module_path,
                                                       "Makefile",
                                                       NULL);

              if (! g_file_test (makefile_path, G_FILE_TEST_EXISTS))
                g_file_set_contents (makefile_path, makefile (), -1, NULL);

              g_free (makefile_path);
            }

          gegl_module_db_load (module_db, module_path);
          g_free (module_path);
        }

      gegl_instrument ("gegl_init", "load modules", gegl_ticks () - time);
    }

  gegl_instrument ("gegl", "gegl_init", gegl_ticks () - global_time);
  gegl_initialized = TRUE;

  return TRUE;
}

static const gchar *makefile (void)
{
  return
    "# This is a generic makefile for GEGL operations. Just add .c files,\n"
    "# rename mentions of the filename and opname to the new name, and it should \n"
    "# compile. Operations in this dir should be loaded by GEGL by default\n"
    "# If the operation being written depends on extra libraries, you'd better\n"
    "# add a dedicated target with the extra bits linked in.\n"
    "\n\n"
    "CFLAGS  += `pkg-config gegl --cflags`  -I. -fPIC\n"
    "LDFLAGS += `pkg-config --libs` -shared\n"
    "SHREXT=.so\n"
    "CFILES = $(wildcard ./*.c)\n"
    "SOBJS  = $(subst ./,,$(CFILES:.c=$(SHREXT)))\n"
    "all: $(SOBJS)\n"
    "%$(SHREXT): %.c $(GEGLHEADERS)\n"
    "	@echo $@; $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDADD)\n"
    "clean:\n"
    "	rm -f *$(SHREXT) $(OFILES)\n";
}
