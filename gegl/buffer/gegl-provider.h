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

#ifndef __GEGL_PROVIDER_H__
#define __GEGL_PROVIDER_H__

#include <glib-object.h>
#include <babl/babl.h>
#include "gegl-buffer-types.h"
#include "gegl-tile.h"

G_BEGIN_DECLS

#define GEGL_TYPE_PROVIDER            (gegl_provider_get_type ())
#define GEGL_PROVIDER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEGL_TYPE_PROVIDER, GeglProvider))
#define GEGL_PROVIDER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  GEGL_TYPE_PROVIDER, GeglProviderClass))
#define GEGL_IS_PROVIDER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEGL_TYPE_PROVIDER))
#define GEGL_IS_PROVIDER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  GEGL_TYPE_PROVIDER))
#define GEGL_PROVIDER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  GEGL_TYPE_PROVIDER, GeglProviderClass))

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

struct _GeglProvider
{
  GObject  parent_instance;
};

struct _GeglProviderClass
{
  GObjectClass  parent_class;

  gpointer      (*command)  (GeglProvider  *gegl_provider,
                             GeglTileCommand command,
                             gint            x,
                             gint            y,
                             gint            z,
                             gpointer        data);
};

GType      gegl_provider_get_type (void) G_GNUC_CONST;



gpointer   gegl_provider_command  (GeglProvider    *gegl_provider,
                                   GeglTileCommand  command,
                                   gint             x,
                                   gint             y,
                                   gint             z,
                                   gpointer         data);

#define gegl_provider_idle(provider) \
   gegl_provider_command(provider,GEGL_TILE_IDLE,0,0,0,NULL)
#define gegl_provider_set_tile(provider,x,y,z,tile) \
   (gboolean)gegl_provider_command(provider,GEGL_TILE_SET,x,y,z,tile)
#define gegl_provider_get_tile(provider,x,y,z) \
   (GeglTile*)gegl_provider_command(provider,GEGL_TILE_GET,x,y,z,NULL)
#define gegl_provider_is_cached(provider,x,y,z) \
   (gboolean)gegl_provider_command(provider,GEGL_TILE_IS_CACHED,x,y,z,NULL)
#define gegl_provider_exist(provider,x,y,z) \
   (gboolean)gegl_provider_command(provider,GEGL_TILE_EXIST,x,y,z,NULL)
#define gegl_provider_void(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_VOID,x,y,z,NULL)
#define gegl_provider_void_tl(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_VOID_TL,x,y,z,NULL)
#define gegl_provider_void_tr(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_VOID_TR,x,y,z,NULL)
#define gegl_provider_void_bl(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_VOID_BL,x,y,z,NULL)
#define gegl_provider_void_br(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_VOID_BR,x,y,z,NULL)
#define gegl_provider_undo_start_group(provider,x,y,z) \
   gegl_provider_command(provider,GEGL_TILE_UNDO_START_GROUP,x,y,z,NULL)

 
G_END_DECLS

#endif
