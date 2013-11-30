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
 * Copyright 2003 Calvin Williamson, Øyvind Kolås
 *           2013 Daniel Sabo
 */

#ifndef __GEGL_INIT_H__
#define __GEGL_INIT_H__

G_BEGIN_DECLS

/***
 * Initialization:
 *
 * Before GEGL can be used the engine should be initialized by either calling
 * #gegl_init or through the use of #gegl_get_option_group. To shut down the
 * GEGL engine call #gegl_exit.
 *
 * ---Code sample:
 * #include <gegl.h>
 *
 * int main(int argc, char **argv)
 * {
 *   gegl_init (&argc, &argv);
 *       # other GEGL code
 *   gegl_exit ();
 * }
 */

/**
 * gegl_init:
 * @argc: (inout): a pointer to the number of command line arguments.
 * @argv: (inout) (array length=argc) (allow-none): a pointer to the array of command line arguments.
 *
 * Call this function before using any other GEGL functions. It will
 * initialize everything needed to operate GEGL and parses some
 * standard command line options.  @argc and @argv are adjusted
 * accordingly so your own code will never see those standard
 * arguments.
 *
 * Note that there is an alternative way to initialize GEGL: if you
 * are calling g_option_context_parse() with the option group returned
 * by #gegl_get_option_group(), you don't have to call #gegl_init().
 **/
void          gegl_init                  (gint          *argc,
                                          gchar       ***argv);
/**
 * gegl_get_option_group: (skip)
 *
 * Returns a GOptionGroup for the commandline arguments recognized
 * by GEGL. You should add this group to your GOptionContext
 * with g_option_context_add_group() if you are using
 * g_option_context_parse() to parse your commandline arguments.
 */
GOptionGroup *gegl_get_option_group      (void);

/**
 * gegl_exit:
 *
 * Call this function when you're done using GEGL. It will clean up
 * caches and write/dump debug information if the correct debug flags
 * are set.
 */
void          gegl_exit                  (void);

/**
 * gegl_load_module_directory:
 * @path: the directory to load modules from
 *
 * Load all gegl modules found in the given directory.
 */
void          gegl_load_module_directory (const gchar *path);

/**
 * gegl_config:
 *
 * Returns a GeglConfig object with properties that can be manipulated to control
 * GEGLs behavior.
 *
 * Return value: (transfer none): a #GeglConfig
 */
GeglConfig   *gegl_config                (void);

G_END_DECLS

#endif /* __GEGL_INIT_H__ */
