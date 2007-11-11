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

#ifndef __GEGL_DATAFILES_H__
#define __GEGL_DATAFILES_H__

#include <time.h>

G_BEGIN_DECLS


typedef struct _GeglDatafileData GeglDatafileData;

struct _GeglDatafileData
{
  const gchar *filename;
  const gchar *dirname;
  const gchar *basename;

  time_t       atime;
  time_t       mtime;
  time_t       ctime;
};

typedef void (* GeglDatafileLoaderFunc) (const GeglDatafileData *file_data,
                                         gpointer                user_data);



gboolean   gegl_datafiles_check_extension  (const gchar            *filename,
                                            const gchar            *extension);

void       gegl_datafiles_read_directories (const gchar            *path_str,
                                            GFileTest               flags,
                                            GeglDatafileLoaderFunc  loader_func,
                                            gpointer                user_data);


G_END_DECLS

#endif  /*  __GEGL_DATAFILES_H__ */
