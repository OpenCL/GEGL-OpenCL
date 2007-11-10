/* This file is part of GEGL editor -- a gtk frontend for GEGL
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2003, 2004, 2006 Øyvind Kolås
 */
#ifndef GEGL_STORE_H
#define GEGL_STORE_H

#include <gtk/gtk.h>
#include <gegl.h>

#define TYPE_GEGL_STORE                  (gegl_store_get_type ())
#define GEGL_STORE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_GEGL_STORE, GeglStore))
#define GEGL_STORE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_GEGL_STORE, GeglStoreClass))
#define IS_GEGL_STORE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_GEGL_STORE))
#define IS_GEGL_STORE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_GEGL_STORE))
#define GEGL_STORE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_GEGL_STORE, GeglStoreClass))

/* The data columns that we export via the tree model interface */

enum
{
  GEGL_STORE_COL_ITEM = 0,
  GEGL_STORE_COL_TYPE,
  GEGL_STORE_COL_TYPESTR,
  GEGL_STORE_COL_ICON,
  GEGL_STORE_COL_NAME,
  GEGL_STORE_COL_SUMMARY,
  GEGL_STORE_N_COLUMNS
};

typedef struct _GeglStore GeglStore;
typedef struct _GeglStoreClass GeglStoreClass;


struct _GeglStore
{
  GObject    parent;

  GeglNode  *gegl;
  GeglNode  *root;
  gint       n_columns;
  GType      column_types[GEGL_STORE_N_COLUMNS];
  gint       stamp;

};

/* GeglStoreClass: more boilerplate GObject stuff */

struct _GeglStoreClass
{
  GObjectClass parent_class;
};

GType       gegl_store_get_type (void);
GeglStore * gegl_store_new      (void);
void        gegl_store_set_gegl (GeglStore  *gegl_store,
                                 GeglNode   *gegl);
void        gegl_store_freeze   (GeglStore  *gegl_store);
void        gegl_store_thaw     (GeglStore  *gegl_store);
void        gegl_store_process  (GeglStore  *gegl_store);

#endif /* _gegl_store_h_included_ */
