/* This file is part of GEGL.
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
 * Copyright 2006 Øyvind Kolås <pippin@gimp.org>
 */

#ifndef __GEGL_TILE_SOURCE_H__
#define __GEGL_TILE_SOURCE_H__

#include <glib-object.h>
#include <babl/babl.h>
#include "gegl-buffer-types.h"
#include "gegl-tile.h"

G_BEGIN_DECLS

#define GEGL_TYPE_TILE_SOURCE       (gegl_tile_source_get_type ())
#define GEGL_TILE_SOURCE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_TILE_SOURCE, GeglTileSource))
#define GEGL_TILE_SOURCE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_TILE_SOURCE, GeglTileSourceClass))
#define GEGL_IS_TILE_SOURCE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_TILE_SOURCE))
#define GEGL_IS_TILE_SOURCE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_TILE_SOURCE))
#define GEGL_TILE_SOURCE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_TILE_SOURCE, GeglTileSourceClass))

typedef gint GeglTileCommand;

enum _GeglTileCommand
{
  GEGL_TILE_IDLE = 0,
  GEGL_TILE_SET,
  GEGL_TILE_GET,
  GEGL_TILE_IS_CACHED,
  GEGL_TILE_EXIST,
  GEGL_TILE_VOID,
  GEGL_TILE_VOID_TL,
  GEGL_TILE_VOID_TR,
  GEGL_TILE_VOID_BL,
  GEGL_TILE_VOID_BR,
  GEGL_TILE_UNDO_START_GROUP,
  GEGL_TILE_LAST_COMMAND
};

struct _GeglTileSource
{
  GObject  parent_instance;
};

struct _GeglTileSourceClass
{
  GObjectClass  parent_class;

  gpointer      (*command)  (GeglTileSource  *gegl_tile_source,
                             GeglTileCommand command,
                             gint            x,
                             gint            y,
                             gint            z,
                             gpointer        data);
};

GType      gegl_tile_source_get_type (void) G_GNUC_CONST;



gpointer   gegl_tile_source_command  (GeglTileSource    *gegl_tile_source,
                                   GeglTileCommand  command,
                                   gint             x,
                                   gint             y,
                                   gint             z,
                                   gpointer         data);

#define gegl_tile_source_idle(source) \
   gegl_tile_source_command(source,GEGL_TILE_IDLE,0,0,0,NULL)

#define gegl_tile_source_set_tile(source,x,y,z,tile) \
   (gboolean)gegl_tile_source_command(source,GEGL_TILE_SET,x,y,z,tile)

#define gegl_tile_source_get_tile(source,x,y,z) \
   (GeglTile*)gegl_tile_source_command(source,GEGL_TILE_GET,x,y,z,NULL)

#define gegl_tile_source_is_cached(source,x,y,z) \
   (gboolean)gegl_tile_source_command(source,GEGL_TILE_IS_CACHED,x,y,z,NULL)

#define gegl_tile_source_exist(source,x,y,z) \
   (gboolean)gegl_tile_source_command(source,GEGL_TILE_EXIST,x,y,z,NULL)

#define gegl_tile_source_void(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_VOID,x,y,z,NULL)

#define gegl_tile_source_void_tl(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_VOID_TL,x,y,z,NULL)

#define gegl_tile_source_void_tr(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_VOID_TR,x,y,z,NULL)

#define gegl_tile_source_void_bl(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_VOID_BL,x,y,z,NULL)

#define gegl_tile_source_void_br(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_VOID_BR,x,y,z,NULL)

#define gegl_tile_source_undo_start_group(source,x,y,z) \
   gegl_tile_source_command(source,GEGL_TILE_UNDO_START_GROUP,x,y,z,NULL)

 
G_END_DECLS

#endif
