/* This file is part of GEGL
 *
 * GEGL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * GEGL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GEGL; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Copyright 2003 Calvin Williamson
 */

#include "config.h"
#include <babl/babl.h>
#include <glib-object.h>
#include "gegl-instrument.h"
#include "gegl-types.h"
#include "gegl-init.h"
#include "buffer/gegl-buffer-allocator.h"
#include "module/geglmodule.h"
#include "module/geglmoduledb.h"
#include <stdlib.h>

static gboolean gegl_initialized = FALSE;

static glong global_time = 0;

void
gegl_init (int *argc,
           char ***argv)
{
  static GeglModuleDB *module_db = NULL;

  if (gegl_initialized)
    return;
  g_assert (global_time == 0);
  global_time = gegl_ticks ();
  g_type_init ();
  babl_init ();

  if (!module_db)
    {
      gchar *load_inhibit = g_strdup ("");
      gchar *module_path;

      setenv ("BABL_ERROR", "0.007", 0);
    
      if (getenv ("GEGL_PATH"))
        {
          module_path = g_strdup (getenv ("GEGL_PATH"));
        }
      else
        module_path  = g_strdup ("/usr/local/lib/gegl");
      
      module_db = gegl_module_db_new (FALSE);
      
      gegl_module_db_set_load_inhibit (module_db, load_inhibit);
      gegl_module_db_load (module_db, module_path);

      g_free (module_path);
      g_free (load_inhibit); 
    }
  gegl_instrument ("gegl", "gegl_init", gegl_ticks () - global_time);
  gegl_initialized = TRUE;
}

void gegl_buffer_stats (void);
void gegl_tile_mem_stats (void);

#include <stdio.h>

void
gegl_exit (void)
{
  long   timing = gegl_ticks ();
  gegl_buffer_allocators_free ();
  babl_destroy ();
  timing = gegl_ticks () - timing;
  gegl_instrument ("gegl", "gegl_exit", timing);

  /* used when tracking buffer leaks (also tracks tiles, as well
   * as tiles.
   */
  if(getenv("GEGL_DEBUG_BUFS")!=NULL)
    {
      gegl_buffer_stats ();
      gegl_tile_mem_stats ();
    }
  global_time = gegl_ticks () - global_time;
  gegl_instrument ("gegl", "gegl", global_time);

  if(getenv("GEGL_DEBUG_TIME")!=NULL)
    {
      printf ("\n%s", gegl_instrument_utf8 ());
    }
}
