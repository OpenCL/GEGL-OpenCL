/* this file is part of GEGL
 *
 * Datafiles module copyight (C) 1996 Federico Mena Quintero
 * federico@nuclecu.unam.mx
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
 * Datafiles module copyight (C) 1996 Federico Mena Quintero
 * federico@nuclecu.unam.mx
 */

#include "config.h"

#include <string.h>

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib-object.h>
#include <glib/gstdio.h>

#ifdef G_OS_WIN32
#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif
#endif

/*
#include "geglbasetypes.h"*/

#include "gegldatafiles.h"

gboolean
gegl_datafiles_check_extension (const gchar *filename,
                                const gchar *extension)
{
  gint name_len;
  gint ext_len;

  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (extension != NULL, FALSE);

  name_len = strlen (filename);
  ext_len  = strlen (extension);

  if (! (name_len && ext_len && (name_len > ext_len)))
    return FALSE;

  return (g_ascii_strcasecmp (&filename[name_len - ext_len], extension) == 0);
}

/**
 * gegl_path_parse:
 * @path:         A list of directories separated by #G_SEARCHPATH_SEPARATOR.
 * @max_paths:    The maximum number of directories to return.
 * @check:        %TRUE if you want the directories to be checked.
 * @check_failed: Returns a #GList of path elements for which the
 *                check failed.
 *
 * Returns: A #GList of all directories in @path.
 **/
static GList *
gegl_path_parse (const gchar  *path,
                 gint          max_paths,
                 gboolean      check,
                 GList       **check_failed)
{
  const gchar  *home;
  gchar       **patharray;
  GList        *list      = NULL;
  GList        *fail_list = NULL;
  gint          i;
  gboolean      exists    = TRUE;

  if (!path || !*path || max_paths < 1 || max_paths > 256)
    return NULL;

  home = g_get_home_dir ();

  patharray = g_strsplit (path, G_SEARCHPATH_SEPARATOR_S, max_paths);

  for (i = 0; i < max_paths; i++)
    {
      GString *dir;

      if (! patharray[i])
        break;

#ifndef G_OS_WIN32
      if (*patharray[i] == '~')
        {
          dir = g_string_new (home);
          g_string_append (dir, patharray[i] + 1);
        }
      else
#endif
        {
          dir = g_string_new (patharray[i]);
        }

      if (check)
        exists = g_file_test (dir->str, G_FILE_TEST_IS_DIR);

      if (exists)
        list = g_list_prepend (list, g_strdup (dir->str));
      else if (check_failed)
        fail_list = g_list_prepend (fail_list, g_strdup (dir->str));

      g_string_free (dir, TRUE);
    }

  g_strfreev (patharray);

  list = g_list_reverse (list);

  if (check && check_failed)
    {
      fail_list = g_list_reverse (fail_list);
      *check_failed = fail_list;
    }

  return list;
}

/**
 * gegl_path_free:
 * @path: A list of directories as returned by gegl_path_parse().
 *
 * This function frees the memory allocated for the list and the strings
 * it contains.
 **/
static void
gegl_path_free (GList *path)
{
  g_list_foreach (path, (GFunc) g_free, NULL);
  g_list_free (path);
}

void
gegl_datafiles_read_directories (const gchar            *path_str,
                                 GFileTest               flags,
                                 GeglDatafileLoaderFunc  loader_func,
                                 gpointer                user_data)
{
  GeglDatafileData  file_data;
  struct stat       filestat;
  gchar            *local_path;
  GList            *path;
  GList            *list;
  gchar            *filename;
  gint              err;
  GDir             *dir;
  const gchar      *dir_ent;

  g_return_if_fail (path_str != NULL);
  g_return_if_fail (loader_func != NULL);

  local_path = g_strdup (path_str);

  path = gegl_path_parse (local_path, 16, TRUE, NULL);

  for (list = path; list; list = g_list_next (list))
    {
      const gchar *dirname = list->data;

      dir = g_dir_open (dirname, 0, NULL);

      if (dir)
        {
          while ((dir_ent = g_dir_read_name (dir)))
            {
              filename = g_build_filename (dirname, dir_ent, NULL);

              err = g_stat (filename, &filestat);

              file_data.filename = filename;
              file_data.dirname  = dirname;
              file_data.basename = dir_ent;
              file_data.atime    = filestat.st_atime;
              file_data.mtime    = filestat.st_mtime;
              file_data.ctime    = filestat.st_ctime;

              if (! err)
                {
                  if (/*(flags & G_FILE_TEST_IS_DIR) &&*/
                           S_ISDIR (filestat.st_mode))
                    {
                      gegl_datafiles_read_directories (filename,
                                                       flags,
                                                       loader_func,
                                                       user_data);
                    }
                  else if (flags & G_FILE_TEST_EXISTS)
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                  else if ((flags & G_FILE_TEST_IS_REGULAR) &&
                           S_ISREG (filestat.st_mode))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
#ifndef G_OS_WIN32
                  else if ((flags & G_FILE_TEST_IS_SYMLINK) &&
                           S_ISLNK (filestat.st_mode))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
#endif
                  else if ((flags & G_FILE_TEST_IS_EXECUTABLE) &&
                           (((filestat.st_mode & S_IXUSR) &&
                             !S_ISDIR (filestat.st_mode)) ||
                            (S_ISREG (filestat.st_mode) /*&&
                             is_script (filename)*/)))
                    {
                      (* loader_func) (&file_data, user_data);
                    }
                }

              g_free (filename);
            }

          g_dir_close (dir);
        }
    }

  gegl_path_free (path);
  g_free (local_path);
}
