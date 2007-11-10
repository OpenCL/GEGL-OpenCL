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
 * Copyright (C) 2006 Øyvind Kolås
 */

#ifndef DEDITOR_H
#define DEDITOR_H

#include <glib-object.h>
#include <gegl-plugin.h>
#include <gegl-module.h>
#include "../gegl-node-editor.h"

void gegl_gui_flush (void);

#define EDITOR_DEFINE_TYPE_EXTENDED(TypeName, type_name, TYPE_PARENT, flags, CODE) \
  \
static void     type_name##_init          (TypeName        *self); \
static void     type_name##_class_init    (TypeName##Class *klass); \
static GType    type_name##_get_type      (GTypeModule     *module); \
const GeglModuleInfo * gegl_module_query  (GTypeModule     *module);\
gboolean        gegl_module_register      (GTypeModule     *module);\
static gpointer type_name##_parent_class = NULL; \
static void \
type_name##_class_intern_init (gpointer klass) \
{ \
  type_name##_parent_class = g_type_class_peek_parent (klass); \
  type_name##_class_init ((TypeName##Class*) klass); \
} \
\
static GType \
type_name##_get_type (GTypeModule *module) \
{ \
  static GType g_define_type_id = 0; \
  if (G_UNLIKELY (g_define_type_id == 0)) \
    { \
      static const GTypeInfo g_define_type_info = \
        { \
          sizeof (TypeName##Class), \
          (GBaseInitFunc) NULL, \
          (GBaseFinalizeFunc) NULL, \
          (GClassInitFunc) type_name##_class_intern_init, \
          (GClassFinalizeFunc) NULL, \
          NULL,   /* class_data */ \
          sizeof (TypeName), \
          0,      /* n_preallocs */ \
          (GInstanceInitFunc) type_name##_init, \
          NULL    /* value_table */ \
        }; \
      g_define_type_id = gegl_module_register_type (module, TYPE_PARENT,\
                                                    "GeglNodeEditor" #TypeName,\
                                                    &g_define_type_info, \
                                                    (GTypeFlags) flags);\
    } \
  return g_define_type_id; \
}\
\
static const GeglModuleInfo modinfo =\
{\
 GEGL_MODULE_ABI_VERSION,\
 "GeglNodeEditor"#TypeName,\
 "v0.0",\
 "(c) 2006, released under the LGPL",\
 "June 2006"\
};\
\
const GeglModuleInfo *\
gegl_module_query (GTypeModule *module)\
{\
  return &modinfo;\
}\
\
gboolean \
gegl_module_register (GTypeModule *module)\
{\
  type_name##_get_type (module);\
  return TRUE;\
}

#define EDITOR_DEFINE_TYPE(TN, t_n, T_P)  EDITOR_DEFINE_TYPE_EXTENDED (TN, t_n, T_P, 0, )

#endif
